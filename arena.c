#include "arena.h"

alignas(16) ARENA_HEADER* arena_header = NULL;

volatile long is_expanding = 0;
volatile long total_blocks = 0;

ARENA_HEADER* Arena_Init(size_t block_size){

    u64 mem_area = 5*MB;

    // ASKING FOR 10 "PAGES" (smallest unit of measurement that the operating system can manage)
    // MEM_RESERVE | MEM_COMMIT IS NECESSARY, AS WINDOWS KERNEL WONT ALLOW TO COMMIT IF YOU DIDN'T RESERVE FIRST
    ARENA_HEADER* aH = VirtualAlloc(NULL, mem_area, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

    while(block_size > mem_area)
        mem_area *= 16;

    // IF NOT DIVISIBLE BY 16, ADD ENOUGH BYTES TO MAKE IT BE
    block_size = makeDiv16(block_size);

    u32 block_quantity = (mem_area-sizeof(ARENA_HEADER))/block_size;
    total_blocks = block_quantity;
    aH->blocks_left = block_quantity;

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

void Arena_Expand(ARENA_HEADER* arena_header){
    
    size_t b_size = arena_header->block_size;
    
    u64 buffer_size = 5*MB;

    // Allocates new memory block
    void* new_mem = VirtualAlloc(NULL, buffer_size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    if(!new_mem) 
        return;

    u32 block_quantity = buffer_size / b_size;

    // necessary to save, to make arena_header->next_mem_block equal to the first block of the new memory buffer
    u8* first_block = (u8*)new_mem;

    u8* curr = (u8*)new_mem;

    //  We slice the new memory buffer
    for (u32 i = 0; i < block_quantity - 1; i++) {
        *(void**)curr = curr + b_size;
        curr += b_size;
    }

    int retry_count = 1;

    // Now we must compare
    alignas(16) u64 snapshot[2];
    while (1) {
        snapshot[0] = (u64)arena_header->next_mem_block;
        snapshot[1] = arena_header->sequence;

        // Last block of the new memory buffer now points to the first block of the arena_header
        *(void**)curr = (void*)snapshot[0];

        // Now we compare arena_head to the snapshot, if equal, it means no other thread has changed it, so we can make it point to the new memory buffer
        if (CompareExchange16((volatile u64*)arena_header, snapshot[1] + 1, (u64)first_block, snapshot))
            break;
        else
            for(int j=0; j<retry_count; j++)  // SEQUENTLY LONGER PAUSES FOR EACH FAILURE
                _mm_pause();

        if(retry_count<64)
            retry_count++;
        }
    
    InterlockedAdd((volatile long*)&(arena_header->blocks_left), block_quantity);
    InterlockedAdd(&total_blocks, block_quantity);
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
            if(_InterlockedCompareExchange(&is_expanding, 1, 0) == 0){
                
                Arena_Expand(arena_header);
                
                InterlockedExchange(&is_expanding, 0);

                continue;
            }
            else{
                _mm_pause();
                continue;
            }
        }

        // By dereferencing block A, we get the value of the NEXT block, let's call it block B
        void* next_block = *(void**)block_to_return;
        
        // COMPARES THE ARENA HEADER WITH THE SNAPSHOT WE TOOK, IF EQUAL, RETURNS TRUE, IF NOT, RETURNS FALSE
        // IF TRUE, arena_header->next_mem_block = next_block, making block_to_return free to use
        int success = CompareExchange16((volatile LONG64*)arena_header, (LONG64)(snapshot[1]+1), (LONG64)next_block, snapshot);
        if(success){
            
            InterlockedDecrement((volatile long*)&arena_header->blocks_left);
            
            // by using this, we know for a fact that it's going to be false, and so, it's not going to switch anything, while also loading the true actual value in current_total
            long current_total = _InterlockedCompareExchange(&total_blocks, 0, 0);
            u64 arena_threshold = (current_total*15)/100;

            // necessary for threads to not be able to expand the arena simultaneously
            if(arena_header->blocks_left <= arena_threshold){
                if(_InterlockedCompareExchange(&is_expanding, 1, 0) == 0){
                    
                    Arena_Expand(arena_header);

                    InterlockedExchange(&is_expanding, 0);
 
                }
            }
                return block_to_return;
            }
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

        if(success){
            InterlockedIncrement((volatile long*)&arena_header->blocks_left);
            return;
        }
        else
            for(int j=0; j<retry_count; j++)
                _mm_pause();

        if(retry_count<64)
            retry_count++;
    }
}
