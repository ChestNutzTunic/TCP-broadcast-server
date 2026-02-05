#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <winsock2.h>
#include <windows.h>

typedef struct sock_arr DYN_SOCKET_ARRAY;

DYN_SOCKET_ARRAY* init_DSA();

void add_to_DSA(DYN_SOCKET_ARRAY* Vec, SOCKET new_item);

int sizeof_DSA(DYN_SOCKET_ARRAY* Vec);

SOCKET* get_elem_DSA(DYN_SOCKET_ARRAY* Vec, int id);

void free_DSA(DYN_SOCKET_ARRAY* Vec);

void remove_of_DSA(DYN_SOCKET_ARRAY* Vec, SOCKET s);