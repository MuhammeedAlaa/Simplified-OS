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
    int memsize;
};

int main(int argc, char *argv[])
{
    char *testnum = argv[1];
    FILE *pFile;
    printf("aaaaaaaaaaaa\n");
    strcat(testnum, ".txt");
    printf("aaaaaaaaaaaabb\n");
    char name[] = "testcases/processes_";
    // strcpy(name, "testcases/processes_");
    printf("aaaaaaaaaaassss\n");
    strcat(name, testnum);
    printf("aaaaaaaaaaaadd\n");
    pFile = fopen(name, "w");
    int no;
    struct processData pData;
    printf("Please enter the number of processes you want to generate: ");
    scanf("%d", &no);
    srand(time(null));
    fprintf(pFile, "#id arrival runtime priority memsize\n");
    pData.arrivaltime = 1;
    for (int i = 1; i <= no; i++)
    {
        //generate Data Randomly
        //[min-max] = rand() % (max_number + 1 - minimum_number) + minimum_number
        pData.id = i;
        pData.arrivaltime += rand() % (11); //processes arrives in order
        pData.runningtime = rand() % (30) + 1;
        pData.priority = rand() % (11);
        pData.memsize = (rand() % 256) + 1;
        fprintf(pFile, "%d\t%d\t%d\t%d\t%d\n", pData.id, pData.arrivaltime, pData.runningtime, pData.priority, pData.memsize);
    }
    fclose(pFile);
}
