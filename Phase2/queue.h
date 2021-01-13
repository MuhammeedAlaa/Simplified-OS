#ifndef Q_H
#define Q_H

#include <stdio.h>
#include <stdlib.h>

typedef struct qNode
{
    int data;
    struct qNode *next;
} qNode;
typedef struct queue
{
    struct qNode *front, *rear;
    int size;
} queue;
qNode *newNode(int d);
queue initQueue();
void popQueue(queue *q);
int front(queue *q);
int rear(queue *q);
void pushQueue(queue *Q, int data);
int isEmptyQueue(queue *q);

#endif
