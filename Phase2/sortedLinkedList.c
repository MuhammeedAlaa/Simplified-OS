#include "sortedLinkedList.h"

struct linkedList *creatLinkedList()
{
    struct linkedList *ll = (struct linkedList *)malloc(sizeof(struct linkedList));
    ll->head = NULL;
    ll->size = 0;
    return ll;
}

void InsertOrdered(struct linkedList *ll, int data)
{
    NODE *newnode;
    newnode = (NODE *)malloc(sizeof(NODE));
    newnode->data = data;
    // printf("newNode data : %d", newnode->data);
    // if it is the first node
    if (ll->size == 0)
    {
        ll->head = newnode;
        ll->head->next = NULL;
        ll->size++;
        return;
    }

    NODE *head = ll->head;
    NODE *previous = NULL;
    NODE *current = head;
    while (current != NULL && data > current->data)
    {
        previous = current;
        current = current->next;
    }
    // then the new element should be the head
    if (previous == NULL)
    {
        ll->head = newnode;
        newnode->next = current;
    }
    else
    {
        previous->next = newnode;
        newnode->next = current;
    }
    ll->size++;
}

int DeleteNode(struct linkedList *ll, int data)
{
    if (ll->size == 0)
    {
        return 0;
    }
    NODE *head = ll->head;
    NODE *previous = NULL;
    NODE *current = head;
    while (current != NULL && current->data != data)
    {
        previous = current;
        current = current->next;
    }
    if (current != NULL) /* if list empty or data not found */
    {
        if (current == head)
        {
            ll->head = current->next;
        }
        else
        {
            previous->next = current->next;
        }
        free(current);
        ll->size--;
        if (ll->size == 0)
        {
            ll->head = NULL;
        }
        return 1;
    }
    else
        return 0;
}

void Traverse(struct linkedList *ll)
{
    NODE *head = ll->head;
    // printf("head data : %d", head->data);
    NODE *current = head;
    while (current != NULL)
    {
        printf(" (%d) - ", current->data);
        current = current->next;
    }
}

int findNode(struct linkedList *ll, int data)
{
    if (ll->size == 0)
    {
        return 0;
    }
    NODE *head = ll->head;
    NODE *previous = NULL;
    NODE *current = head;
    while (current != NULL && current->data != data)
    {
        previous = current;
        current = current->next;
    }
    if (current != NULL)
        return 1;
    else
        return 0;
}

int isempty(struct linkedList *ll)
{
    return (ll->size == 0);
}