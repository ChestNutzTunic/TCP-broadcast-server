// stress_test_auto_ip.c
// Versão que descobre o IP do servidor automaticamente igual ao seu cliente

#include <winsock2.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <conio.h>

#pragma comment(lib, "ws2_32.lib")

#define DEFAULT_BUFFER_SIZE 1024
#define DEFAULT_PORT 8080
#define MAX_MESSAGE_SIZE 256

typedef struct {
    int client_id;
    int port;
    int messages_to_send;
    int messages_sent;
    int messages_received;
    HANDLE completion_event;
} CLIENT_THREAD_DATA;

typedef struct {
    CRITICAL_SECTION cs;
    LONG total_connections;
    LONG successful_connections;
    LONG failed_connections;
    LONG total_messages_sent;
    LONG total_messages_received;
    LONG active_connections;
    LONG peak_connections;
    DWORD start_time;
    DWORD end_time;
    char server_ip[16];  // IP descoberto
} STATISTICS;

STATISTICS g_stats = {0};
HANDLE g_console_handle;
int g_running = 1;

// Função para descobrir o IP local (igual ao seu cliente)
int discover_server_ip(char* server_ip) {
    char hostName[100];
    struct hostent* hostData;
    struct in_addr** addr_list;
    
    if (gethostname(hostName, sizeof(hostName)) == SOCKET_ERROR) {
        printf("Error finding host\n");
        return 0;
    }
    
    hostData = gethostbyname(hostName);
    if (hostData == NULL) {
        printf("Error finding IP\n");
        return 0;
    }
    
    addr_list = (struct in_addr**)hostData->h_addr_list;
    
    printf("\nAvailable network interfaces:\n");
    for (int i = 0; addr_list[i] != NULL; i++) {
        char* ip = inet_ntoa(*addr_list[i]);
        printf("  [%d] %s\n", i, ip);
        
        // Pega o primeiro IP não-loopback (não é 127.0.0.1)
        if (strcmp(ip, "127.0.0.1") != 0 && i == 0) {
            strcpy(server_ip, ip);
        }
    }
    
    // Se não encontrou IP não-loopback, usa o primeiro
    if (strlen(server_ip) == 0 && addr_list[0] != NULL) {
        strcpy(server_ip, inet_ntoa(*addr_list[0]));
    }
    
    return (addr_list[0] != NULL);
}

// Função para gerar mensagens aleatórias
void generate_random_message(char* buffer, int size, int client_id, int msg_id) {
    snprintf(buffer, size, "Client %d - Message %d - %d", 
             client_id, msg_id, rand() % 10000);
}

// Client thread function
DWORD WINAPI client_thread(LPVOID param) {
    CLIENT_THREAD_DATA* data = (CLIENT_THREAD_DATA*)param;
    SOCKET sock;
    struct sockaddr_in server_addr;
    char send_buffer[MAX_MESSAGE_SIZE];
    char recv_buffer[DEFAULT_BUFFER_SIZE];
    int bytes_received;
    DWORD thread_start = GetTickCount();
    
    // Criar socket
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) {
        printf("[Client %d] Failed to create socket\n", data->client_id);
        InterlockedIncrement(&g_stats.failed_connections);
        SetEvent(data->completion_event);
        return 1;
    }
    
    // Configurar servidor
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(data->port);
    server_addr.sin_addr.s_addr = inet_addr(g_stats.server_ip);
    
    printf("[Client %d] Connecting to %s:%d...\n", 
           data->client_id, g_stats.server_ip, data->port);
    
    // Conectar
    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        int error = WSAGetLastError();
        printf("[Client %d] Connection failed: %d\n", data->client_id, error);
        InterlockedIncrement(&g_stats.failed_connections);
        closesocket(sock);
        SetEvent(data->completion_event);
        return 1;
    }
    
    // Sucesso na conexão
    InterlockedIncrement(&g_stats.successful_connections);
    InterlockedIncrement(&g_stats.active_connections);
    
    // Atualizar pico de conexões
    LONG current_active = g_stats.active_connections;
    LONG old_peak;
    do {
        old_peak = g_stats.peak_connections;
        if (current_active <= old_peak) break;
    } while (InterlockedCompareExchange(&g_stats.peak_connections, current_active, old_peak) != old_peak);
    
    printf("[Client %d] Connected successfully! Sending %d messages...\n", 
           data->client_id, data->messages_to_send);
    
    // Loop de mensagens
    for (int msg = 0; msg < data->messages_to_send && g_running; msg++) {
        // Gerar e enviar mensagem
        generate_random_message(send_buffer, sizeof(send_buffer), data->client_id, msg);
        
        if (send(sock, send_buffer, (int)strlen(send_buffer), 0) > 0) {
            data->messages_sent++;
            InterlockedIncrement(&g_stats.total_messages_sent);
            
            // Tentar receber resposta
            bytes_received = recv(sock, recv_buffer, sizeof(recv_buffer) - 1, 0);
            if (bytes_received > 0) {
                recv_buffer[bytes_received] = '\0';
                data->messages_received++;
                InterlockedIncrement(&g_stats.total_messages_received);
            }
            
            // Pequeno delay entre mensagens
            Sleep(rand() % 100 + 20);
        }
    }
    
    // Enviar comando de saída
    send(sock, "exit", 4, 0);
    Sleep(100);
    
    // Cleanup
    closesocket(sock);
    InterlockedDecrement(&g_stats.active_connections);
    
    printf("[Client %d] Completed: Sent=%d, Received=%d, Time=%dms\n", 
           data->client_id, data->messages_sent, data->messages_received,
           GetTickCount() - thread_start);
    
    SetEvent(data->completion_event);
    return 0;
}

