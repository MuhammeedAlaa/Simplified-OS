#ifndef PRIORIYQ_H
#define PRIORIYQ_H

#include <stdio.h>
#include <stdlib.h>

#define LCHILD(x) 2 * x + 1
#define RCHILD(x) 2 * x + 2
#define PARENT(x) (x - 1) / 2

typedef struct node {
    int priority ;
    int data;
} node ;

typedef struct minHeap {
    int size ;
    node *elem ;
} minHeap ;

minHeap initMinHeap();
void swap(node *n1, node *n2) ;
void heapify(minHeap *hp, int i);
void push(minHeap *hp, int priority, int data);
int isEmpty(minHeap *hp);
struct node* pop(minHeap *hp);
struct node* peek(minHeap *hp);



#endif
