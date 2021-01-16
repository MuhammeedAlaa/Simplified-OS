#include "headers.h"
#include "hashmap.h"
#include "priorityQueue.h"
#include "queue.h"
#include "buddy.h"
#include <math.h>

int proc_msgqup_id, proc_msgqdown_id, algorithm, quantaMax;
minHeap readyQueueHPF;
minHeap readyQueueSRTN;
queue readyQueueRR;
queue waitingQueue; // for processes that cannot be allocated in memory

struct hashmap *processTable;
struct hashmap *statsTable;

int createProcess(struct processInfo process, int algorithm, int quantaMax, bool pushInWaitingQueue, int currTime);
void executeAlgorithm(int algorithm, int quantaMax, int *remaingingQuanta, int *runningProcessId);
void updateRunningProcessRemainingTime(int *runningProcessId);

/* Clear the resources before exit */
void cleanup(int signum);

/*functions used for the hash table*/
int process_compare(const void *a, const void *b, void *udata);
bool process_iter(const void *item, void *udata);
uint64_t process_hash(const void *item, uint64_t seed0, uint64_t seed1);

/*functions used for the hash table*/
int stats_compare(const void *a, const void *b, void *udata);
bool stats_iter(const void *item, void *udata);
bool stats_iter_calc_sum(const void *item, void *udata);
bool stats_iter_calc_stdDev(const void *item, void *udata);

uint64_t stats_hash(const void *item, uint64_t seed0, uint64_t seed1);

void printStatsLog(int id, int state, int current_time);
void printOverallStats();

FILE *scheduler_log, *scheduler_perf;
FILE *memory_log;
int sum_running = 0, sum_waiting = 0, n = 0;
float sum_WTA = 0, sum_diff_avg;

int main(int argc, char *argv[])
{ // open files for writing output
    scheduler_log = fopen("scheduler.log", "w");
    scheduler_perf = fopen("scheduler.perf", "w");
    fprintf(scheduler_log, "#At\tTime\tX\tProcess\tY\tState\tArrived\tW\tTotal\tZ\tRemain\tY\tWait\tK\n");

    memory_log = fopen("memory.log", "w");
    fprintf(memory_log, "#At time x allocated y bytes for process z from i to j\n");

    signal(SIGINT, cleanup);
    int remaingingQuanta;
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
    processTable = hashmap_new(sizeof(struct processInfo), 0, seed, seed, process_hash, process_compare, NULL);
    statsTable = hashmap_new(sizeof(struct processStats), 0, seed, seed, stats_hash, stats_compare, NULL);
    // initialize buddy algorithm
    initializeBuddyMem();
    waitingQueue = initQueue();

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

    while (!generator_is_done || !isEmpty(&readyQueueHPF) || !isEmpty(&readyQueueSRTN) || !isEmptyQueue(&readyQueueRR) || runningProcessId != -1 || !isEmptyQueue(&waitingQueue))
    {
        printf("============================START OF TIME STEP ================================\n");
        printf("CURRENT TIME STEP : %d\n", getClk() + 1);
        if (runningProcessId != -1)
        {
            updateRunningProcessRemainingTime(&runningProcessId);
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
                printf("Attempting to Allocate the process %d in memory ... \n", msg.p_info.id);
                createProcess(msg.p_info, algorithm, quantaMax, 1, getClk());
            }
        } while (msg.numberOfProcesses - 1 > 0);

        executeAlgorithm(algorithm, quantaMax, &remaingingQuanta, &runningProcessId);

        printf("-------------------------- Process Table ----------------------------------\n");
        hashmap_scan(processTable, process_iter, NULL);
        printf("---------------------------------------------------------------------------\n");
        printf("--------------------------- Stats Table -----------------------------------\n");
        hashmap_scan(statsTable, stats_iter, NULL);
        printf("---------------------------------------------------------------------------\n");
        printf("-------------------------- waiting Queue ----------------------------------\n");
        visualizeQueue(&waitingQueue);
        printf("---------------------------------------------------------------------------\n");
        printf("============================ END OF TIME STEP ==========================\n");
        printf("\n");
    }

    printOverallStats(statsTable);
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
    printf("process: (id=%d) (arrivalTime=%d) (runTime=%d) (priority=%d) (pid=%d) (state=%d) (remainingTime=%d) (finishTime=%d) (startTime=%d) (memsize=%d) (blockstart=%d) (actualSize=%d) \n", process->id, process->arrivalTime, process->runTime, process->priority, process->pid, process->isRunning, process->remainingTime, process->finishTime, process->startTime, process->memsize, process->blockStart, process->actualSize);
    return true;
}

