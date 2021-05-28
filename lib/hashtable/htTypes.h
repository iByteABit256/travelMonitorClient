#ifndef HT
#define HT
#ifdef open_adress

typedef void *HTItem;

struct hashentry{
	char *key;
	HTItem item;
};

typedef struct hashentry *HTEntry;

struct hthash{
	int curSize;
	HTEntry *entries;
};

typedef struct hthash *HTHash;

#else
#include "../lists/lists.h"

struct hthash{
	int curSize;
	Listptr *ht;
};

typedef struct hthash *HTHash;

typedef void *HTItem;

struct hashentry{
	char *key;
	HTItem item;
};

typedef struct hashentry *HTEntry;

#endif

#endif
