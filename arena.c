#include "arena.h"

alignas(16) ARENA_HEADER* arena_header = NULL;

ARENA_HEADER* Arena_Init(size_t block_size){

    u32 mem_area = MB;

    // ASKING FOR 10 "PAGES" (smallest unit of measurement that the operating system can manage)
    // MEM_RESERVE | MEM_COMMIT IS NECESSARY, AS WINDOWS KERNEL WONT ALLOW TO COMMIT IF YOU DIDN'T RESERVE FIRST
    ARENA_HEADER* aH = VirtualAlloc(NULL, mem_area, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

    while(block_size > mem_area)
        mem_area *= 16;

    // IF NOT DIVISIBLE BY 16, ADD ENOUGH BYTES TO MAKE IT
    block_size = makeDiv16(block_size);

    u32 total_blocks = (mem_area-sizeof(ARENA_HEADER))/block_size;

    // u8 = char, and it's used for 1 byte operations, like the one we do here to split each block
    u8* curr_block = (u8*)aH + sizeof(ARENA_HEADER);

    // INITIALIZE ARENA_HEADER
    aH->next_mem_block = (void*)curr_block;
    aH->sequence = 0;
    aH->block_size = block_size;

    // TOTAL_BLOCKS-1 SINCE THE LAST BLOCK WILL POINT TO NOTHING
    for(int i=0; i<total_blocks-1; i++){
        
        // This is getting the first 8 bytes from curr_block, and making it a usable void pointer
        void* next_pointer = curr_block;

        // then, we find the position of the next block
        u8* next_block = curr_block+block_size;

        *(void**)next_pointer = next_block;

        curr_block = next_block;
    }

    // LAST BLOCK POINTS TO NULL
    *(void**)curr_block = NULL;

    return aH;
}

ARENA_HEADER* Arena_Expand(ARENA_HEADER* arena_head, u32 buffer_size){
    
    size_t b_size = arena_head->block_size; 
    
    // Allocates new memory block
    void* new_mem = VirtualAlloc(NULL, buffer_size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    if(!new_mem) 
        return NULL;

    u32 total_blocks = buffer_size / b_size;
    
    // necessary to save, to make arena_header->next_mem_block equal to the first block of the new memory buffer
    u8* first_block = (u8*)new_mem;

    u8* curr = (u8*)new_mem;

    //  We slice the new memory buffer
    for (u32 i = 0; i < total_blocks - 1; i++) {
        *(void**)curr = curr + b_size;
        curr += b_size;
    }

    int retry_count = 1;

    // Now we must compare
    alignas(16) u64 snapshot[2];
    while (1) {
        snapshot[0] = (u64)arena_head->next_mem_block;
        snapshot[1] = arena_head->sequence;

        // Last block of the new memory buffer now points to the first block of the arena_header
        *(void**)curr = (void*)snapshot[0];

        // Now we compare arena_head to the snapshot, if equal, it means no other thread has changed it, so we can make it point to the new memory buffer
        if (CompareExchange16((volatile u64*)arena_head, snapshot[1] + 1, (u64)first_block, snapshot))
            break;
        else
            for(int j=0; j<retry_count; j++)  // SEQUENTLY LONGER PAUSES FOR EACH FAILURE
                _mm_pause();

        if(retry_count<64)
            retry_count++;
    }

    return arena_head;
}

void* Arena_Pop(ARENA_HEADER* arena_header){
    
    int retry_count = 1;
    
    // basically a void* array with 2 elements, since pointers also occupate 64 bits
    alignas(16) u64 snapshot[2];

    while(1){
        // FETCHING THE NEXT_MEM_BLOCK ADDRESS AS A 64 BIT NUMBER
        snapshot[0] = (u64)arena_header->next_mem_block;
        snapshot[1] = arena_header->sequence;
        
        // For reference, this will be block A
        void* block_to_return = (void*)snapshot[0];

        if(block_to_return == NULL){
            arena_header = Arena_Expand(arena_header, MB);
            if(arena_header == NULL)
                return NULL; // error
            continue;
        }

        // By dereferencing block A, we get the value of the NEXT block, let's call it block B
        u64 next_block = *(u64*)block_to_return;

        // COMPARES THE ARENA HEADER WITH THE SNAPSHOT WE TOOK, IF EQUAL, RETURNS TRUE, IF NOT, RETURNS FALSE
        // IF TRUE, arena_header->next_mem_block = next_block, making block_to_return free to use
        int success = CompareExchange16((volatile LONG64*)arena_header, (LONG64)(snapshot[1]+1), (LONG64)next_block, snapshot);
        if(success)
            return block_to_return;
        else
            for(int j=0; j<retry_count; j++)  // SEQUENTLY LONGER PAUSES FOR EACH FAILURE
                _mm_pause();

        if(retry_count<64)
            retry_count++;
    }
}

void Arena_Push(ARENA_HEADER* arena_header, void* block){
    int retry_count = 1;
    
    alignas(16) u64 snapshot[2];
    while(1){
        // FETCHING THE NEXT_MEM_BLOCK ADDRESS AS A 64 BIT NUMBER
        snapshot[0] = (u64)arena_header->next_mem_block;
        snapshot[1] = arena_header->sequence;

        *(void**)block = (void*)snapshot[0];

        int success = CompareExchange16((volatile LONG64*)arena_header, (LONG64)(snapshot[1]+1), (LONG64)block, snapshot);

        if(success)
            return;
        else
            for(int j=0; j<retry_count; j++)
                _mm_pause();

        if(retry_count<64)
            retry_count++;
    }
}
