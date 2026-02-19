// 0X0600 HEXADECIMAL CODE FOR WINDOWS VISTA VERSION
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0600
#endif
// NECESSARY FOR SRWLOCK USAGE

#include "dyn_arr.h"
#include "crypto.h"
#include <stdbool.h>

#define client_list_size 5

// PASSE O OBJETO "CLIENT" E CRIE UMA NOVA CONVERSA
DWORD WINAPI startClientConversation(LPVOID client_comm_channel);

SRWLOCK LOCK;
DYN_CLIENT_ARRAY* CONN_A = NULL;
SYSTEM_INFO SYS_INFO;

int main() {
    // Estrutura que guardará detalhes da implementação de rede do Windows
    // WINDOWS SOCKET API
    WSADATA wsa;
    InitializeSRWLock(&LOCK);

    GetSystemInfo(&SYS_INFO);
    
    // WSAStartup: WINDOWS SOCKET API
    // MAKEWORD(2,2) solicita a versão 2.2 do Winsock.
    if (WSAStartup(MAKEWORD(2,2), &wsa) != 0) {
        return 1; 
    }

    CONN_A = init_DSA();
    int client_total_count = 0;

    // AF_INET(ADDRESS FAMILY _INTERNET): Usa protocolo IPv4.
    // SOCK_STREAM: Usa o protocolo TCP (garante que os dados chegam inteiros).
    SOCKET s = socket(AF_INET, SOCK_STREAM, 0);
    if (s == INVALID_SOCKET) {
        printf("Erro ao criar socket\n");
        return 1;
    }

    struct sockaddr_in server; // Estrutura que guarda IP e Porta
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY; // Aceita conexões de qualquer placa de rede do PC
    server.sin_port = htons(8080);       // htons: Converte o número 8080 para "Network Byte Order"

    bind(s, (struct sockaddr *)&server, sizeof(server));

    // listen(): Coloca o socket em modo de espera. 
    // O número "client_size" é o tamanho da fila de pessoas esperando para serem atendidas.
    listen(s, client_list_size);
    printf("Aguardando conexoes...\n");

    while(1){
        struct sockaddr_in client_addr;
        int addr_len = sizeof(client_addr);

        SOCKET new_comm = accept(s, (struct sockaddr*)&client_addr, &addr_len);

        if(new_comm != INVALID_SOCKET){
            CLIENT* cl = initialize_client(new_comm, client_total_count++, "MY_MAGIC_KEY");
            
            AcquireSRWLockExclusive(&LOCK);
            add_to_DSA(CONN_A, cl);
            ReleaseSRWLockExclusive(&LOCK);

            HANDLE thr = CreateThread(NULL, 0, &startClientConversation, cl, 0, NULL);
            CloseHandle(thr);
        }
    }

    AcquireSRWLockExclusive(&LOCK);
    free_DSA(CONN_A);
    ReleaseSRWLockExclusive(&LOCK);

    closesocket(s);
    WSACleanup();

    return 0;
}

DWORD WINAPI startClientConversation(LPVOID client_comm_channel){

    // CLIENT_INFO = socket do servidor e id do cliente
    CLIENT* client_info = client_comm_channel;

    DWORD core_num = SYS_INFO.dwNumberOfProcessors;
    DWORD core_id = (1 << (client_info->client_id % core_num));

    SetThreadAffinityMask(GetCurrentThread(), core_id);

    char bufferIN[1024];
    char bufferOUT[1024];
    int bytesRecebidos;

    snprintf(bufferIN, sizeof(bufferIN), "Usuario %d entrou no chat", client_info->client_id);

    AcquireSRWLockShared(&LOCK);
    for(int i=0; i<sizeof_DSA(CONN_A); i++){
        CLIENT* S = get_elem_DSA(CONN_A, i);

        if(S!=NULL && S->comm_channel != INVALID_SOCKET){
            char buff[1024];
            int size_buff = strlen(bufferIN);
            strcpy(buff, bufferIN);
            cipher_buffer(S, buff, size_buff);
            send(S->comm_channel, buff, size_buff, 0);
        }
    }
    ReleaseSRWLockShared(&LOCK);

    while(1){

        memset(bufferIN, 0, sizeof(bufferIN));

        // Le os dados recebidos
        bytesRecebidos = recv(client_info->comm_channel, bufferIN, sizeof(bufferIN)-1, 0);

        if(bytesRecebidos > 0){

            // DECIPHER
            cipher_buffer(client_info, bufferIN, bytesRecebidos);

            // Se digitar "sair", quebra-se o loop
            if (strncmp(bufferIN, "sair", 4) == 0){
                snprintf(bufferOUT, sizeof(bufferOUT), "O amigo %d solicitou o encerramento.\n", client_info->client_id);
                int size_buff = strlen(bufferOUT);

                AcquireSRWLockShared(&LOCK);
                for(int i=0; i<sizeof_DSA(CONN_A); i++){
                CLIENT* S = get_elem_DSA(CONN_A, i);

                    if(S!=NULL && S->comm_channel != INVALID_SOCKET){
                        char buff[1024];
                        memcpy(buff, bufferOUT, size_buff);
                        cipher_buffer(S, buff, size_buff);
                        send(S->comm_channel, buff, size_buff, 0);
                    }
                }
                ReleaseSRWLockShared(&LOCK);

                break;
            }
            
            snprintf(bufferOUT, sizeof(bufferOUT), "Amigo %d diz:\n %s\n", client_info->client_id, bufferIN);
            int size_buff = strlen(bufferOUT);
            
            AcquireSRWLockShared(&LOCK);
            for(int i=0; i<sizeof_DSA(CONN_A); i++){
                CLIENT* S = get_elem_DSA(CONN_A, i);

                if(S!=NULL && S->comm_channel != INVALID_SOCKET){
                    char buff[1024];
                    memcpy(buff, bufferOUT, size_buff);
                    cipher_buffer(S, buff, size_buff);
                    send(S->comm_channel, buff, size_buff, 0);
                }

            }
            ReleaseSRWLockShared(&LOCK);
        }
        else if(bytesRecebidos == 0){
            printf("Amigo %d desconectou-se", client_info->client_id);
        }
        else{
            printf("Erro na conexao: %d\n", WSAGetLastError());
            break;
        }
    }
    
    AcquireSRWLockExclusive(&LOCK);
    remove_of_DSA(CONN_A, client_info);
    ReleaseSRWLockExclusive(&LOCK);

    return 0;
}