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

void writer(int shmid, int intMsg, int index);
void up(int sem);
void down(int sem);
void handler(int signum);
key_t key_id;
union Semun semun;
int up_msgq_id, down_msgq_id, send_val, rec_val, shmid, muxLock, full, empty;
int main()
{

    key_id = ftok("keyfile", 65);
    up_msgq_id = msgget(key_id, 0666 | IPC_CREAT);
    key_id = ftok("keyfile", 66);
    down_msgq_id = msgget(key_id, 0666 | IPC_CREAT);

    key_id = ftok("keyfile", 67);
    shmid = shmget(key_id, BUFFER_SIZE * sizeof(int), 0666 | IPC_CREAT);

    key_id = ftok("keyfile", 68);
    muxLock = semget(key_id, 1, 0666 | IPC_CREAT);

    key_id = ftok("keyfile", 69);
    full = semget(key_id, 1, 0666 | IPC_CREAT);

    key_id = ftok("keyfile", 70);
    empty = semget(key_id, 1, 0666 | IPC_CREAT);
    signal(SIGINT, handler);

    if (up_msgq_id == -1 || down_msgq_id == -1 || shmid == -1 || muxLock == -1 || full == -1 || empty == -1)
    {
        perror("Error in create");
        exit(-1);
    }
    semun.val = 1;
    if (semctl(muxLock, 0, SETVAL, semun) == -1)
    {
        perror("Error in semctl muxlock");
        exit(-1);
    }
    semun.val = 0;
    if (semctl(full, 0, SETVAL, semun) == -1)
    {
        perror("Error in semctl full");
        exit(-1);
    }
    semun.val = BUFFER_SIZE;
    if (semctl(empty, 0, SETVAL, semun) == -1)
    {
        perror("Error in semctl empty");
        exit(-1);
    }

    printf("Up Message Queue ID = %d\n", up_msgq_id);
    printf("Down Message Queue ID = %d\n", down_msgq_id);
    printf("\nShared memory ID = %d\n", shmid);
    printf("\nSemaphore ID = %d\n", muxLock);
    int buffer_pointer = 0;

    for (int i = 1; i <= 4 * BUFFER_SIZE; i++)
    {
        /* Insert into buffer */
        down(empty);
        down(muxLock);

        writer(shmid, i, buffer_pointer);
        buffer_pointer = (buffer_pointer + 1) % BUFFER_SIZE;
        int fullCount = semctl(full, 0, GETVAL, semun);
        if (fullCount == -1)
        {
            perror("Error in semctl full count");
            exit(-1);
        }
        else if (fullCount == BUFFER_SIZE)
        {
            struct msgbuff message;
            rec_val = msgrcv(down_msgq_id, &message, sizeof(message.mtext), 0, !IPC_NOWAIT);
            if (rec_val == -1)
            {
                perror("Error in receive");
                exit(-1);
            }
        }
        // printf("full count : %i\n", fullCount);

        int emptyCount = semctl(empty, 0, GETVAL, semun);
        if (emptyCount == -1)
        {
            perror("Error in semctl empty count");
            exit(-1);
        }
        else if (emptyCount == BUFFER_SIZE)
        {
            struct msgbuff message;
            send_val = msgsnd(up_msgq_id, &message, sizeof(message.mtext), !IPC_NOWAIT);
            if (send_val == -1)
            {
                perror("Errror in send");
                exit(-1);
            }
        }
        // printf("empty count : %i\n", emptyCount);
        up(muxLock);
        up(full);
    }
    // kill(getpid(), SIGINT);
    return 0;
}

void writer(int shmid, int intMsg, int index)
{
    void *shmaddr = shmat(shmid, (void *)0, 0);
    if (shmaddr == NULL)
    {
        perror("Error in attach in writer");
        exit(-1);
    }
    printf("Produced : %i\n", intMsg);
    *((int *)shmaddr + index * sizeof(int)) = intMsg;
    shmdt(shmaddr);
}

void down(int sem)
{
    struct sembuf p_op;

    p_op.sem_num = 0;
    p_op.sem_op = -1;
    p_op.sem_flg = !IPC_NOWAIT;
    if (semop(sem, &p_op, 1) == -1)
    {
        perror("Error in down()");
        exit(-1);
    }
}

void up(int sem)
{
    struct sembuf v_op;

    v_op.sem_num = 0;
    v_op.sem_op = 1;
    v_op.sem_flg = !IPC_NOWAIT;

    if (semop(sem, &v_op, 1) == -1)
    {
        perror("Error in up()");
        exit(-1);
    }
}
void handler(int signum)
{
    shmctl(shmid, IPC_RMID, (struct shmid_ds *)0);
    semctl(muxLock, 0, IPC_RMID, semun);
    semctl(full, 0, IPC_RMID, semun);
    semctl(empty, 0, IPC_RMID, semun);
    msgctl(up_msgq_id, IPC_RMID, (struct msqid_ds *)0);
    msgctl(down_msgq_id, IPC_RMID, (struct msqid_ds *)0);
    exit(0);
}
