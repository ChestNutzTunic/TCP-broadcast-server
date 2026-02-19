#include "crypto.h"
#include "dyn_arr.h"

DWORD WINAPI receiveMessage(LPVOID S);
DWORD WINAPI sendMessage(LPVOID S);

CRITICAL_SECTION CS;

int main() {
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);

    InitializeCriticalSection(&CS);

    char hostName[100];
    struct hostent* hostData;
    struct in_addr** addr_list;

    if(gethostname(hostName, sizeof(hostName)) == SOCKET_ERROR){
        printf("Error finding host\n");
        return SOCKET_ERROR;
    }
    
    // Transform hostname into a structure with network details
    hostData = gethostbyname(hostName);
    
    if(hostData == NULL){
        printf("Error finding IP\n");
        return SOCKET_ERROR;
    }

    // Extract the list of IPs (handles multiple interfaces like Wi-Fi/Ethernet)
    addr_list = (struct in_addr **)hostData->h_addr_list;
            
    for(int i = 0; addr_list[i] != NULL; i++) {
        // Convert binary IP to string format for display
        printf("Local IP: %s\n", inet_ntoa(*addr_list[i]));
    }

    SOCKET s = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in server_addr; 
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080);
    server_addr.sin_addr = *addr_list[0];

    if(connect(s, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0){
        printf("Connection error\n");
        return SOCKET_ERROR;
    }

    printf("CONNECTED TO SERVER\n");

    CLIENT* cl = initialize_client(s, 0, "MY_MAGIC_KEY");

    HANDLE comm_thr[2];
    comm_thr[0] = CreateThread(NULL, 0, &receiveMessage, cl, 0, NULL);
    comm_thr[1] = CreateThread(NULL, 0, &sendMessage, cl, 0, NULL);
    
    WaitForMultipleObjects(2, comm_thr, TRUE, INFINITE);
    
    CloseHandle(comm_thr[0]);
    CloseHandle(comm_thr[1]);

    release_client(cl);
    WSACleanup();
    DeleteCriticalSection(&CS);
    return 0;
}

DWORD WINAPI receiveMessage(LPVOID M){
    CLIENT* S = M;
    char bufferIN[1024];
    int bytesReceived;

    while(1){
        memset(bufferIN, 0, sizeof(bufferIN));

        // Read data sent by the server into the 'bufferIN' array
        bytesReceived = recv(S->comm_channel, bufferIN, sizeof(bufferIN)-1, 0);

        EnterCriticalSection(&CS);
        cipher_buffer(S, bufferIN, bytesReceived);
        LeaveCriticalSection(&CS);

        if (bytesReceived > 0){
            printf("%s\n", bufferIN);
        }
        else{
            printf("Connection closed by server.\n");
            break;
        }
    }
    return 0;
}

DWORD WINAPI sendMessage(LPVOID M){
    CLIENT *S = M;
    char bufferOUT[1024];
    int bytesSent;

    while(1){
        memset(bufferOUT, 0, sizeof(bufferOUT));

        fgets(bufferOUT, sizeof(bufferOUT), stdin);
        int size_buff = strlen(bufferOUT);

        EnterCriticalSection(&CS);
        cipher_buffer(S, bufferOUT, size_buff);
        LeaveCriticalSection(&CS);
        
        bytesSent = send(S->comm_channel, bufferOUT, size_buff, 0);
    }
    return 0;
}