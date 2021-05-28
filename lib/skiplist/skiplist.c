#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "skiplist.h"


// Constructs new skip node
skipNode newSkipNode(char *key, void *item, int lvl){
    skipNode node = malloc(sizeof(struct node_struct)); 

    // Pass by reference to save memory
    node->key = key;

    // Pass by reference
    node->item = item;

    // Allocate forward array and initialize to zero 
    node->forward = malloc(sizeof(skipNode *)*(lvl+1));
    memset(node->forward, 0, sizeof(skipNode *)*(lvl+1));

    return node;
}

// Destroys skip node
void destroySkipNode(skipNode sn){
    free(sn->forward);
}

// Constructs new skip list with given maximum level and
// coin toss heads probability
Skiplist newSkiplist(int maxlvl, float p){
    Skiplist sl = malloc(sizeof(struct skiplist_struct));

    sl->maxlvl = maxlvl;
    sl->p = p;
    sl->lvl = 0;
    sl->dummy = newSkipNode(NULL, NULL, maxlvl);

    return sl;
}

// Random amount of levels for skiplist node
int coinToss(Skiplist sl){
    // Coin toss
    float toss = (float)rand()/(float)RAND_MAX;

    int lvl = 0;

    // Toss until failure or max level reached
    while(toss < sl->p && lvl < sl->maxlvl){
        lvl++;
        toss = (float)rand()/(float)RAND_MAX;
    }

    return lvl;
}

// Insert new node to skiplist
void skipInsert(Skiplist sl, char *key, void *item){

    // Current node
    skipNode curr = sl->dummy; 

    // Directory that keeps track of each level
    skipNode dir[sl->maxlvl+1];
    memset(dir, 0, sizeof(skipNode)*(sl->maxlvl+1));

    for(int i = sl->lvl; i >= 0; i--){
        while(curr->forward[i] != NULL && strcmp(curr->forward[i]->key, key) < 0){
            curr = curr->forward[i]; 
        }
        dir[i] = curr;
    }

    // Check next node of level 0
    curr = curr->forward[0];

    // If end of level, or before a different node then it doesn't exist
    if(curr == NULL || strcmp(curr->key, key)){

        // Start coin tossing to determine node level
        int rand_level = coinToss(sl);

        if(rand_level > sl->lvl){
            for(int i = sl->lvl+1; i <= rand_level; i++){
                dir[i] = sl->dummy;
            }

            sl->lvl = rand_level;
        }

        // New node
        skipNode node = newSkipNode(key, item, rand_level);

        // Update directory and forward nodes
        for(int i = 0; i <= rand_level; i++){
            node->forward[i] = dir[i]->forward[i];
            dir[i]->forward[i] = node;
        }
    }else{
        fprintf(stderr, "Node with key %s already exists in skiplist\n", key);
    }
}

// Check if node exists in skiplist
int skipExists(Skiplist sl, char *key){

    // Current node
    skipNode curr = sl->dummy; 

    // Directory that keeps track of each level
    skipNode dir[sl->maxlvl+1];
    memset(dir, 0, sizeof(skipNode)*(sl->maxlvl+1));

    // Set every new level to dummy and update level variable
    for(int i = sl->lvl; i >= 0; i--){
        while(curr->forward[i] != NULL && strcmp(curr->forward[i]->key, key) < 0){
            curr = curr->forward[i]; 
        }
        dir[i] = curr;
    }

    // Check next node of level 0
    curr = curr->forward[0];

    // If end of level, or before a different node then it doesn't exist
    return !(curr == NULL || strcmp(curr->key, key));
}

// Get item in skiplist
void *skipGet(Skiplist sl, char *key){

    // Current node
    skipNode curr = sl->dummy; 

    // Directory that keeps track of each level
    skipNode dir[sl->maxlvl+1];
    memset(dir, 0, sizeof(skipNode)*(sl->maxlvl+1));

    // Set every new level to dummy and update level variable
    for(int i = sl->lvl; i >= 0; i--){
        while(curr->forward[i] != NULL && strcmp(curr->forward[i]->key, key) < 0){
            curr = curr->forward[i]; 
        }
        dir[i] = curr;
    }

    // Check next node of level 0
    curr = curr->forward[0];

    // If end of level, or before a different node then it doesn't exist
    if(!(curr == NULL || strcmp(curr->key, key))){
        return curr->item;
    }else{
        return NULL;
    }
}

// Deletes node from skiplist
void skipDelete(Skiplist sl, char *key){

    // Current node
    skipNode curr = sl->dummy; 

    // Directory that keeps track of each level
    skipNode dir[sl->maxlvl+1];
    memset(dir, 0, sizeof(skipNode)*(sl->maxlvl+1));

    for(int i = sl->lvl; i >= 0; i--){
        while(curr->forward[i] != NULL && strcmp(curr->forward[i]->key, key) < 0){
            curr = curr->forward[i]; 
        }
        dir[i] = curr;
    }

    // Check next node of level 0
    curr = curr->forward[0];

    // If current is not null and has same key, then delete it
    if(curr != NULL && strcmp(curr->key, key) == 0){

        // Update forward nodes and free
        for(int i = 0; i <= sl->lvl; i++){
            if(dir[i]->forward[i] == curr){
                dir[i]->forward[i] = curr->forward[i];
            }
        }
        destroySkipNode(curr);
        free(curr);
    }else{
        fprintf(stderr, "Node with key %s doesn't exist in skiplist\n", key);
    }
}

// Destroys skiplist
void skipDestroy(Skiplist sl){

    // Traverse bottom level and delete every node
    skipNode curr = sl->dummy;
    while(curr != NULL){
        skipNode next = curr->forward[0];
        destroySkipNode(curr);
        free(curr);
        curr = next;
    }
}

// Prints skiplist
void skipPrint(Skiplist sl){

    /*
     * Format:
     *
     * Ln  : a0 -> a1 -> ... -> an
     * Ln-1: b0 -> b1 -> ... -> bn
     *             ...
     * L0  : c0 -> c1 -> ... -> cn
     *
    */

    for(int i = sl->lvl; i >= 0; i--){
        skipNode node = sl->dummy->forward[i];
        
        while(node != NULL){
            printf("%s", node->key);
            node = node->forward[i];

            if(node != NULL){
                printf("-> ");
            }
        }
        printf("\n");
    }

    printf("\n");
}
