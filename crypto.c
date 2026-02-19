#include "crypto.h"

void release_client(CLIENT* cl){
    closesocket(cl->comm_channel); 
    free(cl);
}

CLIENT* initialize_client(SOCKET comm, DWORD id, unsigned char* KEY){
    CLIENT* cl = malloc(sizeof(CLIENT));
    cl->comm_channel = comm;
    cl->client_id = id;

    for(int i=0; i<256; i++){
        cl->cryp_info.sbox[i] = i;
    }
    
    int key_lenght = strlen(KEY);
    int j=0;
    // SCRAMBLING THE SUBSTITUTION BOX
    for(int i=0; i<256; i++){
        // 0x9E3779B9 = GOLDEN RATION, cuz... why not?
        j = (j + cl->cryp_info.sbox[i] + KEY[i % key_lenght] + 0x9E3779B9) % 256;

        unsigned char temp = cl->cryp_info.sbox[i];
        cl->cryp_info.sbox[i] = cl->cryp_info.sbox[j];
        cl->cryp_info.sbox[j] = temp;
    }
    // SUBSTITUTION BOX PURPOSE IS TO ACHIEVE PRGA (Pseudo-Random Generation Algorithm)

    cl->cryp_info.crypto_i = cl->cryp_info.crypto_j = 0;

    return cl;
}

void cipher_buffer(CLIENT* cl, char* data, int len){
    if(len <= 0) return;

    int* i = &cl->cryp_info.crypto_i;
    int* j = &cl->cryp_info.crypto_j;

    for(int k=0; k<len; k++){
        // i = linear counter  
        *i = (*i+1) % 256;
        // j = jumps to an "unpredictable" position based in the substitution box value
        *j = (*j+cl->cryp_info.sbox[*i]) % 256;

        // SWAP
        unsigned char temp = cl->cryp_info.sbox[*i];
        cl->cryp_info.sbox[*i] = cl->cryp_info.sbox[*j];
        cl->cryp_info.sbox[*j] = temp;

        // KEYSTREAM from sbox[i] + sbox[j]
        int t = (cl->cryp_info.sbox[*i] + cl->cryp_info.sbox[*j]) % 256;

        data[k] ^= cl->cryp_info.sbox[t];
    }
}