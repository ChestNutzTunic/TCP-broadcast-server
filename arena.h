#pragma once
#include "crypto.h"
#include <immintrin.h>

// EACH PAGE IS 4KB
#define MB (1024*1024)

// THIS ARENA WILL BE SPECIFIC FOR COM_PORT_INFO

#define makeDiv16(n) ((n+15)&~15)

extern int CompareExchange16(volatile void* Dest, unsigned __int64 exchangeHIGH, unsigned __int64 exchangeLOW, unsigned __int64* Src);

typedef struct{
    void* next_mem_block;
    u64 sequence;
    u64 block_size;
    u64 blocks_left;
} ARENA_HEADER;

extern ARENA_HEADER* arena_header;

ARENA_HEADER* Arena_Init(size_t block_size);
void* Arena_Pop(ARENA_HEADER* arena_header);
void Arena_Push(ARENA_HEADER* arena_header, void* block);
void Arena_Expand(ARENA_HEADER* arena_head);