// 0x0600 HEXADECIMAL CODE FOR WINDOWS VISTA VERSION
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0600
#endif
// Necessary for SRWLock usage

#include "dyn_arr.h"
#include <stdbool.h>
#include <Psapi.h> // process status API = necessary for server dashboard

#define MAX_THREADS 10

DWORD WINAPI processClientConversation(LPVOID client_comm_channel);
DWORD WINAPI update_server_dashboard(LPVOID lpParam);

SRWLOCK LOCK;
DYN_CLIENT_ARRAY* CONN_A = NULL;
SYSTEM_INFO SYS_INFO;

volatile long client_count;

int main() {

    client_count = 0;

    arena_header = Arena_Init(sizeof(COM_PORT_INFO));    

    WSADATA wsa;

    if (WSAStartup(MAKEWORD(2,2), &wsa) != 0) {
        return 1; 
    }

    InitializeSRWLock(&LOCK);

    GetSystemInfo(&SYS_INFO);

    CONN_A = init_DCA();
    u32 client_total_count = 0;

    SOCKET s = socket(AF_INET, SOCK_STREAM, 0);
    if (s == INVALID_SOCKET) {
        printf("Error creating socket\n");
        return 1;
    }

    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(8080);

    bind(s, (struct sockaddr *)&server, sizeof(server));

    listen(s, 1000);
    printf("Waiting for connections...\n");

    // CREATE PORT FOR IOCP
    HANDLE hPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);

    for(u8 i=0; i<MAX_THREADS; i++){
        HANDLE thread = CreateThread(NULL, 0, processClientConversation, hPort, 0, NULL);

        // You must use a bitmap to set a thread to a specific CPU core
        DWORD core_num = SYS_INFO.dwNumberOfProcessors;

        DWORD core_id = (1 << (i % core_num));

        SetThreadAffinityMask(thread, core_id);
    }

    CreateThread(NULL, 0, update_server_dashboard, NULL, 0, NULL);

    while(1){
        struct sockaddr_in client_addr;
        u32 addr_len = sizeof(client_addr);

        // Accept incoming connection request
        SOCKET new_comm = accept(s, (struct sockaddr*)&client_addr, &addr_len);

        if(new_comm != INVALID_SOCKET){

            CreateIoCompletionPort((HANDLE)new_comm, hPort, (ULONG_PTR)new_comm, 0);

            // Initialize a new CLIENT object with a unique ID and shared secret key
            // the client object is in the DCA, allocated, the arena will only hold it's pointer
            CLIENT* cl = initialize_client(new_comm, client_total_count++, "MY_MAGIC_KEY");
            
            // Critical Section: Thread-safe addition to the connection array
            AcquireSRWLockExclusive(&LOCK);
            add_to_DCA(CONN_A, cl);
            ReleaseSRWLockExclusive(&LOCK);

            InterlockedIncrement(&client_count);

            COM_PORT_INFO* PORT_INFO = Arena_Pop(arena_header);

            memset(&(PORT_INFO->overlapped), 0, sizeof(OVERLAPPED));

            PORT_INFO->client = cl;
            PORT_INFO->operation_info = 0;
            PORT_INFO->wsabuf.buf = PORT_INFO->buffer;
            PORT_INFO->wsabuf.len = MAX_BUFFER_SIZE;

            DWORD flags = 0;
            WSARecv(new_comm, &(PORT_INFO->wsabuf), 1, NULL, &flags, &(PORT_INFO->overlapped), NULL);
        }
    }

    // Resource cleanup
    AcquireSRWLockExclusive(&LOCK);
    free_DCA(CONN_A);
    ReleaseSRWLockExclusive(&LOCK);

    ReleaseSRWLockShared(&LOCK);

    closesocket(s);
    WSACleanup();

    return 0;
}

