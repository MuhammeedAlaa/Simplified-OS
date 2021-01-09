#include <stdio.h> //if you don't use scanf/printf change this include
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/msg.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>

typedef short bool;
#define true 1
#define false 0

#define SHKEY 300
#define SEM1KEY 'B'
#define SEM_PROCESS_KEY 'A'
#define MSG_SCHED_KEY 'H'
#define MSG_PROC_DOWN_KEY 'D'
#define MSG_PROC_UP_KEY 'U'

enum Algorithms
{
    HPF,
    SRTN,
    RR
};

struct processInfo
{
    int id;
    int arrivalTime;
    int runTime;
    int priority;
    bool isRunning;
    int pid;
    int remainingTime;
    int finishTime;
    int startTime;
};

struct msgbuff
{
    long mtype;
    int numberOfProcesses;
    struct processInfo p_info;
};

struct msgAlgorithm
{
    long mtype;
    int algorithm;
    int opts;
};

struct msgProcessTimeBuff
{
    long mtype;
    int remainingTime;
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

///==============================
//don't mess with this variable//
int *shmaddr; //
//===============================

int getClk()
{
    return *shmaddr;
}

/*
 * All process call this function at the beginning to establish communication between them and the clock module.
 * Again, remember that the clock is only emulation!
*/
void initClk()
{
    int shmid = shmget(SHKEY, 4, 0444);
    while ((int)shmid == -1)
    {
        //Make sure that the clock exists
        printf("Wait! The clock not initialized yet!\n");
        sleep(1);
        shmid = shmget(SHKEY, 4, 0444);
    }
    shmaddr = (int *)shmat(shmid, (void *)0, 0);
}

/*
 * All process call this function at the end to release the communication
 * resources between them and the clock module.
 * Again, Remember that the clock is only emulation!
 * Input: terminateAll: a flag to indicate whether that this is the end of simulation.
 *                      It terminates the whole system and releases resources.
*/

void destroyClk(bool terminateAll)
{
    shmdt(shmaddr);
    if (terminateAll)
    {
        killpg(getpgrp(), SIGINT);
    }
}
