#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "htInterface.h"
#define Nil -1
#define defSize 65
#define maxWord 40

int HTHashFunction(char *key, int size){
	int res = 0, i = 0;
	while(key[i] != '\0'){
		res += key[i++];
	}
	return res%size;
}

HTHash HTCreate(void){
	HTHash hash = malloc(sizeof(struct hthash));
	hash->curSize = defSize;
	hash->entries = malloc(sizeof(HTEntry)*defSize);
	for(int i = 0; i < defSize; i++) hash->entries[i] = (HTEntry)Nil;
	return hash;
}

void HTRehash(HTHash hash){
	HTHash newhash = malloc(sizeof(struct hthash));
	newhash->curSize = 2*hash->curSize;
	newhash->entries = malloc(sizeof(HTEntry)*2*hash->curSize);
	for(int i = 0; i < newhash->curSize; i++) newhash->entries[i] = (HTEntry)Nil;
	
	for(int i = 0; i < hash->curSize; i++){
		if(hash->entries[i] != (HTEntry)Nil){
			HTInsert(newhash, hash->entries[i]->key, hash->entries[i]->item);	
		}
	}
	free(hash->entries);
	hash->entries = newhash->entries;
	hash->curSize = newhash->curSize;
}

HTHash HTInsert(HTHash hash, char *key, HTItem item){
	HTEntry entry = malloc(sizeof(struct hashentry));
	entry->key = malloc(sizeof(char)*maxWord);
	strcpy(entry->key, key);
	entry->item = item;

	int index = HTHashFunction(key, hash->curSize);
	int n = 0;
	while(n++ != hash->curSize+1 && hash->entries[index] != (HTEntry)Nil){
		if(!strcmp(hash->entries[index]->key, key)){
			break;
		}
		index++;
		if(index == hash->curSize) index = 0;
	}
	if(n != hash->curSize+1) hash->entries[index] = entry;

	double loadratio = HTSize(hash)/(double)hash->curSize;
	if(loadratio >= 0.9){
		HTRehash(hash);
	}
	return hash;
}

int HTSize(HTHash hash){
	int size = 0;
	for(int i = 0; i < hash->curSize; i++){
		if(hash->entries[i] != (HTEntry)Nil) size++;
	}
	return size;
}

int HTGet(HTHash hash, char *key, HTItem *itemptr){
	int index = HTHashFunction(key, hash->curSize);
	int n = 0;
	while(n++ != hash->curSize+1 && strcmp(hash->entries[index]->key, key)){
		index++;
		if(index == hash->curSize) index = 0;
	}
	if(n != hash->curSize+1){
		*itemptr = hash->entries[index]->item;
		return 1;
	}
	else return -1;
}

int HTGetIndex(HTHash hash, char *key){
	int index = HTHashFunction(key, hash->curSize);
	int n = 0;
	while(n++ != hash->curSize+1 && strcmp(hash->entries[index]->key, key)){
		index++;
		if(index == hash->curSize) index = 0;
	}
	if(n != hash->curSize+1){
		return index;
	}
	else return -1;
}

void HTRemove(HTHash hash, char *key){
	int index = HTGetIndex(hash, key);
	free(hash->entries[index]->key);
	hash->entries[index] = (HTEntry)Nil;
}

void HTVisit(HTHash hash, void (*visit)(HTHash, char *, HTItem)){
	for(int i = 0; i < hash->curSize; i++){
		if(hash->entries[i] != (HTEntry)Nil) visit(hash, hash->entries[i]->key, hash->entries[i]->item);	
	}
}

void HTDestroy(HTHash hash){
	for(int i = 0; i < hash->curSize; i++){
		if(hash->entries[i] != (HTEntry)Nil){
			free(hash->entries[i]->key);
			hash->entries[i] = (HTEntry)Nil;
		}
	}
	hash->curSize = 0;
	free(hash);
}

HTItem HTGetItem(HTHash hash, char *key){
	int index = HTHashFunction(key, hash->curSize);
	int n = 0;
	while(n++ != hash->curSize+1 && strcmp(hash->entries[index]->key, key)){
		index++;
		if(index == hash->curSize) index = 0;
	}
	if(n != hash->curSize+1){
		return hash->entries[index]->item;
	}
	else return NULL;
}
