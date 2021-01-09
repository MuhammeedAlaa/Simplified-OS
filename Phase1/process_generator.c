#include "headers.h"

void clearResources(int);

#define MAXCHAR 1000

void down(int sem);
union Semun semun;
int sem1, sched_msgq_id;
int main(int argc, char *argv[])
{
    signal(SIGINT, clearResources);

    // 1. Read the input files. done
    int numberOfProcess = -1;
    FILE *inputFile;
    char str[MAXCHAR];
    char *fileName = "processes.txt";
    inputFile = fopen(fileName, "r");

    if (inputFile == NULL)
    {
        printf("Could not open file %s", fileName);
        return 1;
    }
    while (fgets(str, MAXCHAR, inputFile) != NULL)
        numberOfProcess++;
    fclose(inputFile);
    inputFile = fopen(fileName, "r");
    struct processInfo info[numberOfProcess];
    int processCount = 0;
    while (fgets(str, MAXCHAR, inputFile) != NULL)
    {
        if (str[0] == '#')
            continue;
        struct processInfo process;
        char *temp;
        int count = 0;
        char *token = strtok(str, "\t");
        process.id = atoi(token);

        for (int i = 0; i < 3; i++)
        {
            token = strtok(NULL, "\t");
            if (i == 0)
                process.arrivalTime = atoi(token);
            else if (i == 1)
                process.runTime = atoi(token);
            else
                process.priority = atoi(token);
        }
        info[processCount] = process;
        processCount++;
    }

    // 2. Ask the user for the chosen scheduling algorithm and its parameters, if there are any.
    int q = -1;
    int algorithm = -1;
    printf("Please enter 0 for HPF, 1 for SRTN, 2 for RR \n");
    scanf("%d", &algorithm);
    if (algorithm == 2)
    {
        printf("Please enter quantum for RR \n");
        scanf("%d", &q);
    }

    // TODO Initialization

    // 3. Initiate and create the scheduler and clock processes.
    int pid1 = fork();
    if (pid1 == -1)
        perror("error in fork");
    else if (pid1 == 0)
    {
        execl("clk.out", "clk", NULL);
    }
    int pid2 = fork();
    if (pid2 == -1)
        perror("error in fork");
    else if (pid2 == 0)
    {
        execl("scheduler.out", "scheduler", NULL);
    }

    // handshaking 
    
    key_t key_id = ftok("keyfile", SEM1KEY);
    sem1 = semget(key_id, 1, 0666 | IPC_CREAT);
    // create message queue to communicate with scheduler
    key_id = ftok("keyfile", MSG_SCHED_KEY);
    sched_msgq_id = msgget(key_id, 0666 | IPC_CREAT);

    int curr_process_index = 0, curr_number_of_processes;
    // TODO: check dynamic memory allocation
    
    struct msgAlgorithm initMsg;
    // prepare & send message to scheduler
    initMsg.mtype = getpid() % 10000;
    initMsg.algorithm = algorithm;
    initMsg.opts = q;
    int send_val = msgsnd(sched_msgq_id, &initMsg, sizeof(initMsg.algorithm) + sizeof(initMsg.opts), !IPC_NOWAIT);
    if (send_val == -1)
    {
        perror("Error in sending from proc_gen\n");
    }

    struct processInfo curr_process;
    struct msgbuff msg;

    // 4. Use this function after creating the clock process to initialize clock
    initClk();
    // To get time use this
    int x = getClk();
    printf("current time is %d\n", x);
    // TODO Generation Main Loop
    // 5. Create a data structure for processes and provide it with its parameters.
    // 6. Send the information to the scheduler at the appropriate time.
    while (1)
    {
        down(sem1);
        int currentTime = getClk();
        // printf("%d \n", currentTime);
        curr_number_of_processes = 0;
        // count number of processes to be sent to the schedular
        while (info[curr_process_index].arrivalTime == currentTime)
        {
            curr_process_index++;
            curr_number_of_processes++;
        }

        if(curr_number_of_processes == 0)
        {
            // prepare & send message to scheduler
            msg.mtype = getpid() % 10000;
            msg.numberOfProcesses = curr_number_of_processes;
            int send_val = msgsnd(sched_msgq_id, &msg, sizeof(msg.numberOfProcesses) + sizeof(msg.p_info), !IPC_NOWAIT);
            if (send_val == -1)
            {
                perror("Error in sending from proc_gen\n");
            }
        }
        else
        {
            curr_process_index = curr_process_index - curr_number_of_processes;
            // fill the array with the processes of the current time
            while (curr_number_of_processes > 0)
            {
                curr_process.id = info[curr_process_index].id;
                curr_process.arrivalTime = info[curr_process_index].arrivalTime;
                curr_process.runTime = info[curr_process_index].runTime;
                curr_process.priority = info[curr_process_index].priority;
                curr_process_index++;

                // prepare & send message to scheduler
                msg.mtype = getpid() % 10000;
                msg.numberOfProcesses = curr_number_of_processes;
                msg.p_info = curr_process;
                // TODO : sizeof(msg.info)
                int send_val = msgsnd(sched_msgq_id, &msg, sizeof(msg.numberOfProcesses) + sizeof(msg.p_info), !IPC_NOWAIT);
                if (send_val == -1)
                {
                    perror("Error in sending from proc_gen\n");
                }
                curr_number_of_processes--;
            }
        }
    }

    // 7. Clear clock resources
    destroyClk(true);
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

void clearResources(int signum)
{
    destroyClk(true);
    semctl(sem1, 0, IPC_RMID, semun);
    msgctl(sched_msgq_id, IPC_RMID, (struct msqid_ds *)0);
    exit(0);
}
