#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bloomfilter.h"
#include "../hash/hash.h" 


// Set n-th bit of array
int setBit(unsigned char *str, unsigned long pos){
    *(str+pos/8) |= (1 << pos%8);

    return 1;
}

// Check n-th bit of array
int checkBit(unsigned char *str, unsigned long pos){
    return (*(str+pos/8) & (1 << pos%8))? 1 : 0;
}

// Initializes a new bloom filter with given size in bytes
BloomFilter bloomInitialize(unsigned int size){
    BloomFilter bl = malloc(sizeof(struct bloomstr));
    bl->bloom = malloc(size);
    bl->size = size;
    memset(bl->bloom, 0, size);

    return bl;
}

// Performs logical OR between two bloom filters
int bloomOR(BloomFilter dest, BloomFilter src){
    if(dest == NULL || src == NULL){
        return 0;
    }

    if(dest->size != src->size){
        return 0;
    }

    for(int i = 0; i < dest->size; i++){
        if(checkBit(src->bloom, i)){
            setBit(dest->bloom, i);
        }
    }
    return 1;
}

// Inserts new entry in bloom filter
unsigned int bloomInsert(BloomFilter bl, char *s){

    for(int i = 0; i < 16; i++){
        setBit(bl->bloom, hash_i((unsigned char *)s,i)%bl->size);
    } 

    return 1;
}

// Checks if entry exists in bloom filter
unsigned int bloomExists(BloomFilter bl, char *s){
    for(int i = 0; i < 16; i++){
        if(checkBit(bl->bloom, hash_i((unsigned char *)s,i)%bl->size) == 0){
            return 0;
        }
    } 

    return 1;
}

// Destroys bloomfilter
void bloomDestroy(BloomFilter bl){
    free(bl->bloom);
}
