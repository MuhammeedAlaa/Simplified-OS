#include "headers.h"




int main(int argc, char *argv[])
{   

    
    initClk();

    // create message queue to communicate with process generator
    int key_id = ftok("keyfile", MSG_SCHED_KEY);
    int sched_msgq_id = msgget(key_id, 0666 | IPC_CREAT);
    struct msgbuff msg;
    int curr_time;
    while (1)
    {
        int rec_val = msgrcv(sched_msgq_id, &msg, sizeof(msg.numberOfProcesses) + sizeof(msg.p_info), 0, !IPC_NOWAIT);
        if (rec_val == -1)
        {
            perror("Error in recieving from proc_gen\n");
        }
        curr_time = getClk();
        printf("Time step : %d \n", curr_time);
        printf("-----------------Schedular-------------------\n");
        printf("Process id : %d \n", msg.p_info.id);
        printf("Process arrival time : %d \n", msg.p_info.arrivalTime);
        printf("Process run time : %d \n", msg.p_info.runTime);
        printf("Process priority : %d \n", msg.p_info.priority);
    }

    //TODO implement the scheduler :)
    //upon termination release the clock resources.
    destroyClk(false);
}
