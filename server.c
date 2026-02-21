// 0x0600 HEXADECIMAL CODE FOR WINDOWS VISTA VERSION
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0600
#endif
// Necessary for SRWLock usage

#include "dyn_arr.h"
#include "crypto.h"
#include <stdbool.h>

#define client_list_size 5

// Function prototype: Thread routine to handle individual client conversations
DWORD WINAPI startClientConversation(LPVOID client_comm_channel);

SRWLOCK LOCK;
DYN_CLIENT_ARRAY* CONN_A = NULL;
SYSTEM_INFO SYS_INFO;

int main() {
    // Structure to hold Windows Socket API implementation details
    WSADATA wsa;
    InitializeSRWLock(&LOCK);

    // Retrieve hardware info (used for CPU affinity mapping)
    GetSystemInfo(&SYS_INFO);
    
    // Initialize Winsock - Requesting version 2.2
    if (WSAStartup(MAKEWORD(2,2), &wsa) != 0) {
        return 1; 
    }

    // Initialize the Dynamic Client Array
    CONN_A = init_DSA();
    int client_total_count = 0;

    // AF_INET: IPv4 address family
    // SOCK_STREAM: TCP protocol (ensures reliable, ordered data delivery)
    SOCKET s = socket(AF_INET, SOCK_STREAM, 0);
    if (s == INVALID_SOCKET) {
        printf("Error creating socket\n");
        return 1;
    }

    struct sockaddr_in server; // Structure for IP and Port configuration
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY; // Listen on all available network interfaces
    server.sin_port = htons(8080);       // Convert port to Network Byte Order (Big Endian)

    // Associate the socket with the local address and port
    bind(s, (struct sockaddr *)&server, sizeof(server));

    // Put the socket in listening mode
    // client_list_size defines the maximum length of the pending connections queue
    listen(s, client_list_size);
    printf("Waiting for connections...\n");

    while(1){
        struct sockaddr_in client_addr;
        int addr_len = sizeof(client_addr);

        // Accept incoming connection request
        SOCKET new_comm = accept(s, (struct sockaddr*)&client_addr, &addr_len);

        if(new_comm != INVALID_SOCKET){
            // Initialize a new CLIENT object with a unique ID and shared secret key
            CLIENT* cl = initialize_client(new_comm, client_total_count++, "MY_MAGIC_KEY");
            
            // Critical Section: Thread-safe addition to the connection array
            AcquireSRWLockExclusive(&LOCK);
            add_to_DSA(CONN_A, cl);
            ReleaseSRWLockExclusive(&LOCK);

            // Spawn a dedicated thread for the new client
            HANDLE thr = CreateThread(NULL, 0, &startClientConversation, cl, 0, NULL);
            CloseHandle(thr); // Handle is closed but the thread continues to run
        }
    }

    // Resource cleanup
    AcquireSRWLockExclusive(&LOCK);
    free_DSA(CONN_A);
    ReleaseSRWLockExclusive(&LOCK);

    closesocket(s);
    WSACleanup();

    return 0;
}

DWORD WINAPI startClientConversation(LPVOID client_comm_channel){

    // Cast the parameter back to our CLIENT structure
    CLIENT* client_info = client_comm_channel;

    // CPU Affinity Logic: Map the client to a specific processor core based on ID
    DWORD core_num = SYS_INFO.dwNumberOfProcessors;
    
    DWORD core_id = (1 << (client_info->client_id % core_num));

    SetThreadAffinityMask(GetCurrentThread(), core_id);

    char bufferIN[1024];
    char bufferOUT[1024];
    int bytesRecebidos;

    // Notify all active clients about the new user entry
    snprintf(bufferIN, sizeof(bufferIN), "User %d joined the chat", client_info->client_id);

    AcquireSRWLockShared(&LOCK);
    for(int i=0; i<sizeof_DSA(CONN_A); i++){
        CLIENT* S = get_elem_DSA(CONN_A, i);

        if(S!=NULL && S->comm_channel != INVALID_SOCKET && S != client_info){
            char buff[1024];
            int size_buff = strlen(bufferIN);
            strcpy(buff, bufferIN);
            
            // Encrypt notification for each specific client
            cipher_buffer(S, buff, size_buff);
            send(S->comm_channel, buff, size_buff, 0);
        }
    }
    ReleaseSRWLockShared(&LOCK);

    // Main communication loop
    while(1){
        memset(bufferIN, 0, sizeof(bufferIN));

        // Read data sent by the client
        bytesRecebidos = recv(client_info->comm_channel, bufferIN, sizeof(bufferIN)-1, 0);

        if(bytesRecebidos > 0){

            // Decrypt incoming message
            cipher_buffer(client_info, bufferIN, bytesRecebidos);

            // Handle exit command
            if (strncmp(bufferIN, "exit", 4) == 0){
                snprintf(bufferOUT, sizeof(bufferOUT), "User %d has requested to leave.\n", client_info->client_id);
                int size_buff = strlen(bufferOUT);

                AcquireSRWLockShared(&LOCK);
                for(int i=0; i<sizeof_DSA(CONN_A); i++){
                    CLIENT* S = get_elem_DSA(CONN_A, i);

                    if(S!=NULL && S->comm_channel != INVALID_SOCKET && S != client_info){
                        char buff[1024];
                        memcpy(buff, bufferOUT, size_buff);
                        cipher_buffer(S, buff, size_buff);
                        send(S->comm_channel, buff, size_buff, 0);
                    }
                }
                ReleaseSRWLockShared(&LOCK);

                break;
            }
            
            // Broadcast message to all other users
            snprintf(bufferOUT, sizeof(bufferOUT), "User %d says:\n %s\n", client_info->client_id, bufferIN);
            int size_buff = strlen(bufferOUT);
            
            AcquireSRWLockShared(&LOCK);
            for(int i=0; i<sizeof_DSA(CONN_A); i++){
                CLIENT* S = get_elem_DSA(CONN_A, i);

                if(S!=NULL && S->comm_channel != INVALID_SOCKET && S != client_info){
                    char buff[1024];
                    memcpy(buff, bufferOUT, size_buff);
                    cipher_buffer(S, buff, size_buff);
                    send(S->comm_channel, buff, size_buff, 0);
                }
            }
            ReleaseSRWLockShared(&LOCK);
        }
        else if(bytesRecebidos == 0){
            printf("User %d disconnected", client_info->client_id);
            break;
        }
        else{
            printf("Connection error: %d\n", WSAGetLastError());
            break;
        }
    }
    
    // Clean up client record upon disconnection
    AcquireSRWLockExclusive(&LOCK);
    remove_of_DSA(CONN_A, client_info);
    ReleaseSRWLockExclusive(&LOCK);

    return 0;
}