#include <winsock2.h>
#include <windows.h>
#include <stdio.h>

DWORD WINAPI receiveMessage(LPVOID S);
DWORD WINAPI sendMessage(LPVOID S);

int main() {
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);

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
    
    // Defina uma porta fixa
    server_addr.sin_port = htons(8080);

    // Uso do IP da lista para o campo de endereço
    // Aqui pegamos o primeiro IP da sua lista local
    server_addr.sin_addr = *addr_list[0];

    if(connect(s, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0){
        printf("Erro ao conectar\n");
        return SOCKET_ERROR;
    }

    printf("CONECTADO AO SERVIDOR\n");

    HANDLE comm_thr[2];
    comm_thr[0] = CreateThread(NULL, 0, &receiveMessage, &s, 0, NULL);
    comm_thr[1] = CreateThread(NULL, 0, &sendMessage, &s, 0, NULL);
    
    WaitForMultipleObjects(2, comm_thr, TRUE, INFINITE);
    
    CloseHandle(comm_thr[0]);
    CloseHandle(comm_thr[1]);

    closesocket(s);
    WSACleanup();
    return 0;
    
}


DWORD WINAPI receiveMessage(LPVOID M){
    SOCKET S = *(SOCKET*)M;

    char bufferIN[1024];
    int bytesRecebidos;

    while(1){

        memset(bufferIN, 0, sizeof(bufferIN));

        // recv(): Lê os dados enviados pelo cliente e guarda no array 'buffer'.
        bytesRecebidos = recv(S, bufferIN, sizeof(bufferIN)-1, 0);

        if (bytesRecebidos > 0) 
            printf("%s\n", bufferIN);
        
    }

    return 0;
}

DWORD WINAPI sendMessage(LPVOID M){
    SOCKET S = *(SOCKET*)M;

    char bufferOUT[1024];
    int bytesEnviados;
    while(1){
        memset(bufferOUT, 0, sizeof(bufferOUT));

        fgets(bufferOUT, sizeof(bufferOUT), stdin);
        
        bytesEnviados = send(S, bufferOUT, strlen(bufferOUT), 0);

    }

    return 0;
}