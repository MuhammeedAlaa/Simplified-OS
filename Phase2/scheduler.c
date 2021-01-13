#include "headers.h"
#include "hashmap.h"
#include "priorityQueue.h"
#include "queue.h"
#include "buddy.h"

int proc_msgqup_id, proc_msgqdown_id;
minHeap readyQueueHPF;
minHeap readyQueueSRTN;
queue readyQueueRR;

void createProcess(struct processInfo process, int algorithm, int quantaMax, struct hashmap *processTable, struct hashmap *statsTable);
void executeAlgorithm(int algorithm, int quantaMax, int *remaingingQuanta, int *runningProcessId, struct hashmap *processTable, struct hashmap *statsTable);
void updateRunningProcessRemainingTime(int *runningProcessId, struct hashmap *processTable, struct hashmap *statsTable);

/* Clear the resources before exit */
void cleanup(int signum);

/*functions used for the hash table*/
int process_compare(const void *a, const void *b, void *udata);
bool process_iter(const void *item, void *udata);
uint64_t process_hash(const void *item, uint64_t seed0, uint64_t seed1);

/*functions used for the hash table*/
int stats_compare(const void *a, const void *b, void *udata);
bool stats_iter(const void *item, void *udata);
uint64_t stats_hash(const void *item, uint64_t seed0, uint64_t seed1);

void printStatsLog(int id, struct hashmap *statsTable, int state);

FILE *scheduler_log;

int main(int argc, char *argv[])
{   
    scheduler_log = fopen("scheduler.log", "w");
    
    signal(SIGINT, cleanup);
    int algorithm, quantaMax, remaingingQuanta;
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
    struct hashmap *statsTable = hashmap_new(sizeof(struct processStats), 0, seed, seed, stats_hash, stats_compare, NULL);

    switch (algorithm)
    {
    case HPF:
        readyQueueHPF = initMinHeap();
        break;
    case SRTN:
        readyQueueSRTN = initMinHeap();
        break;
    case RR:
        readyQueueRR = initQueue();
        break;
    default:
        break;
    }
    int runningProcessId = -1;
    initClk();
    //TODO: Add if Empty Queue is not empty
    while (!generator_is_done || !isEmpty(&readyQueueHPF) || !isEmpty(&readyQueueSRTN) || !isEmptyQueue(&readyQueueRR) || runningProcessId != -1)
    {

        if (runningProcessId != -1)
        {
            updateRunningProcessRemainingTime(&runningProcessId, processTable, statsTable);
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
                createProcess(msg.p_info, algorithm, quantaMax, processTable, statsTable);
            }
        } while (msg.numberOfProcesses - 1 > 0);

        printf("```````````````````````````````````````````\n");
        printf("at time step: %d we have: \n", getClk());
        hashmap_scan(processTable, process_iter, NULL);
        hashmap_scan(statsTable, stats_iter, NULL);
        executeAlgorithm(algorithm, quantaMax, &remaingingQuanta, &runningProcessId, processTable, statsTable);
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
    printf("process: (id=%d) (arrivalTime=%d) (runTime=%d) (priority=%d) (pid=%d) (state=%d) (remainingTime=%d) (finishTime=%d) (startTime=%d) (memsize=%d) \n", process->id, process->arrivalTime, process->runTime, process->priority, process->pid, process->isRunning, process->remainingTime, process->finishTime, process->startTime, process->memsize);
    return true;
}

// returns the hash of the key of the table
uint64_t process_hash(const void *item, uint64_t seed0, uint64_t seed1)
{
    return hashmap_murmur(item, sizeof(int), seed0, seed1);
}

// function used in the hashmap to compare two processes stats for equality
int stats_compare(const void *a, const void *b, void *udata){
    const struct processStats *process_a = a;
    const struct processStats *process_b = b;
    return (process_a->id == process_b->id ? 0 : 1);
}

// function used to iterate over all the hashtable contents and print them
bool stats_iter(const void *item, void *udata){
    const struct processStats *process = item;
    printf("process stats: (id=%d) (arrivalTime=%d) (runTime=%d) (remainingTime=%d) (finishTime=%d) (startTime=%d) (waitingTime=%d) (TA=%d) (WTA=%d)\n", process->id, process->arrivalTime, process->runTime, process->remainingTime, process->finishTime, process->startTime, process->waitingTime, process->TA, process->WTA);
    return true;
}
// returns the hash of the key of the table
uint64_t stats_hash(const void *item, uint64_t seed0, uint64_t seed1){
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
void createProcess(struct processInfo process, int algorithm, int quantaMax, struct hashmap *processTable, struct hashmap *statsTable)
{
    // make a struct containing the process data received from the process generator
    struct processInfo newProcess = {
        .id = process.id,
        .arrivalTime = process.arrivalTime,
        .priority = process.priority,
        .runTime = process.runTime,
        .remainingTime = process.runTime,
        .memsize = process.memsize,
        .finishTime = -1,
        .startTime = -1,
        .isRunning = false };
    
    struct processStats newStats = {
        .id = process.id,
        .arrivalTime = process.arrivalTime,
        .runTime = process.runTime,
        .remainingTime = process.runTime,
        .finishTime = -1,
        .startTime = -1,
        .waitingTime = 0,
        .TA = -1,
        .WTA = -1 };
    //TODO bool isAllocated = allocateMem();
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
        pushQueue(&readyQueueRR, newProcess.id);
        break;
    default:
        break;
    }

    // insert the process in the process table
    hashmap_set(processTable, &newProcess);
    hashmap_set(statsTable, &newStats);
}

