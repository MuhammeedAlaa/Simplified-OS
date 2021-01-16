#include "headers.h"

/* Modify this file as needed*/
int remainingtime = 100; // an aribitrary initialization just to enter the loop once


int main(int agrc, char *argv[])
{

    // make the needed ipcs
    key_t key_id = ftok("keyfile", MSG_PROC_UP_KEY);
    int proc_msgqup_id = msgget(key_id, 0666 | IPC_CREAT);

    key_id = ftok("keyfile", MSG_PROC_DOWN_KEY);
    int proc_msgqdown_id = msgget(key_id, 0666 | IPC_CREAT);
    validate(CREATION, proc_msgqdown_id | proc_msgqup_id);

    struct msgProcessTimeBuff msg;
    
    printf("process pid = %d \n", getpid());

    initClk();

    while (remainingtime > 0)
    {
        // receive remaining time from the scheduler
        int rec_val = msgrcv(proc_msgqdown_id, &msg, sizeof(msg.remainingTime), getpid(), !IPC_NOWAIT);
        if (rec_val == -1)
        {
            perror("Error in recieving remaining time from schedular\n");
        }

        // update the remaining time
        remainingtime = msg.remainingTime;
        remainingtime--;

        // send remaining time to the scheduler
        msg.remainingTime = remainingtime;
        msg.mtype = getpid();
        int send_val = msgsnd(proc_msgqup_id, &msg, sizeof(msg.remainingTime), !IPC_NOWAIT);
        if (send_val == -1)
        {
            perror("Error in sending remaining time from process\n");
        }
    }

    destroyClk(false);
    printf("process %d terminated successfully\n", getpid());
    return 0;
}
