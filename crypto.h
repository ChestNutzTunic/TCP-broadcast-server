#pragma once
#include <stdlib.h>
#include <stdio.h>
#include <winsock2.h>
#include <windows.h>
#include <stdint.h>
#include <stdalign.h>

typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

#define MAX_BUFFER_SIZE 512

typedef struct{
    unsigned char sbox[256]; 
    u16 crypto_i;
    u16 crypto_j;
    
} CRYPTO_INFO;

typedef struct{
    CRYPTO_INFO cryp_info;
    CRITICAL_SECTION CS_lock;
    SOCKET comm_channel;
    volatile LONG ref_counting;
    u64 last_msg_time;
    DWORD client_id;
    u8 flood_threshold;
    
} CLIENT;

typedef enum{
    OP_WRITE,
    OP_WRITE_DONE,
} OP_INFO;

typedef struct{

    OVERLAPPED overlapped;
    char buffer[MAX_BUFFER_SIZE];
    CLIENT* client;
    WSABUF wsabuf;
    OP_INFO operation_info;

} COM_PORT_INFO;

CLIENT* initialize_client(SOCKET comm, DWORD id, unsigned char* KEY);

void cipher_buffer(CLIENT* cl, char* data, u32 len);