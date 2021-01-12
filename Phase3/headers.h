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

const int BUFFER_SIZE = 2;

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

/***
 *       _____                                 _                            
 *      / ____|                               | |                           
 *     | (___    ___  _ __ ___    __ _  _ __  | |__    ___   _ __  ___  ___ 
 *      \___ \  / _ \| '_ ` _ \  / _` || '_ \ | '_ \  / _ \ | '__|/ _ \/ __|
 *      ____) ||  __/| | | | | || (_| || |_) || | | || (_) || |  |  __/\__ \
 *     |_____/  \___||_| |_| |_| \__,_|| .__/ |_| |_| \___/ |_|   \___||___/
 *                                     | |                                  
 *                                     |_|                                  
 */

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








/***
 *     __      __     _  _      _         _    _                    
 *     \ \    / /    | |(_)    | |       | |  (_)                   
 *      \ \  / /__ _ | | _   __| |  __ _ | |_  _   ___   _ __   ___ 
 *       \ \/ // _` || || | / _` | / _` || __|| | / _ \ | '_ \ / __|
 *        \  /| (_| || || || (_| || (_| || |_ | || (_) || | | |\__ \
 *         \/  \__,_||_||_| \__,_| \__,_| \__||_| \___/ |_| |_||___/
 *                                                                  
 *                                                                  
 */

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