// returns the hash of the key of the table
uint64_t process_hash(const void *item, uint64_t seed0, uint64_t seed1)
{
    return hashmap_murmur(item, sizeof(int), seed0, seed1);
}

// function used in the hashmap to compare two processes stats for equality
int stats_compare(const void *a, const void *b, void *udata)
{
    const struct processStats *process_a = a;
    const struct processStats *process_b = b;
    return (process_a->id == process_b->id ? 0 : 1);
}

// function used to iterate over all the hashtable contents and print them
bool stats_iter(const void *item, void *udata)
{
    const struct processStats *process = item;
    printf("process stats: (id=%d) (arrivalTime=%d) (runTime=%d) (remainingTime=%d) (finishTime=%d) (startTime=%d) (waitingTime=%d) (TA=%d) (WTA=%.2f)\n", process->id, process->arrivalTime, process->runTime, process->remainingTime, process->finishTime, process->startTime, process->waitingTime, process->TA, process->WTA);
    return true;
}

// function to iterate over the items in the stats hashmap and 
// calculate the sums of (1-runtime 2-WTA 3-wait)
bool stats_iter_calc_sum(const void *item, void *udata)
{
    const struct processStats *process = item;
    sum_running += process->runTime;
    sum_WTA += process->WTA;
    sum_waiting += process->waitingTime;
    return true;
}

// function to iterate over the items in the stats hashmap and 
// calculate standard deviation of WTA
bool stats_iter_calc_stdDev(const void *item, void *udata)
{
    const struct processStats *process = item;
    float avg_WTA = sum_WTA / (float)n;
    float WTA = process->WTA;
    sum_diff_avg += ((WTA - avg_WTA) * (WTA - avg_WTA));
    return true;
}

// returns the hash of the key of the table
uint64_t stats_hash(const void *item, uint64_t seed0, uint64_t seed1)
{
    return hashmap_murmur(item, sizeof(int), seed0, seed1);
}

/**
 * function to create a new process after receiving it from the process generator
 * @param process contains the process info received
 * @param algorithm the algorithm used in the scheduler
 * @param quantaMax the maximum quanta for running a single process before preemption in RR algorithm
 * @param pushInWaitingQueue if true push the process to waiting queue if it cannot allocate in memory
 * @param currTime the curr time step recieved by the clock
 * 
 */
int createProcess(struct processInfo process, int algorithm, int quantaMax, bool pushInWaitingQueue, int currTime)
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
        .blockStart = -1,
        .actualSize = -1,
        .isRunning = false};

    // make a struct containing the process data received from the process generator for statistics
    struct processStats newStats = {
        .id = process.id,
        .arrivalTime = process.arrivalTime,
        .runTime = process.runTime,
        .remainingTime = process.runTime,
        .finishTime = -1,
        .startTime = -1,
        .waitingTime = 0,
        .TA = -1,
        .WTA = -1};

    int actualSize = -1;
    int blockStart = allocateMem(newProcess.memsize, &actualSize);
    // if it couldnot allocate -> then insert in waiting queue
    if (blockStart == -1)
    {
        if (pushInWaitingQueue == true)
            pushQueue(&waitingQueue, newProcess.id);
        printf("No sufficient memory for process %d, Added to the waiting queue \n", newProcess.id);
    }
    else
    {
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
        newProcess.blockStart = blockStart;
        newProcess.actualSize = actualSize;
        fprintf(memory_log, "At time %d allocated %d bytes for process %d from %d to %d \n", currTime, newProcess.memsize, newProcess.id, newProcess.blockStart, newProcess.blockStart + actualSize - 1);
    }
    // insert the process in the process table
    hashmap_set(processTable, &newProcess);
    hashmap_set(statsTable, &newStats);

    return blockStart;
}

/**
 * function execute the scheduling algorithm for the current time step
 * @param algorithm the algorithm used in the scheduler
 * @param quantaMax the maximum quanta for running a single process before preemption in RR algorithm
 * @param remaingingQuanta the remaining quanta of the running process
 * @param runningProcessId the id of the currently running process
 * 
 */
