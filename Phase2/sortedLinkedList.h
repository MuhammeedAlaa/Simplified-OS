#ifndef SORTEDLINKEDLIST_H
#define SORTEDLINKEDLIST_H

#include <stdio.h>
#include <stdlib.h>

struct nodesorted {
    int data;
    struct nodesorted *next;
};
typedef struct nodesorted NODE;
 
struct linkedList {
    NODE *head;
    int size;
};

/* linked list functions */
struct linkedList * creatLinkedList();
int isempty(struct linkedList * ll);
void InsertOrdered(struct linkedList *ll, int );
int DeleteNode(struct linkedList *ll, int );
int findNode(struct linkedList* ll, int data);
void Traverse(struct linkedList *ll);

#endif