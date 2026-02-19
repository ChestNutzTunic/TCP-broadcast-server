#include "dyn_arr.h"

struct cl_arr{
    CLIENT **cl_arr;
    int size;
    int capacity;
};

DYN_CLIENT_ARRAY* init_DSA(){
    DYN_CLIENT_ARRAY* Vec = malloc(sizeof(DYN_CLIENT_ARRAY));
    Vec->capacity = 1;
    Vec->size = 0;
    Vec->cl_arr = malloc(sizeof(CLIENT*));

    return Vec;
}

void add_to_DSA(DYN_CLIENT_ARRAY* Vec, CLIENT* new_item){
    if(Vec->capacity == Vec->size){
        Vec->capacity *=2;
        Vec->cl_arr = realloc(Vec->cl_arr, sizeof(CLIENT*)*Vec->capacity);
    }
    Vec->cl_arr[Vec->size] = new_item;
    Vec->size++;
}

void remove_of_DSA(DYN_CLIENT_ARRAY* Vec, CLIENT* s){
    int id = -1;
    for(int i=0; i<Vec->size; i++){
        if(Vec->cl_arr[i] == s){
            id = i;
            break;
        }
    }

    if(id != -1){

        if(id < Vec->size - 1){
            memmove(&Vec->cl_arr[id], &Vec->cl_arr[id+1], (Vec->size - 1 - id) * sizeof(CLIENT*));
        }
        
        Vec->size--;
        
    }
}

int sizeof_DSA(DYN_CLIENT_ARRAY* Vec){
    return Vec->size;
}

CLIENT* get_elem_DSA(DYN_CLIENT_ARRAY* Vec, int id){
    if(Vec->size > id)
        return (Vec->cl_arr[id]);

    return NULL;
}

void free_DSA(DYN_CLIENT_ARRAY* Vec){
    for(int i=0; i<Vec->size; i++)
        release_client(Vec->cl_arr[i]);
    
    free(Vec->cl_arr);
    free(Vec);
}