void executeAlgorithm(int algorithm, int quantaMax, int *remaingingQuanta, int *runningProcessId)
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
                printStatsLog(*runningProcessId, 0, getClk());

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
            }
            // if there is a process running
            else
            {
                // send to the process its remaining time
                struct processInfo process = {.id = *runningProcessId};
                struct processInfo *processPtr = hashmap_get(processTable, &process);
                processMsg.mtype = processPtr->pid;
                processMsg.remainingTime = processPtr->remainingTime;
                int send_val = msgsnd(proc_msgqdown_id, &processMsg, sizeof(processMsg.remainingTime), !IPC_NOWAIT);
                if (send_val == -1)
                {
                    perror("Error in sending from schedular to process\n");
                }
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

                struct processStats process_stats = {.id = *runningProcessId};
                struct processStats *processPtr_stats = hashmap_get(statsTable, &process_stats);

                pop(&readyQueueSRTN);
                processPtr->isRunning = true;
                // we have two cases
                // case 1 : the next process hasn't started before (we need to set its starting time)
                // case 2 : the next process has started before but was prempted by another arriving process
                if (processPtr->runTime == processPtr->remainingTime)
                {
                    processPtr->startTime = getClk();
                    processPtr_stats->startTime = getClk();
                    //Started
                    printStatsLog(*runningProcessId, 0, getClk());
                }
                else
                {
                    //Resumed
                    printStatsLog(*runningProcessId, 1, getClk());
                }
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
                // printf("schedular message sent to the process %d with its remaining time\n", processPtr->pid);
            }
            else
            {
                // get the remaining time of the currently running process
                struct processInfo process = {.id = *runningProcessId};
                struct processInfo *runningProcessPtr = hashmap_get(processTable, &process);
                struct processStats process_stats = {.id = *runningProcessId};
                struct processStats *processPtr_stats = hashmap_get(statsTable, &process_stats);

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

                        //Stopped
                        printStatsLog(*runningProcessId, 2, getClk());

                        // set the process with the least remaining time as the running process
                        *runningProcessId = minProcessId;
                        process.id = *runningProcessId;
                        process_stats.id = *runningProcessId;
                        runningProcessPtr = (struct processInfo *)hashmap_get(processTable, &process);
                        processPtr_stats = (struct processStats *)hashmap_get(statsTable, &process_stats);
                        runningProcessPtr->isRunning = 1;
                        // set the process new process starting time and send a message with remaining time
                        runningProcessPtr->startTime = getClk();
                        processPtr_stats->startTime = getClk();

                        //Started
                        printStatsLog(*runningProcessId, 0, getClk());
                        runningProcessPtr = hashmap_set(processTable, runningProcessPtr);
                        processPtr_stats = hashmap_set(statsTable, processPtr_stats);
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
                // printf("schedular message sent to the process %d with its remaining time\n", runningProcessPtr->pid);
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
                struct processStats process_stats = {.id = *runningProcessId};
                struct processStats *processPtr_stats = hashmap_get(statsTable, &process_stats);

                popQueue(&readyQueueRR);
                processPtr->isRunning = true;
                // intailize quanta of new process
                *remaingingQuanta = quantaMax - 1;
                // we have two cases
                // case 1 : the next process hasn't started before (we need to set its starting time)
                // case 2 : the next process has started before but was prempted by another arriving process
                // set the process new process starting time and send a message with remaining time
                if (processPtr->startTime == -1)
                {
                    processPtr->startTime = getClk();
                    processPtr_stats->startTime = getClk();
                    //Started
                    printStatsLog(*runningProcessId, 0, getClk());
                }
                else
                {
                    //Resumed
                    printStatsLog(*runningProcessId, 1, getClk());
                }
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
            }
            else
            {
                // get the remaining time of the currently running process
                struct processInfo process = {.id = *runningProcessId};
                struct processInfo *runningProcessPtr = hashmap_get(processTable, &process);

                struct processStats process_stats = {.id = *runningProcessId};
                struct processStats *processPtr_stats = hashmap_get(statsTable, &process_stats);

                if (*remaingingQuanta == 0)
                {
                    //Stopped
                    printStatsLog(*runningProcessId, 2, getClk());
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
                        process_stats.id = *runningProcessId;
                        runningProcessPtr = (struct processInfo *)hashmap_get(processTable, &process);
                        processPtr_stats = (struct processStats *)hashmap_get(statsTable, &process_stats);
                        runningProcessPtr->isRunning = 1;
                        // set the process new process starting time and send a message with remaining time
                        if (runningProcessPtr->startTime == -1)
                        {
                            //Started
                            runningProcessPtr->startTime = getClk();
                            processPtr_stats->startTime = getClk();
                            printStatsLog(*runningProcessId, 0, getClk());
                        }
                        else
                        {
                            //Resumed
                            printStatsLog(*runningProcessId, 1, getClk());
                        }

                        // intailize quanta of new process
                        *remaingingQuanta = quantaMax - 1;

                        runningProcessPtr = hashmap_set(processTable, runningProcessPtr);
                        processPtr_stats = hashmap_set(statsTable, processPtr_stats);
                    }
                }
                else
                    *remaingingQuanta = *remaingingQuanta - 1;

                // send messege to the process with its remaining time
                processMsg.mtype = runningProcessPtr->pid;
                processMsg.remainingTime = runningProcessPtr->remainingTime;
                int send_val = msgsnd(proc_msgqdown_id, &processMsg, sizeof(processMsg.remainingTime), !IPC_NOWAIT);
                if (send_val == -1)
                {
                    perror("Error in sending from schedular to process\n");
                }
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
 * 
 */
