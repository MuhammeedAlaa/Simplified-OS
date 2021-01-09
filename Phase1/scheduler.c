#include "headers.h"
#include "hashmap.h"
#include "priorityQueue.h"

int proc_msgqup_id, proc_msgqdown_id;
minHeap readyQueueHPF;

void createProcess(struct processInfo process, int algorithm, int quantaMax, struct hashmap *processTable);
void executeAlgorithm(int algorithm, int quantaMax, int *runningProcessId, struct hashmap *processTable);
void updateRunningProcessRemainingTime(int* runningProcessId, struct hashmap* processTable);

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
    destroyClk(false);
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
        if (*runningProcessId == -1 && !isEmpty(&readyQueueHPF))
        {
            // get next process to be scheduled from the process table and remove it from ready list
            *runningProcessId = peek(&readyQueueHPF)->data;
            struct processInfo process = {.id = *runningProcessId};
            struct processInfo *processPtr = hashmap_get(processTable, &process);
            pop(&readyQueueHPF);

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
        break;
    case SRTN:
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
void updateRunningProcessRemainingTime(int* runningProcessId, struct hashmap* processTable)
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
