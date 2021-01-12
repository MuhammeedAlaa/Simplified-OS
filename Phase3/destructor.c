#include "headers.h"

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
    validate(CREATION, up_msgq_id | down_msgq_id | producerQIndex | consumerQIndex | shmid | muxLock | full | empty);

    shmctl(shmid, IPC_RMID, (struct shmid_ds *)0);
    shmctl(consumerQIndex, IPC_RMID, (struct shmid_ds *)0);
    shmctl(producerQIndex, IPC_RMID, (struct shmid_ds *)0);
    semctl(muxLock, 0, IPC_RMID, semun);
    semctl(full, 0, IPC_RMID, semun);
    semctl(empty, 0, IPC_RMID, semun);
    msgctl(up_msgq_id, IPC_RMID, (struct msqid_ds *)0);
    msgctl(down_msgq_id, IPC_RMID, (struct msqid_ds *)0);
    return 0;
}

