#include "queue.h"
qNode *newNode(int d)
{
    qNode *temp = (qNode *)malloc(sizeof(qNode));
    temp->data = d;
    temp->next = NULL;
    return temp;
}

queue initQueue()
{
    queue q;
    q.front = q.rear = NULL;
    q.size = 0;
    return q;
}

void pushQueue(queue *q, int k)
{
    qNode *temp = newNode(k);
    if (q->rear == NULL)
    {
        q->front = q->rear = temp;
        q->size = q->size + 1;
        return;
    }
    q->rear->next = temp;
    q->rear = temp;
    q->size = q->size + 1;
}
void popQueue(queue *q)
{
    if (q->front == NULL)
        return;
    qNode *temp = q->front;
    q->front = q->front->next;
    if (q->front == NULL)
        q->rear = NULL;

    free(temp);
    q->size = q->size - 1;
}
int front(queue *q)
{

    if (q->front != NULL)
        return q->front->data;
    return -1;
};

int rear(queue *q)
{

    if (q->rear != NULL)
        return q->rear->data;
    return -1;
};

int isEmptyQueue(queue *q)
{
    return q->size == 0;
}