#include "headers.h"

void clearResources(int);

#define MAXCHAR 1000

struct processInfo
{
    int id;
    int arrivalTime;
    int runTime;
    int priority;
};

/* arg for semctl system calls. */
union Semun
{
    int val;               /* value for SETVAL */
    struct semid_ds *buf;  /* buffer for IPC_STAT & IPC_SET */
    ushort *array;         /* array for GETALL & SETALL */
    struct seminfo *__buf; /* buffer for IPC_INFO */
    void *__pad;
};

void down(int sem);
union Semun semun;
int sem1;
int main(int argc, char *argv[])
{
    signal(SIGINT, clearResources);

    // 1. Read the input files. done
    int numberOfProcess = -1;
    FILE *inputFile;
    char str[MAXCHAR];
    char *fileName = "/mnt/c/Users/Dell/Desktop/Simplified-OS/Phase1/code/processes.txt";
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
        execl("/mnt/c/Users/Dell/Desktop/Simplified-OS/Phase1/code/clock", "clock", NULL);
    }
    int pid2 = fork();
    if (pid2 == -1)
        perror("error in fork");
    else if (pid2 == 0)
    {
        execl("/mnt/c/Users/Dell/Desktop/Simplified-OS/Phase1/code/scheduler", "scheduler", NULL);
    }
    // 4. Use this function after creating the clock process to initialize clock
    initClk();
    // To get time use this
    int x = getClk();
    printf("current time is %d\n", x);
    // TODO Generation Main Loop
    // 5. Create a data structure for processes and provide it with its parameters.
    // 6. Send the information to the scheduler at the appropriate time.

    key_t key_id = ftok("keyfile", SEM1KEY);
    sem1 = semget(key_id, 1, 0666 | IPC_CREAT);
    while (1)
    {
        down(sem1);
        int currentTime = getClk();
        printf("%d \n", currentTime);
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
    exit(0);
}