// Statistics display thread
DWORD WINAPI stats_thread(LPVOID param) {
    int num_clients = *(int*)param;
    DWORD last_update = GetTickCount();
    
    while (g_running) {
        if (GetTickCount() - last_update >= 1000) {
            last_update = GetTickCount();
            
            DWORD elapsed = (GetTickCount() - g_stats.start_time) / 1000;
            if (elapsed == 0) elapsed = 1;
            
            COORD pos = {0, 0};
            SetConsoleCursorPosition(g_console_handle, pos);
            
            printf("============================================================\n");
            printf("              STRESS TEST - CONNECTED TO %s:%d\n", 
                   g_stats.server_ip, DEFAULT_PORT);
            printf("============================================================\n");
            printf("Time: %ld seconds\n", elapsed);
            printf("------------------------------------------------------------\n");
            printf("CONNECTIONS:\n");
            printf("  Active: %ld | Peak: %ld\n", 
                   g_stats.active_connections, g_stats.peak_connections);
            printf("  Successful: %ld | Failed: %ld\n", 
                   g_stats.successful_connections, g_stats.failed_connections);
            printf("------------------------------------------------------------\n");
            printf("MESSAGES:\n");
            printf("  Sent: %ld | Received: %ld\n", 
                   g_stats.total_messages_sent, g_stats.total_messages_received);
            printf("  Msgs/sec: %.1f\n", 
                   (float)g_stats.total_messages_sent / elapsed);
            printf("------------------------------------------------------------\n");
            printf("Progress: %d/%d clients completed\n", 
                   (int)(g_stats.successful_connections + g_stats.failed_connections), 
                   num_clients);
            printf("Press 'q' to stop\n");
            printf("============================================================\n");
            
            if (_kbhit() && _getch() == 'q') {
                g_running = 0;
            }
        }
        Sleep(100);
    }
    return 0;
}

