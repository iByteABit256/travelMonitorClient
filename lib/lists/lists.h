#include "listsTypes.h"
Listptr ListCreate();
void ListPrintList(Listptr list);
Listptr ListSearch(Listptr list, ItemType item);
void ListInsertLast(Listptr list, ItemType item);
void ListInsertAfter(Listptr list, ItemType item, Listptr node);
void ListInsertSorted(Listptr list, ItemType item, int (compareFunction)(ItemType a, ItemType b));
void ListDeleteLast(Listptr list);
void ListDelete(Listptr list, Listptr node);
void ListDestroy(Listptr list);
int ListSize(Listptr list);
Listptr ListGetNext(Listptr);
Listptr ListGetFirst(Listptr);
Listptr ListGetLast(Listptr);
