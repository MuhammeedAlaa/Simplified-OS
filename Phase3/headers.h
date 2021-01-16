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

const int BUFFER_SIZE = 3;

typedef short bool;
#define true 1
#define false 0

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

/**
 * function to perform a down operation on a semaphore providing its id
 * @param semid the id of the semaphore to be modified
 */
void down(int semid)
{
    struct sembuf p_op;

    p_op.sem_num = 0;
    p_op.sem_op = -1;
    p_op.sem_flg = !IPC_NOWAIT;
    validate(SEM_DOWN, semop(semid, &p_op, 1));
}

/**
 * function to perform an up operation on a semaphore providing its id
 * @param semid the id of the semaphore to be modified
 */
void up(int semid)
{
    struct sembuf v_op;

    v_op.sem_num = 0;
    v_op.sem_op = 1;
    v_op.sem_flg = !IPC_NOWAIT;
    validate(SEM_UP, semop(semid, &v_op, 1));
}

/***
 *       _____  _                            _   __  __                                      
 *      / ____|| |                          | | |  \/  |                                     
 *     | (___  | |__    __ _  _ __  ___   __| | | \  / |  ___  _ __ ___    ___   _ __  _   _ 
 *      \___ \ | '_ \  / _` || '__|/ _ \ / _` | | |\/| | / _ \| '_ ` _ \  / _ \ | '__|| | | |
 *      ____) || | | || (_| || |  |  __/| (_| | | |  | ||  __/| | | | | || (_) || |   | |_| |
 *     |_____/ |_| |_| \__,_||_|   \___| \__,_| |_|  |_| \___||_| |_| |_| \___/ |_|    \__, |
 *                                                                                      __/ |
 *                                                                                     |___/ 
 */


/**
 * function to read the integer content of a shared memory providing its Shmid 
 * @param shmid the id of the shared memory to be read
 * @param offset the offset of the place to be read from the provided shared memory
 * @param destroyCurrentContent it replaces the content of the shared memory with zero
 * @returns the value read from the shared memory
 */
int readSharedMemory(int shmid, int offset, bool destoryCurrentContent)
{
  // attach and validate
  void *shmaddr = shmat(shmid, (void *)0, 0);
  validate(SHM_ATTCH, shmaddr == (char *)-1 ? -1 : 0);

  // get the value in the shared memory with offset
  int val = *((int *)shmaddr + offset * sizeof(int));
  if(destoryCurrentContent)
    *((int *)shmaddr + offset * sizeof(int)) = 0;
  
  // detach the shared memory
  shmdt(shmaddr);
  return val;
}

/**
 * function to write an integer value to a shared memory providing its Shmid 
 * @param shmid the id of the shared memory to be read
 * @param offset the offset of the place to be read from the provided shared memory
 * @param val the value to be written in the shared memory
 */
void writeSharedMemory(int shmid, int offset, int val)
{
  // attach and validate
  void *shmaddr = shmat(shmid, (void *)0, 0);
  validate(SHM_ATTCH, shmaddr == (char *)-1 ? -1 : 0);

  // write the new value and detach
  *((int *)shmaddr + offset * sizeof(int)) = val;
  shmdt(shmaddr);
}


/***
 *      ____           __   __               _____  _          _               
 *     |  _ \         / _| / _|             / ____|| |        | |              
 *     | |_) | _   _ | |_ | |_  ___  _ __  | (___  | |_  __ _ | |_  _   _  ___ 
 *     |  _ < | | | ||  _||  _|/ _ \| '__|  \___ \ | __|/ _` || __|| | | |/ __|
 *     | |_) || |_| || |  | | |  __/| |     ____) || |_| (_| || |_ | |_| |\__ \
 *     |____/  \__,_||_|  |_|  \___||_|    |_____/  \__|\__,_| \__| \__,_||___/
 *                                                                             
 *                                                                             
 */

// We treat the shared buffer as a circular queue where the rear pointer points at
// the location where new data will be inserted.
// Meanwhile the front pointer points at the position of the first item in the Q

// Notes:
// Front Pointer is stored at (shared memory containing Consumer index) 
// while Rear pointer is stored at (shared memory containing Producer index) 



/**
 * function to check whether the buffer is empty
 * @param rearShmid the id of the shared memory containing the Rear pointer of the circular queue (Producer Index)
 * @param frontShmid the id of the shared memory containing the Front pointer of the circular queue (Consumer Index)
 * @returns boolean value True when the queue is empty otherwise False
 */
bool isBufferEmpty(int rearShmid, int frontShmid)
{

    int front = readSharedMemory(frontShmid, 0, false);
    int rear = readSharedMemory(rearShmid, 0, false);
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

/**
 * function to check whether the buffer is full
 * @param frontShmid the id of the shared memory containing the Front pointer of the circular queue (Consumer Index)
 * @returns boolean value True when the queue is full otherwise False
 */
bool isBufferFull(int frontShmid)
{
    int front = readSharedMemory(frontShmid, 0, false);
    if (front == -1)
    {
        return true;
    }
    else
    {
        return false;
    }
}

/**
 * function to dispaly the content of the Shared Buffer note that zero elements represent empty spaces
 * @param shmid the id of the shared memory containing buffer
 * 
 */
void displaySharedBuffer(int shmid)
{
    void *shmaddr = shmat(shmid, (void *)0, 0);
    for (int i = 0; i < BUFFER_SIZE; i++)
        printf(" %2i", *((int *)shmaddr + i * sizeof(int)));
    printf("\n=========================================\n");
    shmdt(shmaddr);
}


