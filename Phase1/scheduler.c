#include "headers.h"
#include "hashmap.h"
#include "priorityQueue.h"

int proc_msgqup_id, proc_msgqdown_id;
minHeap readyQueueHPF;
minHeap readyQueueSRTN;

void createProcess(struct processInfo process, int algorithm, int quantaMax, struct hashmap *processTable);
void executeAlgorithm(int algorithm, int quantaMax, int *runningProcessId, struct hashmap *processTable);
void updateRunningProcessRemainingTime(int *runningProcessId, struct hashmap *processTable);

/* Clear the resources before exit */
void cleanup(int signum);

/*functions used for the hash table*/
int process_compare(const void *a, const void *b, void *udata);
bool process_iter(const void *item, void *udata);
uint64_t process_hash(const void *item, uint64_t seed0, uint64_t seed1);

int main(int argc, char *argv[])
{
    signal(SIGINT, cleanup);
    int algorithm, quantaMax;
    bool generator_is_done = false;

    // create message queue to communicate with process generator
    key_t key_id = ftok("keyfile", MSG_SCHED_KEY);
    int sched_msgq_id = msgget(key_id, 0666 | IPC_CREAT);

    // create messege queues to communicate with the processes
    key_id = ftok("keyfile", MSG_PROC_UP_KEY);
    proc_msgqup_id = msgget(key_id, 0666 | IPC_CREAT);
    key_id = ftok("keyfile", MSG_PROC_DOWN_KEY);
    proc_msgqdown_id = msgget(key_id, 0666 | IPC_CREAT);

    // create messege buffers used in message queues
    struct msgAlgorithm initMsg;
    struct msgbuff msg;

    // get the algorithm and its options from the process generator
    int rec_val = msgrcv(sched_msgq_id, &initMsg, sizeof(initMsg.algorithm) + sizeof(initMsg.opts), 0, !IPC_NOWAIT);
    if (rec_val == -1)
    {
        perror("Error in recieving from proc_gen\n");
    }
    algorithm = initMsg.algorithm;
    quantaMax = initMsg.opts;

    // create process table and the dataStructure used as the ready list for the algorithm
    int seed = time(NULL);
    srand(time(NULL));
    struct hashmap *processTable = hashmap_new(sizeof(struct processInfo), 0, seed, seed, process_hash, process_compare, NULL);
    switch (algorithm)
    {
    case HPF:
        readyQueueHPF = initMinHeap();
        break;
    case SRTN:
        readyQueueSRTN = initMinHeap();
        break;
    case RR:
        break;
    default:
        break;
    }
    int runningProcessId = -1;
    initClk();

    while (!generator_is_done || !isEmpty(&readyQueueHPF) || runningProcessId != -1)
    {

        if (runningProcessId != -1)
        {
            updateRunningProcessRemainingTime(&runningProcessId, processTable);
        }
        do
        {
            int rec_val = msgrcv(sched_msgq_id, &msg, sizeof(msg.numberOfProcesses) + sizeof(msg.p_info) + sizeof(msg.finished), 0, !IPC_NOWAIT);
            if (rec_val == -1)
            {
                perror("Error in recieving new process form process gen\n");
            }
            generator_is_done = msg.finished;
            if (msg.numberOfProcesses)
            {
                createProcess(msg.p_info, algorithm, quantaMax, processTable);
            }
        } while (msg.numberOfProcesses - 1 > 0);

        printf("```````````````````````````````````````````\n");
        printf("at time step: %d we have: \n", getClk());
        hashmap_scan(processTable, process_iter, NULL);

        executeAlgorithm(algorithm, quantaMax, &runningProcessId, processTable);
    }
    // destroyClk(false);
    cleanup(0);
}

// function used in the hashmap to compare two processes for equality
int process_compare(const void *a, const void *b, void *udata)
{
    const struct processInfo *process_a = a;
    const struct processInfo *process_b = b;
    return (process_a->id == process_b->id ? 0 : 1);
}

// function used to iterate over all the hashtable contents and print them
bool process_iter(const void *item, void *udata)
{
    const struct processInfo *process = item;
    printf("process: (id=%d) (arrivalTime=%d) (runTime=%d) (priority=%d) (pid=%d) (state=%d) (remainingTime=%d) (finishTime=%d) (startTime=%d)\n", process->id, process->arrivalTime, process->runTime, process->priority, process->pid, process->isRunning, process->remainingTime, process->finishTime, process->startTime);
    return true;
}

