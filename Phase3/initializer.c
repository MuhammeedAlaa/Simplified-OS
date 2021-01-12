#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <signal.h>

const int BUFFER_SIZE = 20;
enum error_types
{
    CREATION,
    EMPTY_SEM_SET_VAL,
    FULL_SEM_SET_VAL,
    MUTEX_SEM_SET_VAL,
    SHM_ATTCH,
};

/* arg for semctl system calls. */
union Semun
{
    int val;               /* value for SETVAL */
    struct semid_ds *buf;  /* buffer for IPC_STAT & IPC_SET */
    ushort *array;         /* array for GETALL & SETALL */
    struct seminfo *__buf; /* buffer for IPC_INFO */
    void *__pad;
};

void validate(int errorNumber, int checkVar);
void indexInit(int shmid);

int main()
{
    key_t key_id;
    union Semun semun;
    int up_msgq_id, down_msgq_id, send_val, rec_val, shmid, producerQIndex, consumerQIndex, muxLock, full, empty;

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
    validate(CREATION, up_msgq_id | shmid | muxLock | full | empty);

    semun.val = 1;
    validate(MUTEX_SEM_SET_VAL, semctl(muxLock, 0, SETVAL, semun));

    semun.val = 0;
    validate(FULL_SEM_SET_VAL, semctl(full, 0, SETVAL, semun));

    semun.val = BUFFER_SIZE;
    validate(EMPTY_SEM_SET_VAL, semctl(empty, 0, SETVAL, semun));

    indexInit(consumerQIndex);
    indexInit(producerQIndex);
    return 0;
}

void indexInit(int shmid)
{
    void *shmaddr = shmat(shmid, (void *)0, 0);
    validate(SHM_ATTCH, shmaddr == (char *)-1 ? -1 : 0);
    *(int *)shmaddr = -1;
    shmdt(shmaddr);
}

void validate(int errorNumber, int checkVar)
{
    if (checkVar != -1)
        return;

    switch (errorNumber)
    {
    case CREATION:
        perror("error in creation.\n");
        break;

    case SHM_ATTCH:
        perror("error in attaching shared memeory.\n");
        break;
    case FULL_SEM_SET_VAL:
        perror("error in setting the full value.\n");
        break;
    case EMPTY_SEM_SET_VAL:
        perror("error in setting the empty value.\n");
        break;
    case MUTEX_SEM_SET_VAL:
        perror("error in setting the mutex value.\n");
        break;
    default:
        perror("unhandled error occured.\n");
        break;
    }
    exit(-1);
}
