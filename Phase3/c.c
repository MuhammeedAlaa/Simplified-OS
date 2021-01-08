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
    EMPTY_SEM_INIT,
    EMPTY_SEM_GET_VAL,
    EMPTY_SEM_SET_VAL,
    FULL_SEM_INIT,
    FULL_SEM_GET_VAL,
    FULL_SEM_SET_VAL,
    MUTEX_SEM_SET_VAL,
    MSG_RECEIVE,
    MSG_SEND,
    SHM_ATTCH,
    SEM_DOWN,
    SEM_UP
};

struct msgbuff
{
    long mtype;
    char mtext[1];
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

void reader(int shmid, int intMsg, int index);
void up(int sem);
void down(int sem);
void handler(int signum);
void validate(int errorNumber, int checkVar);
key_t key_id;
union Semun semun;
int up_msgq_id, down_msgq_id, send_val, rec_val, shmid, muxLock, full, empty;

int main()
{
    signal(SIGINT, handler);

    key_id = ftok("keyfile", 65);
    up_msgq_id = msgget(key_id, 0666 | IPC_CREAT);
    // key_id = ftok("keyfile", 66);
    // down_msgq_id = msgget(key_id, 0666 | IPC_CREAT);

    key_id = ftok("keyfile", 67);
    shmid = shmget(key_id, BUFFER_SIZE * sizeof(int), 0666 | IPC_CREAT);

    key_id = ftok("keyfile", 68);
    muxLock = semget(key_id, 1, 0666 | IPC_CREAT);

    key_id = ftok("keyfile", 69);
    full = semget(key_id, 1, 0666 | IPC_CREAT);

    key_id = ftok("keyfile", 70);
    empty = semget(key_id, 1, 0666 | IPC_CREAT);

    // check for any creation error
    validate(CREATION, up_msgq_id | shmid | muxLock | full | empty);

    int buffer_pointer = 0;
    int i = 1;
    while (1)
    {
        down(full);
        down(muxLock);

        int emptyCount = semctl(empty, 0, GETVAL, semun);
        validate(EMPTY_SEM_GET_VAL, emptyCount);
        if (emptyCount == BUFFER_SIZE)
        {
            struct msgbuff message;
            rec_val = msgrcv(up_msgq_id, &message, sizeof(message.mtext), 0, !IPC_NOWAIT);
            validate(MSG_RECEIVE, rec_val);
        }

        reader(shmid, i, buffer_pointer);
        buffer_pointer = (buffer_pointer + 1) % BUFFER_SIZE;

        int fullCount = semctl(full, 0, GETVAL, semun);
        validate(FULL_SEM_GET_VAL, fullCount);
        if (fullCount == BUFFER_SIZE)
        {
            struct msgbuff message;
            send_val = msgsnd(up_msgq_id, &message, sizeof(message.mtext), !IPC_NOWAIT);
            validate(MSG_SEND, send_val);
        }

        // sleep(1);
        up(muxLock);
        up(empty);
        i++;
    }
    return 0;
}

void reader(int shmid, int intMsg, int index)
{
    void *shmaddr = shmat(shmid, (void *)0, 0);
    validate(SHM_ATTCH, shmaddr == (char *)-1 ? -1 : 0);
    intMsg = *((int *)shmaddr + index * sizeof(int));
    printf("Consumed : %i\n", intMsg);
    shmdt(shmaddr);
}

void down(int sem)
{
    struct sembuf p_op;

    p_op.sem_num = 0;
    p_op.sem_op = -1;
    p_op.sem_flg = !IPC_NOWAIT;
    validate(SEM_DOWN, semop(sem, &p_op, 1));
}

void up(int sem)
{
    struct sembuf v_op;

    v_op.sem_num = 0;
    v_op.sem_op = 1;
    v_op.sem_flg = !IPC_NOWAIT;
    validate(SEM_UP, semop(sem, &v_op, 1));
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
    case EMPTY_SEM_INIT:
        perror("error in initializing empty semaphore.\n");
        break;
    case EMPTY_SEM_GET_VAL:
        perror("error in getting empty semaphore value.\n");
        break;
    case FULL_SEM_INIT:
        perror("error in initializing full semaphore.\n");
        break;
    case FULL_SEM_GET_VAL:
        perror("error in getting full semaphore value.\n");
        break;
    case MSG_RECEIVE:
        perror("error in receiving messege.\n");
        break;
    case MSG_SEND:
        perror("error in sending messege.\n");
        break;
    case SHM_ATTCH:
        perror("error in attaching shared memeory.\n");
        break;
    case SEM_DOWN:
        perror("error in downing the semaphore value.\n");
        break;
    case SEM_UP:
        perror("error in uping the semaphore value.\n");
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

void handler(int signum)
{
    shmctl(shmid, IPC_RMID, (struct shmid_ds *)0);
    semctl(muxLock, 0, IPC_RMID, semun);
    semctl(full, 0, IPC_RMID, semun);
    semctl(empty, 0, IPC_RMID, semun);
    msgctl(up_msgq_id, IPC_RMID, (struct msqid_ds *)0);
    //msgctl(down_msgq_id, IPC_RMID, (struct msqid_ds *)0);
    exit(0);
}
