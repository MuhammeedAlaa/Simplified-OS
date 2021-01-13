#include "headers.h"

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
    printf("Consumed : %i at index %i\n", val, index);
}







