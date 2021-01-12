#include "headers.h"

void reader(int shmid, int index);
int readIndex(int shmid);
void writeIndex(int shmid, int val);
void displaySharedBuffer(int shmid);
void up(int sem);
void down(int sem);
void validate(int errorNumber, int checkVar);
bool isBufferEmpty(int shmidP, int shmidC);
bool isBufferFull(int shmid);
void consume(int consumerQIndex, int producerQIndex, int shmid, int i);

int main()
{
    key_t key_id;
    union Semun semun;
    int up_msgq_id, down_msgq_id, send_val, rec_val, shmid, producerQIndex, consumerQIndex, muxLock, full, empty;

    key_id = ftok("keyfile", 64);
    down_msgq_id = msgget(key_id, 0666 | IPC_CREAT);

    key_id = ftok("keyfile", 65);
    up_msgq_id = msgget(key_id, 0666 | IPC_CREAT);

    key_id = ftok("keyfile", 66);
    producerQIndex = shmget(key_id, sizeof(int), 0666 | IPC_CREAT);

    key_id = ftok("keyfile", 67);
    consumerQIndex = shmget(key_id, sizeof(int), 0666 | IPC_CREAT);

    key_id = ftok("keyfile", 68);
    shmid = shmget(key_id, BUFFER_SIZE * sizeof(int), 0666 | IPC_CREAT);

    key_id = ftok("keyfile", 69);
    muxLock = semget(key_id, 1, 0666 | IPC_CREAT);

    key_id = ftok("keyfile", 70);
    full = semget(key_id, 1, 0666 | IPC_CREAT);

    key_id = ftok("keyfile", 71);
    empty = semget(key_id, 1, 0666 | IPC_CREAT);

    // check for any creation error
    validate(CREATION, up_msgq_id | down_msgq_id | shmid | muxLock | full | empty);

    int i = 1;
    while (1)
    {
        down(full);
        down(muxLock);
        // printf("front is %d and rear is %d \n", readIndex(consumerQIndex), readIndex(producerQIndex));
        // if (isBufferFull(consumerQIndex))
        // {
        //     printf("consume empty\n");
        //     struct msgbuff message;
        //     rec_val = msgrcv(down_msgq_id, &message, sizeof(message.mtext), 0, !IPC_NOWAIT);
        //     validate(MSG_RECEIVE, rec_val);
        //     consume(consumerQIndex, producerQIndex, shmid, i);
        // }
        // else if (isBufferEmpty(producerQIndex, consumerQIndex))
        // {
        //     printf("consume full\n");
        //     consume(consumerQIndex, producerQIndex, shmid, i);

        //     struct msgbuff message;
        //     send_val = msgsnd(up_msgq_id, &message, sizeof(message.mtext), !IPC_NOWAIT);
        //     validate(MSG_SEND, send_val);
        // }
        // else
        // {
            printf("normal consume\n");
            consume(consumerQIndex, producerQIndex, shmid, i);
        // }
        displaySharedBuffer(shmid);
        i++;
        up(muxLock);
        up(empty);
        // printf("front is %d and rear is %d \n", readIndex(consumerQIndex), readIndex(producerQIndex));

        printf("````````````````````````````````````\n");
        usleep(1000000);
    }
    return 0;
}

void consume(int consumerQIndex, int producerQIndex, int shmid, int i)
{
    int front = readIndex(consumerQIndex);
    int rear = readIndex(producerQIndex);
    reader(shmid, front);
    if (front == rear)
    {
        writeIndex(consumerQIndex, -1);
        writeIndex(producerQIndex, -1);
    }
    else
    {
        writeIndex(consumerQIndex, (front + 1) % BUFFER_SIZE);
    }
}
void reader(int shmid, int index)
{
    void *shmaddr = shmat(shmid, (void *)0, 0);
    validate(SHM_ATTCH, shmaddr == (char *)-1 ? -1 : 0);
    int intMsg = *((int *)shmaddr + index * sizeof(int));
    *((int *)shmaddr + index * sizeof(int)) = 0;
    printf("Consumed : %i at index %i\n", intMsg, index);
    shmdt(shmaddr);
}

bool isBufferEmpty(int shmidP, int shmidC)
{

    void *shmaddr = shmat(shmidC, (void *)0, 0);
    int front = *(int *)shmaddr;
    shmdt(shmaddr);

    shmaddr = shmat(shmidP, (void *)0, 0);
    int rear = *(int *)shmaddr;
    shmdt(shmaddr);
    if (front == 0 && rear == BUFFER_SIZE - 1)
    {
        return true;
    }
    if (front == rear + 1)
    {
        return true;
    }
    return false;
}

bool isBufferFull(int shmid)
{
    void *shmaddr = shmat(shmid, (void *)0, 0);
    int front = *(int *)shmaddr;
    shmdt(shmaddr);
    if (front == -1)
    {
        return true;
    }
    else
    {
        return false;
    }
}
int readIndex(int shmid)
{
    void *shmaddr = shmat(shmid, (void *)0, 0);
    validate(SHM_ATTCH, shmaddr == (char *)-1 ? -1 : 0);
    int index = *(int *)shmaddr;
    shmdt(shmaddr);
    return index;
}

void writeIndex(int shmid, int val)
{
    void *shmaddr = shmat(shmid, (void *)0, 0);
    validate(SHM_ATTCH, shmaddr == (char *)-1 ? -1 : 0);
    *((int *)shmaddr + 0 * sizeof(int)) = val;
    shmdt(shmaddr);
}
void displaySharedBuffer(int shmid)
{
    void *shmaddr = shmat(shmid, (void *)0, 0);
    for (int i = 0; i < BUFFER_SIZE; i++)
        printf(" %i", *((int *)shmaddr + i * sizeof(int)));
    printf("\n");
    shmdt(shmaddr);
}


