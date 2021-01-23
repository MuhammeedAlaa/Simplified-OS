#include "headers.h"

void produce(int producerIndex, int consumerIndex, int shmid, int val);

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

    // values to be produced by the producer in the buffer
    int i = 1;

    while (1)
    {
        down(empty);
        down(muxLock);
        produce(producerIndex, consumerIndex, shmid, i);
        displaySharedBuffer(shmid);
        up(muxLock);
        up(full);
        i++;
        sleep(1);
    }
    return 0;
}

/**
 * function to produce an item and place it in the buffer
 * @param producerIndex the id of the shared memory containing the Rear pointer of the circular queue (Producer Index)
 * @param consumerIndex the id of the shared memory containing the Front pointer of the circular queue (Consumer Index)
 * @param shmid the id of the shared memory containing the buffer
 * @param val value to be inserted in the buffer 
 */
void produce(int producerIndex, int consumerIndex, int shmid, int val)
{
     // A) get front and rear pointers
    int front = readSharedMemory(consumerIndex, 0, false);
    int rear = readSharedMemory(producerIndex, 0, false);

    // B) modify the pointers for the buffer (circular queue) accordingly
    if (front == -1)
    {
        front = 0;
        writeSharedMemory(consumerIndex, 0, front);
    }
    rear = (rear + 1) % BUFFER_SIZE;
    writeSharedMemory(producerIndex, 0, rear);


    // C) write the produced value to the shared memory and display it
    writeSharedMemory(shmid, rear, val);
    printf("Producer(%d) produced: %i at index %i\n", getpid(), val, rear);
}
