#include "crypto.h"

CLIENT* initialize_client(SOCKET comm, DWORD id, unsigned char* KEY){
    CLIENT* cl = malloc(sizeof(CLIENT));
    cl->comm_channel = comm;
    cl->client_id = id;

    // Initialize the Substitution Box
    for(u16 i=0; i<256; i++){
        cl->cryp_info.sbox[i] = i;
    }

    cl->ref_counting = 1;
    
    u32 key_lenght = strlen(KEY);
    u16 j=0;
    
    // SCRAMBLING THE SUBSTITUTION BOX USING A PRGA (PSEUDO-RANDOM GENERATOR ALGORITHM)
    for(u16 i=0; i<256; i++){
        // 0x9E3779B9 = GOLDEN RATIO, cuz... why not?
        j = (j + cl->cryp_info.sbox[i] + KEY[i % key_lenght] + 0x9E3779B9) % 256;

        unsigned char temp = cl->cryp_info.sbox[i];
        cl->cryp_info.sbox[i] = cl->cryp_info.sbox[j];
        cl->cryp_info.sbox[j] = temp;
    }

    cl->cryp_info.crypto_i = cl->cryp_info.crypto_j = 0;

    return cl;
}

void cipher_buffer(CLIENT* cl, char* data, u32 len){
    if(len <= 0) return;

    u16* i = &cl->cryp_info.crypto_i;
    u16* j = &cl->cryp_info.crypto_j;

    for(u64 k=0; k<len; k++){
        // i = linear counter  
        *i = (*i+1) % 256;
        // j = pseudorandom jump based on sbox values
        *j = (*j+cl->cryp_info.sbox[*i]) % 256;

        // SWAP state values
        unsigned char temp = cl->cryp_info.sbox[*i];
        cl->cryp_info.sbox[*i] = cl->cryp_info.sbox[*j];
        cl->cryp_info.sbox[*j] = temp;

        // Generate KEYSTREAM from sbox state
        u16 t = (cl->cryp_info.sbox[*i] + cl->cryp_info.sbox[*j]) % 256;

        data[k] ^= cl->cryp_info.sbox[t];

    }
}