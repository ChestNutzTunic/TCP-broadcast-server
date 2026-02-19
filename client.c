#include "crypto.h"
#include "dyn_arr.h"

DWORD WINAPI receiveMessage(LPVOID S);
DWORD WINAPI sendMessage(LPVOID S);

CRITICAL_SECTION CS;

int main() {
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);

    InitializeCriticalSection(&CS);

    char nomeHost[100];
    struct hostent* hostData;
    struct in_addr** addr_list;

    if(gethostname(nomeHost, sizeof(nomeHost)) == SOCKET_ERROR){
        printf("erro ao encontrar host\n");
        return SOCKET_ERROR;
    }
    // 2. Transforma o nome em uma estrutura cheia de detalhes de rede
    hostData = gethostbyname(nomeHost);
    
    int ip;

    if(hostData == NULL){
        printf("erro ao encontrar IP\n");
        return SOCKET_ERROR;
    }

    // 3. Extrai a lista de IPs (pode haver mais de um, como Wi-Fi e Ethernet)
    addr_list = (struct in_addr **)hostData->h_addr_list;
            
    for(int i = 0; addr_list[i] != NULL; i++) {
        // 4. Converte o IP de binário para texto e imprime
        printf("IP Local: %s\n", inet_ntoa(*addr_list[i]));
    }

    SOCKET s = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in server_addr; 
    server_addr.sin_family = AF_INET;
    
    // Defina uma porta fixa para conectar
    server_addr.sin_port = htons(8080);

    // Uso do IP da lista para o campo de endereço
    // Aqui pegamos o primeiro IP da sua lista local
    server_addr.sin_addr = *addr_list[0];

    if(connect(s, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0){
        printf("Erro ao conectar\n");
        return SOCKET_ERROR;
    }

    printf("CONECTADO AO SERVIDOR\n");

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
    int bytesRecebidos;

    while(1){

        memset(bufferIN, 0, sizeof(bufferIN));

        // recv(): Lê os dados enviados pelo cliente e guarda no array 'buffer'.
        bytesRecebidos = recv(S->comm_channel, bufferIN, sizeof(bufferIN)-1, 0);

        EnterCriticalSection(&CS);
        cipher_buffer(S, bufferIN, bytesRecebidos);
        LeaveCriticalSection(&CS);

        if (bytesRecebidos > 0){
            printf("%s\n", bufferIN);
        }
        else{
            printf("Conexão encerrada pelo servidor.\n");
            break;
        }
    }

    return 0;
}

DWORD WINAPI sendMessage(LPVOID M){
    CLIENT *S = M;

    char bufferOUT[1024];
    int bytesEnviados;
    while(1){
        memset(bufferOUT, 0, sizeof(bufferOUT));

        fgets(bufferOUT, sizeof(bufferOUT), stdin);
        int size_buff = strlen(bufferOUT);

        EnterCriticalSection(&CS);
        cipher_buffer(S, bufferOUT, size_buff);
        LeaveCriticalSection(&CS);
        
        bytesEnviados = send(S->comm_channel, bufferOUT, size_buff, 0);

    }

    return 0;
}