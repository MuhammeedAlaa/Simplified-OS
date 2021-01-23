#include "headers.h"

void consume(int producerIndex, int consumerIndex, int shmid);

int main()
{
    // create the needed IPCs
    key_t key_id = ftok("keyfile", 66);
    int producerIndex = shmget(key_id, sizeof(int), 0666 | IPC_CREAT);

    key_id = ftok("keyfile", 67);
    int consumerIndex = shmget(key_id, sizeof(int), 0666 | IPC_CREAT);

    key_id = ftok("keyfile", 68);
    int shmid = shmget(key_id, BUFFER_SIZE * sizeof(int), 0666 | IPC_CREAT);

    key_id = ftok("keyfile", 69);
    int muxLock = semget(key_id, 1, 0666 | IPC_CREAT);

    key_id = ftok("keyfile", 70);
    int full = semget(key_id, 1, 0666 | IPC_CREAT);

    key_id = ftok("keyfile", 71);
    int empty = semget(key_id, 1, 0666 | IPC_CREAT);

    // check for any creation error
    validate(CREATION, producerIndex | consumerIndex | shmid | muxLock | full | empty);

    while (1)
    {
        down(full);
        down(muxLock);
        consume(producerIndex, consumerIndex, shmid);
        displaySharedBuffer(shmid);
        up(muxLock);
        up(empty);
        sleep(1);
    }
    return 0;
}

/**
 * function to consume the item pointed at by the front pointer of the buffer
 * @param producerIndex the id of the shared memory containing the Rear pointer of the circular queue (Producer Index)
 * @param consumerIndex the id of the shared memory containing the Front pointer of the circular queue (Consumer Index)
 * @param shmid the id of the shared memory containing the buffer
 */
void consume(int producerIndex, int consumerIndex, int shmid)
{
    // A) get front and rear pointers
    int front = readSharedMemory(consumerIndex, 0, false);
    int rear = readSharedMemory(producerIndex, 0, false);

    // B) get the value pointed at by front pointer and display it
    int oldVal = readSharedMemory(shmid, front, true);
    printf("Consumer(%d) consumed: %i at index %i\n", getpid(), oldVal, front);

    // C) modify the pointers for the buffer (circular queue) accordingly
    if (front == rear)
    {
        writeSharedMemory(consumerIndex, 0, -1);
        writeSharedMemory(producerIndex, 0, -1);
    }
    else
    {
        writeSharedMemory(consumerIndex, 0, (front + 1) % BUFFER_SIZE);
    }
}