// returns the hash of the key of the table
uint64_t process_hash(const void *item, uint64_t seed0, uint64_t seed1)
{
    return hashmap_murmur(item, sizeof(int), seed0, seed1);
}

/**
 * function to create a new process after receiving it from the process generator
 * @param process contains the process info received
 * @param algorithm the algorithm used in the scheduler
 * @param quantaMax the maximum quanta for running a single process before preemption in RR algorithm
 * @param processTable the process table that contains the processes active now in the scheduler
 * 
 */
void createProcess(struct processInfo process, int algorithm, int quantaMax, struct hashmap *processTable)
{
    // make a struct containing the process data received from the process generator
    struct processInfo newProcess = {
        .id = process.id,
        .arrivalTime = process.arrivalTime,
        .priority = process.priority,
        .runTime = process.runTime,
        .remainingTime = process.runTime,
        .finishTime = -1,
        .isRunning = false};
    int pid = fork();
    if (pid == -1)
        perror("error in forking new process");
    else if (pid == 0)
    {
        execl("process.out", "process", NULL);
    }
    newProcess.pid = pid;
    // insert the process in the appropriate data structure according to the algorithm
    switch (algorithm)
    {
    case HPF:
        push(&readyQueueHPF, newProcess.priority, newProcess.id);
        break;
    case SRTN:
        push(&readyQueueSRTN, newProcess.remainingTime, newProcess.id);
        break;
    case RR:
        break;
    default:
        break;
    }

    // insert the process in the process table
    hashmap_set(processTable, &newProcess);
}

/**
 * function execute the scheduling algorithm for the current time step
 * @param algorithm the algorithm used in the scheduler
 * @param quantaMax the maximum quanta for running a single process before preemption in RR algorithm
 * @param runningProcessId the id of the currently running process
 * @param processTable the process table that contains the processes active now in the scheduler
 * 
 */