// THIS FUNCTION WILL ONLY PROCESS INFORMATION AND DISCLOSE IT TO OTHER CLIENTS
DWORD WINAPI processClientConversation(LPVOID completion_port){

    HANDLE hPort = completion_port;
    DWORD bytesTransferred;
    ULONG_PTR completionKey;
    LPOVERLAPPED lpOverlapped;

    COM_PORT_INFO* client_info;

    while(TRUE){
        bool result = GetQueuedCompletionStatus(hPort, &bytesTransferred, &completionKey, &lpOverlapped, INFINITE);

        if (lpOverlapped == NULL) continue;

        client_info = (COM_PORT_INFO*)lpOverlapped;

        // SWITCH CASE TO ORGANIZE WSASend(1) and WSARecv(0)
        switch(client_info->operation_info){

            case 0: // WRITE OPERATION
            
                if(result && bytesTransferred > 0){

                    // Decrypt incoming message
                    EnterCriticalSection(&(client_info->client->CS_lock));

                    InterlockedIncrement(&(client_info->client->ref_counting));

                    cipher_buffer(client_info->client, client_info->wsabuf.buf, bytesTransferred);
                    LeaveCriticalSection(&(client_info->client->CS_lock));

                    if(bytesTransferred < MAX_BUFFER_SIZE){
                        // \0 ISN'T TRANSPORTED THROUGH SOCKETS WITH 'SEND', SO IT'S NECESSARY TO DICTATE WHERE THE STRING ENDS
                        client_info->wsabuf.buf[bytesTransferred] = '\0';
                    }
                    else{
                        client_info->wsabuf.buf[bytesTransferred-1] = '\0';
                    }

                    char bufferOUT[MAX_BUFFER_SIZE];

                    // Handle exit command
                    if (strncmp(client_info->wsabuf.buf, "exit", 4) == 0){
                        snprintf(bufferOUT, sizeof(bufferOUT), "User %d has requested to leave.\n", client_info->client->client_id);
                        u64 size_buff = strlen(bufferOUT);

                        AcquireSRWLockShared(&LOCK);
                        
                        broadcast_to_DCA(client_info, CONN_A, bufferOUT, size_buff);

                        ReleaseSRWLockShared(&LOCK);


                        // LIBERATE FROM DYNAMIC ARRAY (ONLY CLOSES SOCKET)
                        AcquireSRWLockExclusive(&LOCK);
                        remove_of_DCA(CONN_A, client_info->client);
                        ReleaseSRWLockExclusive(&LOCK);
                        InterlockedDecrement(&client_count);

                        // Since this if statement handles the exit of clients, it's necessary to decrement the reference counting
                        if(InterlockedDecrement(&(client_info->client->ref_counting)) == 0){
                            DeleteCriticalSection(&(client_info->client->CS_lock));
                            free(client_info->client);
                        }

                        Arena_Push(arena_header, (void*)client_info);

                        continue;
                        
                    }
                    else{

                        // Broadcast message to all other users
                        snprintf(bufferOUT, sizeof(bufferOUT), "User %d says:\n %s\n", client_info->client->client_id, client_info->wsabuf.buf);
                        u64 size_buff = strlen(bufferOUT);
                    
                        AcquireSRWLockShared(&LOCK);

                        broadcast_to_DCA(client_info, CONN_A, bufferOUT, size_buff);

                        ReleaseSRWLockShared(&LOCK);

                    }

                }
                else{

                    AcquireSRWLockExclusive(&LOCK);
                    remove_of_DCA(CONN_A, client_info->client);
                    ReleaseSRWLockExclusive(&LOCK);
                    InterlockedDecrement(&client_count);

                    if(InterlockedDecrement(&client_info->client->ref_counting) == 0){
                        DeleteCriticalSection(&(client_info->client->CS_lock));
                        free(client_info->client);
                    }
                    
                    Arena_Push(arena_header, (void*)client_info);
                    continue;

                }

                memset(&client_info->overlapped, 0, sizeof(client_info->overlapped));

                DWORD flags = 0;
                WSARecv(client_info->client->comm_channel, &(client_info->wsabuf), 1, NULL, &flags, &(client_info->overlapped), NULL);
            
                break;

            case 1: // WRITE_DONE OPERATION

                // save client_info->client at this very moment to free later if necessary
                // even though we free the client before giving the block to the arena, it's better to save the reference for future use
                CLIENT* ref = client_info->client;

                //  IF REFERENCE_COUNTING REACHES 0, THEN ALL PENDING WSASends HAVE COMPLETED
                if(InterlockedDecrement(&(ref->ref_counting)) == 0){
                    DeleteCriticalSection(&(ref->CS_lock));
                    free(ref);
                }


                // SINCE IT'S A WRITE_DONE OPERATION, CLIENT_INFO IS JUST THE WRITE CONTEXT
                Arena_Push(arena_header, (void*)client_info);

                break;

        }
    }

    return 0;
}

DWORD WINAPI update_server_dashboard(LPVOID lpParam){
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    system("cls");
    while(TRUE){
        PROCESS_MEMORY_COUNTERS_EX PR_MEM_C;
        GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&PR_MEM_C, sizeof(PR_MEM_C));

        // Move o cursor para o topo em vez de limpar tudo
        COORD cursorPosition = {0, 0};
        SetConsoleCursorPosition(hConsole, cursorPosition);

        printf("====================================================\n");
        printf("                  SERVER DASHBOARD                  \n");
        printf("----------------------------------------------------\n");
        printf(" Connected Clients: %-10ld                     \n", client_count);
        printf(" Memory Allocated:  %-10lu MB                  \n", PR_MEM_C.PrivateUsage / (1024*1024));
        printf("====================================================\n");

        Sleep(100);
    }
    return 0;
}