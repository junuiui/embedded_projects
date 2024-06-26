// Referenced SFU Dr. Brian Fraser's PRU Guide

#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <pthread.h>
#include "hal/sharedMem-Linux.h"
#include "../../pru-tl/sharedDataStruct.h"
#include "../../app/include/utils.h"

// General PRU Memomry Sharing Routine
// ----------------------------------------------------------------
#define PRU_ADDR 0x4A300000 // Start of PRU memory Page 184 am335x TRM
#define PRU_LEN 0x80000 // Length of PRU memory
#define PRU0_DRAM 0x00000 // Offset to DRAM
#define PRU1_DRAM 0x02000
#define PRU_SHAREDMEM 0x10000 // Offset to shared memory
#define PRU_MEM_RESERVED 0x200 // Amount used by stack and heap

// Convert base address to each memory section
#define PRU0_MEM_FROM_BASE(base) ( (base) + PRU0_DRAM + PRU_MEM_RESERVED)
#define PRU1_MEM_FROM_BASE(base) ( (base) + PRU1_DRAM + PRU_MEM_RESERVED)
#define PRUSHARED_MEM_FROM_BASE(base) ( (base) + PRU_SHAREDMEM)

static int shouldRun = 1;
static int status = 1;
// Return the address of the PRU's base memory
volatile void* getPruMmapAddr(void) {
    int fd = open("/dev/mem", O_RDWR | O_SYNC);
    if (fd == -1) {
        perror("ERROR: could not open /dev/mem");
        exit(EXIT_FAILURE);
    }
    // Points to start of PRU memory.
    volatile void* pPruBase = mmap(0, PRU_LEN, PROT_READ | PROT_WRITE, MAP_SHARED, fd, PRU_ADDR);

    if (pPruBase == MAP_FAILED) {
        perror("ERROR: could not map memory");
        exit(EXIT_FAILURE);
    }
    close(fd);
    return pPruBase;
}

void freePruMmapAddr(volatile void* pPruBase){
    if (munmap((void*) pPruBase, PRU_LEN)) {
        perror("PRU munmap failed");
        exit(EXIT_FAILURE);
    }
}

void sharedMem_init(){
    runCommand("config-pin p8_11 pruout");
    runCommand("config-pin p8_15 pruin");
    runCommand("config-pin p8_16 pruin");
}

void* sharedMem_thread (void* args) {

    // Get access to shared memory for my uses
    volatile void *pPruBase = getPruMmapAddr();
    volatile sharedMemStruct_t *pSharedPru0 = PRU0_MEM_FROM_BASE(pPruBase);

    pSharedPru0->isDownPressed = false;
    pSharedPru0->isRightPressed = false;
    pSharedPru0->mode = false;
    pSharedPru0->gTime = 15000; // 15 sec
    pSharedPru0->rTime = 15000; // 15
    pSharedPru0->yTime = 2000;  // 2

    // Drive it
    while(shouldRun) {
        if(pSharedPru0->isRightPressed){
            shouldRun = 0;
            break;
        }

        printf("DOWN? %d ", pSharedPru0->isDownPressed);
        printf("RIGHT?: %d ", pSharedPru0->isRightPressed);
        printf("mode: %d\n", pSharedPru0->mode);
        sleepForMs(100);
    }

    // Cleanup
    status = 0;
    freePruMmapAddr(pPruBase);
    pthread_exit(NULL);
}

void sharedMem_cleanup(){
    printf("\n*********\n");
	printf("Cleaning sharedMem\n");

	printf("\tStopping sharedMem Thread...");
	shouldRun = 0;
    
	printf(" DONE\n");

	printf("sharedMem Successfully Cleaned\n");
	printf("*********\n");

}

int sharedMem_getStatus(){
    return status;
}