void executeAlgorithm(int algorithm, int quantaMax, int *runningProcessId, struct hashmap *processTable)
{
    struct msgProcessTimeBuff processMsg;

    switch (algorithm)
    {
    case HPF:
        if (!isEmpty(&readyQueueHPF) || *runningProcessId != -1)
        {
            if (*runningProcessId == -1)
            {
                // get next process to be scheduled from the process table and remove it from ready list
                *runningProcessId = peek(&readyQueueHPF)->data;
                pop(&readyQueueHPF);
                struct processInfo process = {.id = *runningProcessId};
                struct processInfo *processPtr = hashmap_get(processTable, &process);
                // change the status, start time of the process to be scheduled
                processPtr->isRunning = true;
                processPtr->startTime = getClk();
                processPtr = hashmap_set(processTable, processPtr);

                // send messege to the process with its remaining time
                processMsg.mtype = processPtr->pid;
                processMsg.remainingTime = processPtr->remainingTime;
                int send_val = msgsnd(proc_msgqdown_id, &processMsg, sizeof(processMsg.remainingTime), !IPC_NOWAIT);
                if (send_val == -1)
                {
                    perror("Error in sending from schedular to process\n");
                }
                printf("schedular message sent to the process %d with its remaining time\n", processPtr->pid);
            }
            // if there is a process running
            else
            {
                struct processInfo process = {.id = *runningProcessId};
                struct processInfo *processPtr = hashmap_get(processTable, &process);
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
        break;
    case SRTN:
        if (!isEmpty(&readyQueueSRTN) || *runningProcessId != -1)
        {
            if (*runningProcessId == -1)
            {
                // schedule the next process in the queue
                // get next process to be scheduled from the process table and remove it from ready list
                *runningProcessId = peek(&readyQueueSRTN)->data;
                struct processInfo process = {.id = *runningProcessId};
                struct processInfo *processPtr = hashmap_get(processTable, &process);
                pop(&readyQueueSRTN);
                processPtr->isRunning = true;
                // we have two cases
                // case 1 : the next process hasn't started before (we need to set its starting time)
                // case 2 : the next process has started before but was prempted by another arriving process
                if (processPtr->runTime == processPtr->remainingTime)
                    processPtr->startTime = getClk();
                // send messege to the process with its remaining time
                processMsg.mtype = processPtr->pid;
                processMsg.remainingTime = processPtr->remainingTime;
                int send_val = msgsnd(proc_msgqdown_id, &processMsg, sizeof(processMsg.remainingTime), !IPC_NOWAIT);
                if (send_val == -1)
                {
                    perror("Error in sending from schedular to process\n");
                }
                printf("schedular message sent to the process %d with its remaining time\n", processPtr->pid);
            }
            else
            {
                // get the remaining time of the currently running process
                struct processInfo process = {.id = *runningProcessId};
                struct processInfo *runningProcessPtr = hashmap_get(processTable, &process);
                int runningProcessRemainingTime = runningProcessPtr->remainingTime;
                if (!isEmpty(&readyQueueSRTN))
                {
                    // get the arriving process with the min remaining time
                    int minProcessRemainingTime = peek(&readyQueueSRTN)->priority;

                    // compare it to the currently running process remaining time
                    /*
                - if no process arrives then the process at the top of the queue will definelty have a 
                remaining time larger than the currently running process or else it would have been scheduled
                instead of the currently running process when it arrived
                - assume we only pre-empt if the running time of the new process is less than the one currently
                running
                */
                    if (minProcessRemainingTime < runningProcessRemainingTime)
                    {
                        // get the id of the process with the least running time
                        int minProcessId = peek(&readyQueueSRTN)->data;
                        // remove the process with the least remaining time from the ready queue to make it running
                        pop(&readyQueueSRTN);
                        // pre-empt the currently running process
                        runningProcessPtr->isRunning = 0;
                        push(&readyQueueSRTN, runningProcessPtr->remainingTime, runningProcessPtr->id);
                        // set the process with the least remaining time as the running process
                        *runningProcessId = minProcessId;
                        process.id = *runningProcessId;
                        runningProcessPtr = (struct processInfo *)hashmap_get(processTable, &process);
                        runningProcessPtr->isRunning = 1;
                        // set the process new process starting time and send a message with remaining time
                        runningProcessPtr->startTime = getClk();
                    }
                }
                // send messege to the process with its remaining time
                processMsg.mtype = runningProcessPtr->pid;
                processMsg.remainingTime = runningProcessPtr->remainingTime;
                int send_val = msgsnd(proc_msgqdown_id, &processMsg, sizeof(processMsg.remainingTime), !IPC_NOWAIT);
                if (send_val == -1)
                {
                    perror("Error in sending from schedular to process\n");
                }
                printf("schedular message sent to the process %d with its remaining time\n", runningProcessPtr->pid);
            }
        }
        break;
    case RR:
        break;
    default:
        break;
    }
}

/**
 * function to update the PCB of the currently running process in the process table
 * @param runningProcessId the id of the currently running process
 * @param processTable the process table that contains the processes active now in the scheduler
 * 
 */
void updateRunningProcessRemainingTime(int *runningProcessId, struct hashmap *processTable)
{
    // get the process control block
    struct processInfo process = {.id = *runningProcessId};
    struct processInfo *processPtr = hashmap_get(processTable, &process);

    // receive the remaining time from the running process
    struct msgProcessTimeBuff processMsg;
    int rec_val = msgrcv(proc_msgqup_id, &processMsg, sizeof(processMsg.remainingTime), 0, !IPC_NOWAIT);
    if (rec_val == -1)
    {
        perror("Error in recieving remaining time from process \n");
    }
    printf("schedular received remaining time %d from running process: %ld\n", processMsg.remainingTime, processMsg.mtype);

    // update remaining time for the running process
    processPtr->remainingTime = processMsg.remainingTime;

    // check if the process is done to update its stats
    if (processMsg.remainingTime == 0)
    {
        *runningProcessId = -1;
        processPtr->isRunning = 0;
        processPtr->finishTime = getClk() + 1;
    }

    hashmap_set(processTable, processPtr);
}

void cleanup(int signum)
{
    msgctl(proc_msgqup_id, IPC_RMID, (struct msqid_ds *)0);
    msgctl(proc_msgqdown_id, IPC_RMID, (struct msqid_ds *)0);
    destroyClk(false);
    printf("schedular terminating!\n");
    kill(getppid(), SIGUSR1);
    exit(0);
}
