#include <string.h>
#include <stdio.h>
#include <stdlib.h>

// Checks if a buffer is empty
int bufferEmpty(char *buff, int buffsize){
    for(int i = 0; i < buffsize; i++){
        if(buff[i] != 0){
            return 0;
        }
    }
    return 1;
}

// Gets last delimiter position in buffer
int getLastDel(char *buff, int buffsize){
    int lastDel = -1;

    for(int i = buffsize-1; i >= 0; i--){
        if(buff[i] == '\n'){
            lastDel = i;
            break;
        }
    }

    return lastDel;
}

// Checks if a string fits in a buffer
int strFits(char *buff, int buffsize, char *str){
    int len = strlen(str)+1, lastDel = getLastDel(buff, buffsize);

    // covers both cases since -(-1) cancels out with -1
    int rem_len = buffsize-lastDel-1;

    return len <= rem_len;
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

// Remove last string from buffer and return it
char *buffGetLast(char *buff, int buffsize){
    int prev_del = -1; // Previous from last delimiter
    int last_del = getLastDel(buff, buffsize);

    for(int i = last_del-1; i >= 0; i--){
        if(buff[i] == '\n'){
            prev_del = i;
            break;
        }
    }

    char *res = NULL;
    int len = 0;
    
    for(int i = prev_del+1; i < last_del+1; i++){
        if(buff[i] == '\n'){
            buff[i] = 0;
            break;
        }

        res = realloc(res, ++len+1);
        res[len-1] = buff[i];
        buff[i] = 0;
    }
    res[len] = '\0';

    return res;
}
