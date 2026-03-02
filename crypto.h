#pragma once
#include <stdlib.h>
#include <stdio.h>
#include <windows.h>
#include <winsock2.h>
#include <stdint.h>

typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

#define MAX_BUFFER_SIZE 1152 // 1024 + 128 (extra space for texts headers)

typedef struct{
    unsigned char sbox[256]; 
    u16 crypto_i;
    u16 crypto_j;
    
} CRYPTO_INFO;

typedef struct{
    SOCKET comm_channel;
    DWORD client_id;
    CRYPTO_INFO cryp_info;
    volatile LONG ref_counting;
    
} CLIENT;

typedef enum{
    OP_WRITE,
    OP_WRITE_DONE,
} OP_INFO;

typedef struct{

    OVERLAPPED overlapped;
    CLIENT* client;
    WSABUF wsabuf;
    char buffer[1024];
    OP_INFO operation_info;

} COM_PORT_INFO;

CLIENT* initialize_client(SOCKET comm, DWORD id, unsigned char* KEY);

void cipher_buffer(CLIENT* cl, char* data, u32 len);