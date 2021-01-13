#include "headers.h"

void produce(int consumerQIndex, int producerQIndex, int shmid, int i);

int main()
{

    key_t key_id;
    union Semun semun;
    int up_msgq_id, down_msgq_id, send_val, rec_val, producerQIndex, consumerQIndex, shmid, muxLock, full, empty;
    
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

    int buffer_pointer;
    int i = 1;
    while (1)
    {
        down(empty);
        down(muxLock);
        /* Insert into buffer */
        printf("front is %d and rear is %d \n", readIndex(consumerQIndex), readIndex(producerQIndex));
        // if (isFull(producerQIndex, consumerQIndex))
        // {
        //     printf("full produce\n");
        //     struct msgbuff message;
        //     rec_val = msgrcv(up_msgq_id, &message, sizeof(message.mtext), 0, !IPC_NOWAIT);
        //     validate(MSG_RECEIVE, rec_val);
        //     produce(consumerQIndex, producerQIndex, shmid, i);
        // }
        // else if (isEmpty(consumerQIndex))
        // {
        //     printf("empty produce\n");
        //     produce(consumerQIndex, producerQIndex, shmid, i);
        //     struct msgbuff message;
        //     send_val = msgsnd(down_msgq_id, &message, sizeof(message.mtext), !IPC_NOWAIT);
        //     validate(MSG_SEND, send_val);
        // }
        // else
        // {
            printf("normal produce\n");
            produce(consumerQIndex, producerQIndex, shmid, i);
        // }

        showMem(shmid);
        i++;
        up(muxLock);
        up(full);
        // printf("front is %d and rear is %d \n", readIndex(consumerQIndex), readIndex(producerQIndex));
        printf("```````````````````````````````````\n");
        usleep(1000000);
    }
    return 0;
}

void produce(int consumerQIndex, int producerQIndex, int shmid, int i)
{
    int front = readIndex(consumerQIndex);
    int rear = readIndex(producerQIndex);
    if (front == -1)
    {
        front = 0;
        writeIndex(consumerQIndex, front);
    }
    rear = (rear + 1) % BUFFER_SIZE;
    writeIndex(producerQIndex, rear);
    writer(shmid, i, rear);
    printf("Produced : %i at index %i\n", offset, index);
}
