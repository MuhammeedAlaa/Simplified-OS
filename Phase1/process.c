#include "headers.h"

/* Modify this file as needed*/
int remainingtime;
int semid;

void down(int sem);

int main(int agrc, char * argv[])
{
    key_t key_id = ftok("keyfile", SEM_PROCESS_KEY);
    semid = semget(key_id, 1, 0666 | IPC_CREAT);

    key_id = ftok("keyfile", MSG_PROC_UP_KEY);
    int proc_msgqup_id = msgget(key_id, 0666 | IPC_CREAT);
    key_id = ftok("keyfile", MSG_PROC_DOWN_KEY);
    int proc_msgqdown_id = msgget(key_id, 0666 | IPC_CREAT);

    struct msgProcessTimeBuff msg;
    printf("process pid = %d \n", getpid());
    int rec_val = msgrcv(proc_msgqdown_id, &msg, sizeof(msg.remainingTime), getpid(), !IPC_NOWAIT);
    if (rec_val == -1)
    {
        perror("Error in recieving remaining time from schedular\n");
    }
    
    remainingtime = msg.remainingTime;
    printf("after message receive rem time = %d\n", remainingtime);

    initClk();
    
    //TODO it needs to get the remaining time from somewhere
    //remainingtime = ??;
    while (remainingtime > 0)
    {
        printf("current time seen in the process %d id before semaphore%d\n", getpid(), getClk());
        down(semid);
        printf("current time seen in the process %d id after semaphore%d\n", getpid(), getClk());
        remainingtime--;
        msg.remainingTime = remainingtime;
        msg.mtype = getpid();
        int send_val = msgsnd(proc_msgqup_id, &msg, sizeof(msg.remainingTime), !IPC_NOWAIT);
        if (send_val == -1)
        {
            perror("Error in sending remaining time from process\n");
        }
    }
    
    printf("process %d sent to destory clk\n", getpid());
    destroyClk(false);
    printf("process %d terminated successfully\n", getpid());
    return 0;
}

void down(int sem)
{
    struct sembuf p_op;

    p_op.sem_num = 0;
    p_op.sem_op = -1;
    p_op.sem_flg = !IPC_NOWAIT;
    if (semop(sem, &p_op, 1) == -1)
    {
        perror("error in down");
    }
}
