#include <string.h>
#include <stdio.h>
#include <stdlib.h>

// Checks if a buffer is empty
int bufferEmpty(char *buff, int buffsize){
    for(int i = 0; i < buffsize; i++){
        if(buff[i] != 0){
            return 1;
        }
    }
    return 0;
}

// Gets last delimiter position in buffer
int getLastDel(char *buff, int buffsize){
    int lastDel = -1;

    for(int i = buffsize-1; i >= 0; i--){
        if(buff[i] == '\n'){
            lastDel = i;
        }
    }

    return lastDel;
}

// Checks if a string fits in a buffer
int strFits(char *buff, int buffsize, char *str){
    int len = strlen(str)+1, lastDel = getLastDel(buff, buffsize);

    // covers both cases since -(-1) cancels out with -1
    int rem_len = buffsize-lastDel-1;

    return len < rem_len;
}

// Inserts string to buffer
void buffInsert(char *buff, int buffsize, char *str){
    int lastDel = getLastDel(buff, buffsize), len = strlen(str)+1;

    int str_ind = 0;
    for(int i = lastDel+1; i < lastDel+len; i++){
        buff[i] = str[str_ind++];
    }
    buff[lastDel+len] = '\n'; 
}

// Remove first string from buffer and return it
char *buffGetFirst(char *buff, int buffsize){
    char *res = malloc(1);
    int len = 0;
    
    for(int i = 0; i < buffsize; i++){
        if(buff[i] == '\n'){
            buff[i] = 0;
            break;
        }

        res = realloc(res, ++len);
        res[i] = buff[i];
        buff[i] = 0;
    }

    return res;
}
