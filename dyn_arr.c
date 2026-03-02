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