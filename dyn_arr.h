#pragma once
#include "crypto.h"

typedef struct cl_arr DYN_CLIENT_ARRAY;

DYN_CLIENT_ARRAY* init_DCA();

void add_to_DCA(DYN_CLIENT_ARRAY* Vec, CLIENT* new_item);

u32 sizeof_DCA(DYN_CLIENT_ARRAY* Vec);

CLIENT* get_elem_DCA(DYN_CLIENT_ARRAY* Vec, u32 id);

void free_DCA(DYN_CLIENT_ARRAY* Vec);

void remove_of_DCA(DYN_CLIENT_ARRAY* Vec, CLIENT* s);

void broadcast_to_DCA(COM_PORT_INFO* client_info, DYN_CLIENT_ARRAY* DCA, char* bufferOUT, int size_buff);