#include "headers.h"
#include "hashmap.h"
#include "priorityQueue.h"

int process_compare(const void *a, const void *b, void *udata)
{
    const struct processInfo *process_a = a;
    const struct processInfo *process_b = b;
    return (process_a->id == process_b->id ? 0 : 1);
}

bool process_iter(const void *item, void *udata)
{
    const struct processInfo *process = item;
    printf("process: (id=%d) (arrivalTime=%d) (runTime=%d) (priority=%d) (pid=%d) (state=%d) (remainingTime=%d) (finishTime=%d) (startTime=%d)\n", process->id, process->arrivalTime, process->runTime, process->priority, process->pid, process->isRunning, process->remainingTime, process->finishTime, process->startTime);
    return true;
}

uint64_t process_hash(const void *item, uint64_t seed0, uint64_t seed1) {
    return hashmap_murmur(item, sizeof(int), seed0, seed1);
}

int proc_msgqup_id, proc_msgqdown_id;
/* Clear the resources before exit */
void cleanup(int signum)
{
    msgctl(proc_msgqup_id, IPC_RMID, (struct msqid_ds *)0);
    msgctl(proc_msgqdown_id, IPC_RMID, (struct msqid_ds *)0);
    printf("schedular terminating!\n");
    exit(0);
}
int main(int argc, char *argv[])
{
    signal(SIGINT, cleanup);
    int algorithm, quantaMax;
    struct hashmap *processTable = hashmap_new(sizeof(struct processInfo), 0, 0, 0,
                                               process_hash, process_compare, NULL);
    // create message queue to communicate with process generator
    key_t key_id = ftok("keyfile", MSG_SCHED_KEY);
    int sched_msgq_id = msgget(key_id, 0666 | IPC_CREAT);
    key_id = ftok("keyfile", MSG_PROC_UP_KEY);
    proc_msgqup_id = msgget(key_id, 0666 | IPC_CREAT);
    key_id = ftok("keyfile", MSG_PROC_DOWN_KEY);
    proc_msgqdown_id = msgget(key_id, 0666 | IPC_CREAT);
    struct msgAlgorithm initMsg;
    struct msgProcessTimeBuff processMsg;
    int rec_val = msgrcv(sched_msgq_id, &initMsg, sizeof(initMsg.algorithm) + sizeof(initMsg.opts), 0, !IPC_NOWAIT);
    if (rec_val == -1)
    {
        perror("Error in recieving from proc_gen\n");
    }
    algorithm = initMsg.algorithm;
    quantaMax = initMsg.opts;
    struct msgbuff msg;
    
    minHeap readyQueueHPF = initMinHeap();

    int curr_time;
    int runningProcessId = -1;

    initClk();
    while (1)
    {
        if(runningProcessId != -1)
        {
            struct processInfo process = {.id = runningProcessId};
            struct processInfo* processPtr = hashmap_get(processTable, &process);
            int rec_val = msgrcv(proc_msgqup_id, &processMsg, sizeof(processMsg.remainingTime), 0, !IPC_NOWAIT);
            if (rec_val == -1)
            {
                perror("Error in recieving remaining time from process \n");
            }
            printf("schedular received remaining time %d from running process: %ld\n",processMsg.remainingTime, processMsg.mtype);
            processPtr->remainingTime = processMsg.remainingTime;
            if(processMsg.remainingTime == 0)
            { 
                runningProcessId = -1;
                processPtr->isRunning = 0;
                processPtr->finishTime = getClk();
            }
            hashmap_set(processTable,processPtr);
        }
        do
        {
            int rec_val = msgrcv(sched_msgq_id, &msg, sizeof(msg.numberOfProcesses) + sizeof(msg.p_info), 0, !IPC_NOWAIT);
            if (rec_val == -1)
            {
                perror("Error in recieving new process form process gen\n");
            }
            if(msg.numberOfProcesses)
            {
                struct processInfo process = {
                    .id = msg.p_info.id,
                    .arrivalTime = msg.p_info.arrivalTime,
                    .priority = msg.p_info.priority,
                    .runTime = msg.p_info.runTime,
                    .remainingTime = msg.p_info.runTime,
                    .isRunning = false
                    };
                if(algorithm == HPF)
                {
                    push(&readyQueueHPF, process.priority, process.id);
                }

                int pid = fork();
                if (pid == -1)
                    perror("error in fork");
                else if (pid == 0)
                {
                    execl("process.out", "process", NULL);
                }
                process.pid = pid;
                hashmap_set(processTable, &process);
            }
        } while (msg.numberOfProcesses - 1 > 0);
        curr_time = getClk();
        printf("```````````````````````````````````````````\n");
        printf("at time step: %d we have: \n", curr_time);
        hashmap_scan(processTable, process_iter, NULL);

        // HPF only
        if(runningProcessId == -1 && !isEmpty(&readyQueueHPF))
        {
            runningProcessId = peek(&readyQueueHPF)->data;
            printf("schedular will schedule the process with id %d\n", runningProcessId);
            hashmap_scan(processTable, process_iter, NULL);
            struct processInfo process = {.id = runningProcessId};
            printf("process is: %d\n", process.id);
            struct processInfo* processPtr = hashmap_get(processTable, &process);
            printf("schedular will schedule a new process to run\n");
            pop(&readyQueueHPF);
            printf("schedular will schedule the process %p to run\n", processPtr);

            processPtr->isRunning = true;
            processPtr->startTime = curr_time;
            processPtr = hashmap_set(processTable,processPtr);
            processMsg.mtype = processPtr->pid;
            processMsg.remainingTime = processPtr->remainingTime;
            int send_val = msgsnd(proc_msgqdown_id, &processMsg, sizeof(processMsg.remainingTime), !IPC_NOWAIT);
            if (send_val == -1)
            {
                perror("Error in sending from schedular to process\n");
            }
            printf("schedular message sent to the process %d with its remaining time\n", processPtr->pid);

        }
        
    }

    // remember to update the pid when the process is forked
    //TODO implement the scheduler :)
    //upon termination release the clock resources.
    destroyClk(false);
}
