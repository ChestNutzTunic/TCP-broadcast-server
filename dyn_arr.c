#include "dyn_arr.h"

struct cl_arr{
    CLIENT **cl_arr;
    u32 size;
    u32 capacity;
};

DYN_CLIENT_ARRAY* init_DCA(){
    DYN_CLIENT_ARRAY* Vec = malloc(sizeof(DYN_CLIENT_ARRAY));
    Vec->capacity = 1;
    Vec->size = 0;
    Vec->cl_arr = malloc(sizeof(CLIENT*));
    return Vec;
}

void add_to_DCA(DYN_CLIENT_ARRAY* Vec, CLIENT* new_item){
    if(Vec->capacity == Vec->size){
        Vec->capacity *= 2;
        Vec->cl_arr = realloc(Vec->cl_arr, sizeof(CLIENT*)*Vec->capacity);
    }
    Vec->cl_arr[Vec->size] = new_item;
    Vec->size++;
}

// IT'S GOING TO REMOVE OF THE DYNAMIC ARRAY AND CLOSE THE SOCKET, BUT IT IS *NOT* GOING TO FREE THE CLIENT OBJECT, SINCE IT IS MANAGED BY THE REF_COUNTING VARIABLE
// IMPORTANT NOTE: THIS FUNCTION CLOSES THE SOCKET SO ANY "WSASend" REQUEST IS AUTOMATICALLY DISCARTED BY THE KERNEL, SO THIS SOCKET WILL NOT RECEIVE ANY FURTHER MESSAGES
void remove_of_DCA(DYN_CLIENT_ARRAY* Vec, CLIENT* s){
    i32 id = -1;
    for(u32 i=0; i < Vec->size; i++){
        if(Vec->cl_arr[i] == s){
            id = i;
            closesocket(Vec->cl_arr[i]->comm_channel);
            break;
        }
    }
    
    // CLIENT OBJECT IS STILL ALLOCATED, JUST NOT IN THE LIST
    if(id != -1){
        // SWITCHES POSITION WITH THE LAST ELEMENT, SINCE SEQUENCE DOESN'T AFFECT CHAT PERFORMANCE
        Vec->cl_arr[id] = Vec->cl_arr[Vec->size - 1];
        Vec->size--;
    }
}

u32 sizeof_DCA(DYN_CLIENT_ARRAY* Vec){
    return Vec->size;
}

CLIENT* get_elem_DCA(DYN_CLIENT_ARRAY* Vec, u32 id){
    if(id >= 0 && id < Vec->size)
        return (Vec->cl_arr[id]);

    return NULL;
}

void free_DCA(DYN_CLIENT_ARRAY* Vec){
    for(u32 i=0; i<Vec->size; i++){
        closesocket(Vec->cl_arr[i]->comm_channel);

        if(InterlockedDecrement(&(Vec->cl_arr[i]->ref_counting)) == 0){
             free(Vec->cl_arr[i]);
        }
        
    }
    free(Vec->cl_arr);
    free(Vec);
}

// FUNCTION FOR GETTING PRE-FORMATTED STRING AND BROADCASTING IT
void broadcast_to_DCA(COM_PORT_INFO* client_info, DYN_CLIENT_ARRAY* DCA, char* bufferOUT, int size_buff){

    for(u32 i=0; i<sizeof_DCA(DCA); i++){
        CLIENT* S = get_elem_DCA(DCA, i);

        if(S!=NULL && S->comm_channel != INVALID_SOCKET && S != client_info->client){
            char buff[MAX_BUFFER_SIZE];

            memcpy(buff, bufferOUT, size_buff);
            // CIPHER MESSAGE
            cipher_buffer(S, buff, size_buff);

            // COPYING COMMUNICATION PORT INFO
            COM_PORT_INFO* write_context = malloc(sizeof(COM_PORT_INFO));
            write_context->client = S;
            write_context->operation_info = OP_WRITE_DONE;

            write_context->wsabuf.buf = write_context->buffer;
            write_context->wsabuf.len = size_buff;
            memset(&write_context->overlapped, 0, sizeof(OVERLAPPED));

            memcpy(write_context->buffer, buff, size_buff);
            
            // MESSAGES COUNT (WHEN 0, FREE CLIENT)
            InterlockedIncrement(&(S->ref_counting));

            // IF WSASend is successful, returns 0, otherwise, returns SOCKET_ERROR (-1)
            int res = WSASend(S->comm_channel, &(write_context->wsabuf), 1, NULL, 0, &(write_context->overlapped), NULL);
            if(res == SOCKET_ERROR){
                int err = WSAGetLastError();
                if(err != WSA_IO_PENDING)
                    // IMEDIATE ERROR, DECREMENT COUNTER
                    InterlockedDecrement(&(S->ref_counting));
                    free(write_context);
            }
            // IF res == 0 or lastError == WSA_IO_PENDING, the information is still being processed by the kernel

        }
    }

}