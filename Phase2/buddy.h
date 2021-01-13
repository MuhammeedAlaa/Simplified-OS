#include <math.h>
#include "sortedLinkedList.h"

#define minBlockPower 3
#define maxBlockPower 10
/* To keep track of unallocate memory blocks */
struct linkedList * buddyFreeMem[maxBlockPower+1];


void initializeBuddyMem();
void printBuddyMem();
int findNearstPowerIndex(int reqMemorySize);
int splitMem(int powerIndex, int startIndex);
int allocateMem(int reqMemorySize, int* allocatedSize);
void mergeBlocks(int block_start, int oldIndex, int oldSize, char mergeDirection);
void deallocateMem(int block_start, int actualSize);
void destroyBuddyMem();