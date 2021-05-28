#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "listsTypes.h"

char *ItemFormat = "%s";

// Initializes new list
Listptr ListCreate(){ 
	Listptr head = malloc(sizeof(Listnode));
	Listptr tail = malloc(sizeof(Listnode));
	head->next = tail;
	head->tail = tail;
	head->head = head;
	tail->head = head;
	tail->tail = tail;
	tail->next = NULL;
   
	return head;
}

// # of items in list
int ListSize(Listptr list){
	int size = 0;
	if(list == NULL) return 0;
	while(list->next != list->tail){
		list = list->next;
		size++;
	}
	return size;
}

// prints items in list using ItemFormat
void ListPrintList(Listptr list){
   if(list == NULL){
      printf("Error: list not initialized\n");
	}else{
		if(list->head->next != list->tail){
			list = list->head->next;
			while(list->next != list->tail){
				printf(ItemFormat, list->value);
				printf(" -> ");
				list = list->next;
			}
			printf(ItemFormat, list->value);
			printf("\n");
		}else printf("List is empty\n");
	}
}

// searches for item in list, returns pointer if found
Listptr ListSearch(Listptr list, ItemType item){
	if(list == NULL){
		return NULL;
	}
	if(list->next == list->tail) return NULL;
	else list = list->next;
	while(list->next != list->tail){
		if(item ==  list->value) return list;
		else list = list->next;
	}
	if(item == list->value) return list;
	return NULL;
}

// inserts at the end of list
void ListInsertLast(Listptr list, ItemType item){
   if(list == NULL){
      printf("Error: list not initialized\n");
   }
   Listptr new = malloc(sizeof(Listnode));
   new->head = list->head;
   new->tail = list->tail;
   new->next = list->tail;
   new->value = item;
   Listptr temp = list->head;
   while(temp->next != list->tail) temp = temp->next;
   temp->next = new;
}

// inserts after specific list node
void ListInsertAfter(Listptr list, ItemType item, Listptr node){
    if(list == NULL){
		printf("Error: list not initialized\n");
	}
	if(node != list->tail){
		Listptr new = malloc(sizeof(Listnode));
		new->head = list->head;
		new->tail = list->tail;
		new->next = node->next;
		new->value = item;
		node->next = new;
	}else{
		printf("Error: Incorrect parameters for insertAfter(Listptr, ItemType, Listptr)\n");
	}
}

// inserts in a sorted way
void ListInsertSorted(Listptr list, ItemType item, int (compareFunc)(ItemType a, ItemType b)){
   if(list == NULL){
      printf("Error: list not initialized\n");
   }
   Listptr new = malloc(sizeof(Listnode));
   new->head = list->head;
   new->tail = list->tail;
   new->next = list->tail;
   new->value = item;
   Listptr temp;

   for(temp = list->head; temp->next != list->tail; temp = temp->next){
	   if(compareFunc(item, temp->next->value) < 0){
		   new->next = temp->next;
		   temp->next = new;
		   return;
	   }
   }
   new->next = temp->next;
   temp->next = new;
}

// deletes last element of list
void ListDeleteLast(Listptr list){
	if(list == NULL){
		printf("Error: list not initialized\n");
	}else{
		if(list->head->next == list->tail){
			printf("List is empty\n");
		}else{      
			Listptr temp = list->head; 
			while(temp->next->next != temp->tail) temp = temp->next;
			Listptr last = temp->next;
			temp->next = temp->tail;
			free(last);
		}
	}
}

// deletes list node from list
void ListDelete(Listptr list, Listptr node){
	if(list != NULL){
		if(node == list->head || node == list->tail) printf("Error: Incorrect parameters for delete(Listptr, Listptr)\n");
		else{
			while(list->next != node && list->next != list->tail){
				list = list->next;
			}   
			if(list->next != list->tail){
				list->next = node->next;
				free(node);
			}
		}
	}else{
		printf("Error: list not initialized\n");
	}
}

// destroys list and frees memory
void ListDestroy(Listptr list){
	if(list == NULL) return;
	Listptr next;
	for(Listptr l = list->head; l != NULL; l = next){
		next = l->next;
		free(l);
	}
}

// returns next list node
Listptr ListGetNext(Listptr list){
	if(list == NULL || list->next == list->tail) return NULL;
	return list->next;
}

// returns start of list
Listptr ListGetFirst(Listptr list){
	if(list == NULL) return NULL;
	list = list->head;
	if(list->next != list->tail) return list->next;
	else return NULL;
}

// returns tail of list
Listptr ListGetLast(Listptr list){
	if(list == NULL) return NULL;
	Listptr l;
	for(l = list->head; l->next != l->tail; l = l->next);
	if(l == list->head) return NULL;
	else return l;
}