void updateRunningProcessRemainingTime(int *runningProcessId)
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

        //finished
        printStatsLog(processPtr_stats->id, 3, getClk() + 1);

        deallocateMem(processPtr->blockStart, processPtr->actualSize);
        fprintf(memory_log, "At time %d freed %d from process %d from %d to %d\n", processPtr->finishTime, processPtr->memsize, processPtr->id, processPtr->blockStart, processPtr->blockStart + processPtr->actualSize - 1);
        // after deallocating check if a process from the waiting queue can be allocated
        int blockStart, id;
        while (!isEmptyQueue(&waitingQueue))
        {
            id = front(&waitingQueue);
            struct processInfo pr = {.id = id};
            struct processInfo *p = hashmap_get(processTable, &pr);
            blockStart = createProcess(*p, algorithm, quantaMax, 0, getClk() + 1);
            if (blockStart == -1)
                break;
            popQueue(&waitingQueue);
        }
        //free the PCB of finished process
        hashmap_delete(processTable, &process);
    }
    else
    {
        hashmap_set(processTable, processPtr);
    }

    hashmap_set(statsTable, processPtr_stats);
}
/**
 * @param id the id of the process to be printed
 * @param state to indicate the state of the process (started, resumed, stopped, or finished)
 * @param current_time the current time seen by the scheduler
 */
void printStatsLog(int id, int state, int current_time)
{
    char *state_str;
    if (state == 0)
        state_str = "started";
    else if (state == 1)
        state_str = "resumed";
    else if (state == 2)
        state_str = "stopped";
    else
        state_str = "finished";

    struct processStats process_stats = {.id = id};
    struct processStats *processPtr_stats = hashmap_get(statsTable, &process_stats);

    int y = id;                              //id
    int w = processPtr_stats->arrivalTime;   //arrival time
    int z = processPtr_stats->runTime;       //tun time
    int r = processPtr_stats->remainingTime; //remain time
    int k = current_time - w - z + r;        //wait time

    processPtr_stats->waitingTime = k;
    processPtr_stats->remainingTime = r;

    if (state == 3)
    {
        int TA = current_time - w;
        float WTA = (float)TA / (float)z;
        processPtr_stats->TA = TA;
        processPtr_stats->WTA = WTA;
        fprintf(scheduler_log, " At\tTime\t%d\tProcess\t%d\t%s\tArrived\t%d\tTotal\t%d\tRemain\t%d\tWait\t%d\tTA\t%d\tWTA\t%.2f\n", current_time, y, state_str, w, z, r, k, TA, WTA);
    }
    else
        fprintf(scheduler_log, " At\tTime\t%d\tProcess\t%d\t%s\tArrived\t%d\tTotal\t%d\tRemain\t%d\tWait\t%d\n", current_time, y, state_str, w, z, r, k);

    hashmap_set(statsTable, processPtr_stats);
}

/**
 * function to store the performance of the scheduler in a file
 */
void printOverallStats()
{
    int finalTime = getClk() - 1;
    hashmap_scan(statsTable, stats_iter_calc_sum, NULL);
    n = hashmap_count(statsTable);
    float cpu_utilization = ((float)sum_running / (float)finalTime) * 100;
    fprintf(scheduler_perf, "CPU utilization = %.2f%% \n", cpu_utilization);
    fprintf(scheduler_perf, "Avg WTA = %.2f \n", (float)sum_WTA / (float)n);
    fprintf(scheduler_perf, "Avg Waiting = %.2f \n", (float)sum_waiting / (float)n);
    hashmap_scan(statsTable, stats_iter_calc_stdDev, NULL);
    float STD_WTA = sqrt(sum_diff_avg / (float)n);
    fprintf(scheduler_perf, "Std WTA = %.2f \n", STD_WTA);
}

/**
 * function to clean up the resources used
 */
void cleanup(int signum)
{
    fclose(scheduler_log);
    fclose(scheduler_perf);
    fclose(memory_log);
    msgctl(proc_msgqup_id, IPC_RMID, (struct msqid_ds *)0);
    msgctl(proc_msgqdown_id, IPC_RMID, (struct msqid_ds *)0);
    hashmap_free(processTable);
    hashmap_free(statsTable);
    destroyClk(false);
    destroyBuddyMem();
    printf("schedular terminating!\n");
    kill(getppid(), SIGUSR1);
    exit(0);
}
