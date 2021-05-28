#ifndef LST
#define LST
typedef void *ItemType;
typedef int(*CompareType)(ItemType a, ItemType b);

typedef struct node{
   ItemType value;
   struct node *next;
   struct node *head;
   struct node *tail;
}Listnode;

typedef Listnode *Listptr;
#endif
