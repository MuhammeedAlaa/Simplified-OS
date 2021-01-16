#include "headers.h"

#define MAXCHAR 1000

// Function declarations
void clearResources(int);
void down(int sem);

// Global variables
union Semun semun;
int sem1, sched_msgq_id;
struct processInfo *processes_info = NULL;

int main(int argc, char *argv[])
{
    signal(SIGINT, clearResources);
    signal(SIGUSR1, clearResources);

    /*
    1. allocate an array of structs, then read the processes info from the file and store them in the array one by one 
    */
    FILE *input_file;
    char str[MAXCHAR];
    if (argc == 1)
    {
        printf("Please provide the file name (path relative to current directory) as an argument !\n");
        exit(-1);
    }
    char *file_path = argv[1];

    int number_of_processes = 0, memory_size = 10;
    processes_info = (struct processInfo *)malloc(sizeof(struct processInfo) * memory_size);

    input_file = fopen(file_path, "r");
    int process_index = 0;
    struct processInfo process;
    while (fgets(str, MAXCHAR, input_file) != NULL)
    {
        if (str[0] == '#')
            continue;
        char *token = strtok(str, "\t");
        process.id = atoi(token);

        for (int i = 0; i < 4; i++)
        {
            token = strtok(NULL, "\t");
            if (i == 0)
                process.arrivalTime = atoi(token);
            else if (i == 1)
                process.runTime = atoi(token);
            else if (i == 2)
                process.priority = atoi(token);
            else
                process.memsize = atoi(token);
        }
        processes_info[process_index] = process;
        process_index++;
        number_of_processes++;

        // if we need more memory then reallocate with twice the memory
        if (process_index == memory_size - 1)
        {
            memory_size = memory_size * 2;
            struct processInfo *new = realloc(processes_info, sizeof(struct processInfo) * memory_size);
            if (new == NULL)
            {
                perror("Error while reallocating memory\n");
            }
            processes_info = new;
        }
    }
    // reallocate memory to fit the number of processes
    struct processInfo *new = realloc(processes_info, sizeof(struct processInfo) * number_of_processes);
    if (new == NULL)
    {
        perror("Error while reallocating memory\n");
    }
    processes_info = new;
    printf("%d processes read from file \n", number_of_processes);
    fclose(input_file);

    // 2. Ask the user for the chosen scheduling algorithm and its parameters, if there are any.
    int quantum = -1;
    int algorithm_number = -1;
    printf("Please enter : \n[0] for HPF\n[1] for SRTN\n[2] for RR\n");
    scanf("%d", &algorithm_number);
    if (algorithm_number == 2)
    {
        printf("Please enter the required quantum for RR : \n");
        scanf("%d", &quantum);
    }

    // 3. Initiate and create the scheduler and clock processes.
    // -- forking the clock process
    int clock_pid = fork();
    if (clock_pid == -1)
        perror("error in forking process");
    else if (clock_pid == 0)
    {
        execl("clk.out", "clk", NULL);
    }
    // -- forking the scheduler process
    int sched_pid = fork();
    if (sched_pid == -1)
        perror("error in fork");
    else if (sched_pid == 0)
    {
        execl("scheduler.out", "scheduler", NULL);
    }

    key_t key_id = ftok("keyfile", SEM1KEY);
    sem1 = semget(key_id, 1, 0666 | IPC_CREAT);

    key_id = ftok("keyfile", MSG_SCHED_KEY);
    sched_msgq_id = msgget(key_id, 0666 | IPC_CREAT);

    int curr_process_index = 0, curr_number_of_processes;
    // TODO: check dynamic memory allocation

    struct msgAlgorithm initMsg;
    // prepare & send message to scheduler
    initMsg.mtype = getpid() % 10000;
    initMsg.algorithm = algorithm_number;
    initMsg.opts = quantum;
    int send_val = msgsnd(sched_msgq_id, &initMsg, sizeof(initMsg.algorithm) + sizeof(initMsg.opts), !IPC_NOWAIT);
    if (send_val == -1)
    {
        perror("Error in sending from proc_gen\n");
    }

    struct processInfo curr_process;
    struct msgbuff msg;
    msg.finished = false;

    // 4. initialize clock
    initClk();
    // int x = getClk();
    // printf("current time is %d\n", x);
    // 5. Send the information to the scheduler at the appropriate time.
    /* 
        Initialize the IPC Resources needed:
         - A semaphore to synchronize with the clock time
         - A message queue to send process data to the schedular through
    */
    // 6. Send the information to the scheduler at the appropriate time.
    while (1)
    {
        // wait till the clock changes
        down(sem1);
        int currentTime = getClk();
        curr_number_of_processes = 0;
        // count number of processes to be sent to the schedular at this moment of time
        while (curr_process_index < number_of_processes && processes_info[curr_process_index].arrivalTime == currentTime)
        {
            curr_process_index++;
            curr_number_of_processes++;
        }
        if (curr_process_index == number_of_processes)
            msg.finished = true;

        if (curr_number_of_processes == 0)
        {
            // prepare & send message to scheduler
            msg.mtype = getpid() % 10000;
            msg.numberOfProcesses = curr_number_of_processes;
            curr_process.id = -1;
            curr_process.arrivalTime = -1;
            curr_process.runTime = -1;
            curr_process.priority = -1;
            curr_process.memsize = -1;
            curr_process.isRunning = 0;
            curr_process.pid = -1;
            curr_process.remainingTime = -1;
            curr_process.finishTime = -1;
            curr_process.startTime = -1;
            curr_process.blockStart = -1;
            curr_process.actualSize = -1;
            msg.p_info = curr_process;
            int send_val = msgsnd(sched_msgq_id, &msg, sizeof(msg.numberOfProcesses) + sizeof(msg.p_info) + sizeof(msg.finished), !IPC_NOWAIT);
            if (send_val == -1)
            {
                perror("Error in sending from proc_gen\n");
            }
        }
        else
        {
            curr_process_index = curr_process_index - curr_number_of_processes;
            // Send the proesses to the schedular one by one
            while (curr_number_of_processes > 0)
            {
                curr_process.id = processes_info[curr_process_index].id;
                curr_process.arrivalTime = processes_info[curr_process_index].arrivalTime;
                curr_process.runTime = processes_info[curr_process_index].runTime;
                curr_process.priority = processes_info[curr_process_index].priority;
                curr_process.memsize = processes_info[curr_process_index].memsize;
                curr_process.isRunning = 0;
                curr_process.pid = -1;
                curr_process.remainingTime = -1;
                curr_process.finishTime = -1;
                curr_process.startTime = -1;
                curr_process.blockStart = -1;
                curr_process.actualSize = -1;
                curr_process_index++;

                // prepare & send message to scheduler
                msg.mtype = getpid() % 10000;
                msg.numberOfProcesses = curr_number_of_processes;
                msg.p_info = curr_process;

                int send_val = msgsnd(sched_msgq_id, &msg, sizeof(msg.numberOfProcesses) + sizeof(msg.p_info) + sizeof(msg.finished), !IPC_NOWAIT);
                validate(MSG_SEND, send_val);
                curr_number_of_processes--;
            }
        }
    }

    // 6. Clear clock resources
    destroyClk(true);
}

void down(int sem)
{
    struct sembuf p_op;

    p_op.sem_num = 0;
    p_op.sem_op = -1;
    p_op.sem_flg = !IPC_NOWAIT;

    validate(SEM_DOWN, semop(sem, &p_op, 1));
}

void clearResources(int signum)
{

    destroyClk(true);
    if (signum == 2)
    {
        // semctl(sem1, 0, IPC_RMID, semun);
        msgctl(sched_msgq_id, IPC_RMID, (struct msqid_ds *)0);
        if (processes_info != NULL)
        {
            free(processes_info);
            processes_info = NULL;
        }
    }
    exit(0);
}
