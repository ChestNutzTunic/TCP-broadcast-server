#include "dyn_arr.h"

struct sock_arr{
    SOCKET *sock_arr;
    int size;
    int capacity;
};

DYN_SOCKET_ARRAY* init_DSA(){
    DYN_SOCKET_ARRAY* Vec = malloc(sizeof(DYN_SOCKET_ARRAY));
    Vec->capacity = 1;
    Vec->size = 0;
    Vec->sock_arr = malloc(sizeof(SOCKET));

    return Vec;
}

void add_to_DSA(DYN_SOCKET_ARRAY* Vec, SOCKET new_item){
    if(Vec->capacity == Vec->size){
        Vec->capacity *=2;
        Vec->sock_arr = realloc(Vec->sock_arr, sizeof(SOCKET)*Vec->capacity);
    }
    Vec->sock_arr[Vec->size] = new_item;
    Vec->size++;
}

void remove_of_DSA(DYN_SOCKET_ARRAY* Vec, SOCKET s){
    int id = -1;
    for(int i=0; i<Vec->size-1; i++){
        if(Vec->sock_arr[i] == s){
            id = i;
            break;
        }
    }

    if(id != -1){
        closesocket(Vec->sock_arr[id]);

        if(id < Vec->size-1)
            memmove(&Vec->sock_arr[id], &Vec->sock_arr[id+1], (Vec->size - 1 - id) * sizeof(SOCKET));

        Vec->size--;
    }
}

int sizeof_DSA(DYN_SOCKET_ARRAY* Vec){
    return Vec->size;
}

SOCKET* get_elem_DSA(DYN_SOCKET_ARRAY* Vec, int id){
    if(Vec->size > id)
        return &(Vec->sock_arr[id]);
}

void free_DSA(DYN_SOCKET_ARRAY* Vec){
    free(Vec->sock_arr);
    free(Vec);
}
