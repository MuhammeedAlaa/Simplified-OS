#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#define null 0

struct processData
{
    int arrivaltime;
    int priority;
    int runningtime;
    int id;
};

int main(int argc, char *argv[])
{
    char *testnum = argv[1];
    FILE *pFile;
    strcat(testnum, ".txt");
    char *name;
    strcpy(name, "testcases/processes_");
    strcat(name, testnum);
    pFile = fopen(name, "w");
    int no;
    struct processData pData;
    printf("Please enter the number of processes you want to generate: ");
    scanf("%d", &no);
    srand(time(null));
    fprintf(pFile, "#id arrival runtime priority\n");
    pData.arrivaltime = 1;
    for (int i = 1; i <= no; i++)
    {
        pData.id = i;
        pData.arrivaltime += rand() % (11); //processes arrives in order
        pData.runningtime = rand() % (30);
        pData.priority = rand() % (11);
        fprintf(pFile, "%d\t%d\t%d\t%d\n", pData.id, pData.arrivaltime, pData.runningtime, pData.priority);
    }
    fclose(pFile);
}
