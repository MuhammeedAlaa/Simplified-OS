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

typedef short bool;
#define true 1
#define false 0

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
int readIndex(int shmid);
void writeIndex(int shmid, int val);
void showMem(int shmid);
void up(int sem);
void down(int sem);
void validate(int errorNumber, int checkVar);
bool isFull(int shmidP, int shmidC);
bool isEmpty(int shmid);
void consume(int consumerQIndex, int producerQIndex, int shmid, int i);

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

    int i = 1;
    while (1)
    {
        down(muxLock);
        if (isEmpty(consumerQIndex))
        {
            struct msgbuff message;
            rec_val = msgrcv(up_msgq_id, &message, sizeof(message.mtext), 0, !IPC_NOWAIT);
            validate(MSG_RECEIVE, rec_val);
            consume(consumerQIndex, producerQIndex, shmid, i);
        }
        else if (isFull(producerQIndex, consumerQIndex))
        {
            consume(consumerQIndex, producerQIndex, shmid, i);

            struct msgbuff message;
            send_val = msgsnd(up_msgq_id, &message, sizeof(message.mtext), !IPC_NOWAIT);
            validate(MSG_SEND, send_val);
        }
        else
        {
            consume(consumerQIndex, producerQIndex, shmid, i);
        }

        sleep(2);
        up(muxLock);
        i++;
        showMem(shmid);
    }
    return 0;
}

void consume(int consumerQIndex, int producerQIndex, int shmid, int i)
{
    int front = readIndex(consumerQIndex);
    int rear = readIndex(producerQIndex);
    reader(shmid, i, front);
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
void reader(int shmid, int intMsg, int index)
{
    void *shmaddr = shmat(shmid, (void *)0, 0);
    validate(SHM_ATTCH, shmaddr == (char *)-1 ? -1 : 0);
    intMsg = *((int *)shmaddr + index * sizeof(int));
    *((int *)shmaddr + index * sizeof(int)) = 0;
    printf("Consumed : %i at index %i\n", intMsg, index);
    shmdt(shmaddr);
}

bool isFull(int shmidP, int shmidC)
{

    void *shmaddr = shmat(shmidP, (void *)0, 0);
    int front = *(int *)shmaddr;
    shmdt(shmaddr);

    shmaddr = shmat(shmidC, (void *)0, 0);
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

bool isEmpty(int shmid)
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
void showMem(int shmid)
{
    void *shmaddr = shmat(shmid, (void *)0, 0);
    for (int i = 0; i < BUFFER_SIZE; i++)
        printf(" %i", *((int *)shmaddr + i * sizeof(int)));
    printf("\n");
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