int main(int argc, char* argv[]) {
    WSADATA wsa;
    int port = DEFAULT_PORT;
    int num_clients = 5;
    int messages_per_client = 5;
    
    // Parse command line arguments
    if (argc >= 2) num_clients = atoi(argv[1]);
    if (argc >= 3) messages_per_client = atoi(argv[2]);
    if (argc >= 4) port = atoi(argv[3]);
    
    // Initialize Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        printf("WSAStartup failed\n");
        return 1;
    }
    
    // Discover server IP (igual ao seu cliente)
    memset(g_stats.server_ip, 0, sizeof(g_stats.server_ip));
    if (!discover_server_ip(g_stats.server_ip)) {
        printf("Failed to discover server IP\n");
        WSACleanup();
        return 1;
    }
    
    printf("\n============================================================\n");
    printf("Server IP detected: %s\n", g_stats.server_ip);
    printf("Testing with: %d clients, %d messages each\n", 
           num_clients, messages_per_client);
    printf("============================================================\n\n");
    
    // Initialize random seed
    srand((unsigned int)time(NULL));
    
    // Get console handle
    g_console_handle = GetStdHandle(STD_OUTPUT_HANDLE);
    
    // Initialize statistics
    InitializeCriticalSection(&g_stats.cs);
    g_stats.start_time = GetTickCount();
    
    // Hide cursor
    CONSOLE_CURSOR_INFO cursorInfo;
    GetConsoleCursorInfo(g_console_handle, &cursorInfo);
    cursorInfo.bVisible = FALSE;
    SetConsoleCursorInfo(g_console_handle, &cursorInfo);
    
    // Start stats thread
    HANDLE hStats = CreateThread(NULL, 0, stats_thread, &num_clients, 0, NULL);
    
    // Create client threads
    HANDLE* threads = (HANDLE*)malloc(num_clients * sizeof(HANDLE));
    CLIENT_THREAD_DATA* thread_data = (CLIENT_THREAD_DATA*)malloc(num_clients * sizeof(CLIENT_THREAD_DATA));
    HANDLE* events = (HANDLE*)malloc(num_clients * sizeof(HANDLE));
    
    printf("Starting clients...\n");
    
    for (int i = 0; i < num_clients; i++) {
        thread_data[i].client_id = i;
        thread_data[i].port = port;
        thread_data[i].messages_to_send = messages_per_client;
        thread_data[i].messages_sent = 0;
        thread_data[i].messages_received = 0;
        thread_data[i].completion_event = CreateEvent(NULL, FALSE, FALSE, NULL);
        events[i] = thread_data[i].completion_event;
        
        InterlockedIncrement(&g_stats.total_connections);
        
        threads[i] = CreateThread(NULL, 0, client_thread, &thread_data[i], 0, NULL);
        
        if (threads[i] == NULL) {
            printf("Failed to create thread for client %d\n", i);
        }
        
        // Pequeno delay entre conexões para não sobrecarregar
        Sleep(50);
        
        if ((i + 1) % 10 == 0) {
            printf("Started %d clients...\n", i + 1);
        }
    }
    
    printf("\nAll clients started. Monitoring...\n\n");
    
    // Wait for all clients to complete
    WaitForMultipleObjects(num_clients, events, TRUE, 60000);  // 60 second timeout
    g_running = 0;
    g_stats.end_time = GetTickCount();
    
    // Final report
    float elapsed = (float)(g_stats.end_time - g_stats.start_time) / 1000;
    
    printf("\n\n============================================================\n");
    printf("                     TEST COMPLETED                         \n");
    printf("============================================================\n");
    printf("Server IP: %s:%d\n", g_stats.server_ip, port);
    printf("Time: %.2f seconds\n", elapsed);
    printf("------------------------------------------------------------\n");
    printf("Total connections attempted: %ld\n", g_stats.total_connections);
    printf("Successful connections:      %ld\n", g_stats.successful_connections);
    printf("Failed connections:           %ld\n", g_stats.failed_connections);
    printf("Peak concurrent:              %ld\n", g_stats.peak_connections);
    printf("------------------------------------------------------------\n");
    printf("Total messages sent:          %ld\n", g_stats.total_messages_sent);
    printf("Total messages received:      %ld\n", g_stats.total_messages_received);
    printf("Messages per second:          %.1f\n", 
           (float)g_stats.total_messages_sent / elapsed);
    printf("============================================================\n");
    
    // Cleanup
    for (int i = 0; i < num_clients; i++) {
        CloseHandle(threads[i]);
        CloseHandle(events[i]);
    }
    
    free(threads);
    free(thread_data);
    free(events);
    CloseHandle(hStats);
    
    // Show cursor again
    cursorInfo.bVisible = TRUE;
    SetConsoleCursorInfo(g_console_handle, &cursorInfo);
    
    DeleteCriticalSection(&g_stats.cs);
    WSACleanup();
    
    return 0;
}