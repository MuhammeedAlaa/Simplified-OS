#include "headers.h"

int main()
{
    key_t key_id;
    union Semun semun;
    int send_val, rec_val, shmid, producerQIndex, consumerQIndex, muxLock, full, empty;

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
    validate(CREATION, producerQIndex | consumerQIndex | shmid | muxLock | full | empty);

    shmctl(shmid, IPC_RMID, (struct shmid_ds *)0);
    shmctl(consumerQIndex, IPC_RMID, (struct shmid_ds *)0);
    shmctl(producerQIndex, IPC_RMID, (struct shmid_ds *)0);
    semctl(muxLock, 0, IPC_RMID, semun);
    semctl(full, 0, IPC_RMID, semun);
    semctl(empty, 0, IPC_RMID, semun);
    return 0;
}

