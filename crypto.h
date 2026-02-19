#pragma once
#include <stdlib.h>
#include <stdio.h>
#include <windows.h>
#include <winsock2.h>

typedef struct{
    unsigned char sbox[256]; 
    int crypto_i;
    int crypto_j;
    
} CRYPTO_INFO;

typedef struct{
    SOCKET comm_channel;
    DWORD client_id;
    CRYPTO_INFO cryp_info;
    
} CLIENT;

void release_client(CLIENT* cl);

CLIENT* initialize_client(SOCKET comm, DWORD id, unsigned char* KEY);

void cipher_buffer(CLIENT* cl, char* data, int len);