/**
 * function execute the scheduling algorithm for the current time step
 * @param algorithm the algorithm used in the scheduler
 * @param quantaMax the maximum quanta for running a single process before preemption in RR algorithm
 * @param runningProcessId the id of the currently running process
 * @param processTable the process table that contains the processes active now in the scheduler
 * 
 */
void executeAlgorithm(int algorithm, int quantaMax, int *remaingingQuanta, int *runningProcessId, struct hashmap *processTable, struct hashmap *statsTable)
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

                struct processStats process_stats = {.id = *runningProcessId};
                struct processStats *processPtr_stats = hashmap_get(statsTable, &process_stats);

                // change the status, start time of the process to be scheduled
                processPtr->isRunning = true;
                processPtr->startTime = getClk();
                processPtr_stats->startTime = getClk();

                ///////////////////////////////////////////////////////////////print started//////////////////
                // printStatsLog(runningProcessId, statsTable, 0);

                processPtr = hashmap_set(processTable, processPtr);
                processPtr_stats = hashmap_set(statsTable, processPtr_stats);
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
        if (!isEmptyQueue(&readyQueueRR) || *runningProcessId != -1)
        {
            if (*runningProcessId == -1)
            {
                // schedule the next process in the queue
                // get next process to be scheduled from the process table and remove it from ready list
                *runningProcessId = front(&readyQueueRR);
                struct processInfo process = {.id = *runningProcessId};
                struct processInfo *processPtr = hashmap_get(processTable, &process);
                popQueue(&readyQueueRR);
                processPtr->isRunning = true;
                // intailize quanta of new process
                *remaingingQuanta = quantaMax - 1;
                // we have two cases
                // case 1 : the next process hasn't started before (we need to set its starting time)
                // case 2 : the next process has started before but was prempted by another arriving process
                // set the process new process starting time and send a message with remaining time
                if (processPtr->startTime == -1)
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

                if (*remaingingQuanta == 0)
                {
                    if (!isEmptyQueue(&readyQueueRR))
                    {
                        // get the id of next process to run
                        int currentProcessId = front(&readyQueueRR);
                        // remove next process from the ready queue to make it running
                        popQueue(&readyQueueRR);
                        // pre-empt the currently running process
                        runningProcessPtr->isRunning = 0;
                        pushQueue(&readyQueueRR, runningProcessPtr->id);
                        // set next process  as the running process
                        *runningProcessId = currentProcessId;
                        process.id = *runningProcessId;
                        runningProcessPtr = (struct processInfo *)hashmap_get(processTable, &process);
                        runningProcessPtr->isRunning = 1;
                        // set the process new process starting time and send a message with remaining time
                        if (runningProcessPtr->startTime == -1)
                            runningProcessPtr->startTime = getClk();

                        // intailize quanta of new process
                        *remaingingQuanta = quantaMax - 1;
                    }
                }
                else
                    *remaingingQuanta = *remaingingQuanta - 1;

                // printf("quanta : %d   : running %d \n", *remaingingQuanta, *runningProcessId);
                // printf("------------------------------\n");

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
void updateRunningProcessRemainingTime(int *runningProcessId, struct hashmap *processTable, struct hashmap *statsTable)
{
    // get the process control block
    struct processInfo process = {.id = *runningProcessId};
    struct processInfo *processPtr = hashmap_get(processTable, &process);
    
    struct processStats process_stats = {.id = *runningProcessId};
    struct processStats *processPtr_stats = hashmap_get(statsTable, &process_stats);
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
    processPtr_stats->remainingTime = processMsg.remainingTime;
    // check if the process is done to update its stats
    if (processMsg.remainingTime == 0)
    {
        *runningProcessId = -1;
        processPtr->isRunning = 0;
        processPtr->finishTime = getClk() + 1;
        processPtr_stats->finishTime = getClk() + 1;

        // printStatsLog(runningProcessId, statsTable, 3);
        //TODO deAllocateMem()
        //TODO pop waitingQueue -> allocateMem

    }

    hashmap_set(processTable, processPtr);
    hashmap_set(statsTable, processPtr_stats);
    
}

void printStatsLog(int id, struct hashmap *statsTable, int state){
    char *state_str;
    if(state==0)
        state_str = "started";
    else if(state==1)
        state_str = "resumed";
    else if(state==2)
        state_str = "stopped";
    else
        state_str = "finished";

    //fprintf(scheduler_log, "")          
}

void cleanup(int signum)
{
    fclose(scheduler_log);
    msgctl(proc_msgqup_id, IPC_RMID, (struct msqid_ds *)0);
    msgctl(proc_msgqdown_id, IPC_RMID, (struct msqid_ds *)0);
    destroyClk(false);
    printf("schedular terminating!\n");
    kill(getppid(), SIGUSR1);
    exit(0);
}

