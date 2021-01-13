#include "buddy.h"


void initializeBuddyMem() {
    // initialze a linked list for each block size to keep track of free memory of that size
    for(int i = minBlockPower; i <= maxBlockPower; i++) {
        buddyFreeMem[i] = creatLinkedList();
    }
    // insert total memory as first free block 
    InsertOrdered(buddyFreeMem[maxBlockPower], 0);
}

void printBuddyMem() {
    printf("----------- Buddy System Table -------------------------\n");
    for(int i = minBlockPower; i <= maxBlockPower; i++) {
        printf("size:%d    |", (int)pow(2, i));
        Traverse(buddyFreeMem[i]);
        printf("\n");
    }
    printf("--------------------------------------------------------\n");
}

int findNearstPowerIndex(int reqMemorySize) {
    int powerIndex = (int)log2(reqMemorySize);
    if(reqMemorySize == (int)pow(2, powerIndex))
        return powerIndex;
    else
     return powerIndex+1;
}

// split memory block
int splitMem(int powerIndex, int startIndex) {
    int blockSize  = (int) pow(2, powerIndex);
    // printf("blockSize : %d \n", blockSize);
    // printf("powerIndex in split : %d \n", powerIndex);
    powerIndex--;
    int j = startIndex + blockSize/2;
    InsertOrdered(buddyFreeMem[powerIndex], j);
    return 1;
}

// returns the startindex and the allocated size or -1 if no available memory
int allocateMem(int reqMemorySize, int* allocatedSize) {
    // get next largest power of 2
    int powerIndex = findNearstPowerIndex(reqMemorySize);
    if (powerIndex < minBlockPower) powerIndex = minBlockPower;
    printf("powerIndex : %d\n", powerIndex);
    int cntTrials = 0;
    int i = powerIndex;
    // check for suitable memory size available
    while(i <= maxBlockPower && isempty(buddyFreeMem[i])) {
        cntTrials++;
        i++;
    }
    printf("cntTrials : %d \n", cntTrials);
    // if no memory is available
    if(i == maxBlockPower+1) 
        return -1;

    *allocatedSize = (int)pow(2, powerIndex);
    printf("allocatedSize : %d \n", *allocatedSize);
    int startIndex;
    startIndex = buddyFreeMem[i]->head->data;
    DeleteNode(buddyFreeMem[i], startIndex);
    // split till reaching the size needed
    while(cntTrials > 0) {
        splitMem(i, startIndex);
        i--;
        cntTrials--;
    }
    printf("memory allocated from i : %d to j : %d \n", startIndex, startIndex + *allocatedSize - 1);
    printBuddyMem();
    return startIndex;
}

// mergeDirection => position of the one to be merged
void mergeBlocks(int block_start, int oldIndex, int oldSize, char mergeDirection) {
    if(mergeDirection == 'r') {
        // remove the block on the right 
        DeleteNode(buddyFreeMem[oldIndex], block_start + oldSize);
    }
    else {
        // remove the block on the left
        DeleteNode(buddyFreeMem[oldIndex], block_start);
    }
        // int newIndex = oldIndex + 1;
        // InsertOrdered(buddyFreeMem[newIndex], block_start);
}

void deallocateMem(int block_start, int actualSize) {
    int i = (int)log2(actualSize);
    while(i < maxBlockPower) {
        if ((block_start/actualSize) % 2 == 0) {
            // if buddy is found
            if(findNode(buddyFreeMem[i], block_start + actualSize) == 1) {
                mergeBlocks(block_start, i, actualSize, 'r');
            }
            else {
                InsertOrdered(buddyFreeMem[i], block_start);
                break;
            }
        }
        else {
            if(findNode(buddyFreeMem[i], block_start - actualSize) == 1) {
                block_start = block_start - actualSize;
                mergeBlocks(block_start, i, actualSize, 'l');
            }
            else {
                InsertOrdered(buddyFreeMem[i], block_start);
                break;
            }
        }
        i++;
        actualSize = actualSize * 2;
    }
    if(i == maxBlockPower) {
        InsertOrdered(buddyFreeMem[maxBlockPower], 0);
    }
    printBuddyMem();
}

void destroyBuddyMem() {
    for(int i = minBlockPower; i <= maxBlockPower; i++) {
        free(buddyFreeMem[i]);
    }
}
