///////////////////////////////////////////////////////////////////////////////
//
// Copyright 2019-2020 Jim Skrentny
// Posting or sharing this file is prohibited, including any changes/additions.
// Used by permission Fall 2020, CS354-deppeler
//
///////////////////////////////////////////////////////////////////////////////
// Main File:        myHeap.c
// This File:        myHeap.c
// Other Files:      None
// Semester:         CS 354 Fall 2020
// Instructor:       deppeler
// 
// Discussion Group: DISC 631
// Author:           Weihang Guo
// Email:            wguo63@wisc.edu
// CS Login:         weihang
//
/////////////////////////// OTHER SOURCES OF HELP //////////////////////////////
//                   fully acknowledge and credit all sources of help,
//                   other than Instructors and TAs.
//
// Persons:          None
//
// Online sources:   None
//
//////////////////////////// 80 columns wide ///////////////////////////////////
 
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <stdio.h>
#include <string.h>
#include "myHeap.h"
 
/*
 * This structure serves as the header for each allocated and free block.
 * It also serves as the footer for each free block but only containing size.
 */
typedef struct blockHeader {           
    int size_status;
    /*
    * Size of the block is always a multiple of 8.
    * Size is stored in all block headers and free block footers.
    *
    * Status is stored only in headers using the two least significant bits.
    *   Bit0 => least significant bit, last bit
    *   Bit0 == 0 => free block
    *   Bit0 == 1 => allocated block
    *
    *   Bit1 => second last bit 
    *   Bit1 == 0 => previous block is free
    *   Bit1 == 1 => previous block is allocated
    * 
    * End Mark: 
    *  The end of the available memory is indicated using a size_status of 1.
    * 
    * Examples:
    * 
    * 1. Allocated block of size 24 bytes:
    *    Header:
    *      If the previous block is allocated, size_status should be 27
    *      If the previous block is free, size_status should be 25
    * 
    * 2. Free block of size 24 bytes:
    *    Header:
    *      If the previous block is allocated, size_status should be 26
    *      If the previous block is free, size_status should be 24
    *    Footer:
    *      size_status should be 24
    */
} blockHeader;         

/* Global variable - DO NOT CHANGE. It should always point to the first block,
 * i.e., the block at the lowest address.
 */
blockHeader *heapStart = NULL;     

/* Size of heap allocation padded to round to nearest page size.
 */
int allocsize;

/*
 * The most recently allocated block.
 */
blockHeader *mostRecent = NULL;



/* 
 * Function for allocating 'size' bytes of heap memory.
 * Argument size: requested size for the payload
 * Returns address of allocated block on success.
 * Returns NULL on failure.
 * This function should:
 * - Check size - Return NULL if not positive or if larger than heap space.
 * - Determine block size rounding up to a multiple of 8 and possibly adding padding as a result.
 * - Use NEXT-FIT PLACEMENT POLICY to chose a free block
 * - Use SPLITTING to divide the chosen free block into two if it is too large.
 * - Update header(s) and footer as needed.
 * Tips: Be careful with pointer arithmetic and scale factors.
 */
void* myAlloc(int size) {
    //Check size - Return NULL if not positive or if larger than heap space.  
    int totalSize = size + 4;//size plus the size of header is the total size
    //Determine block size rounding up to a multiple of 8 and possibly adding padding as a result.
    int padding = 0;
    if (totalSize % 8 != 0){
        padding = 8 - totalSize % 8;
    }
    totalSize += padding;//update the total size with padding
    if (size <= 0 || totalSize > allocsize) {
        return NULL;
    }
    if (mostRecent == NULL) {//if no block has been allocated, set the frist block as next block
        mostRecent = heapStart;
    }
    int found = 0;//mark if an eligible block is found
    blockHeader *startBlock = mostRecent;//mark the block which is first searched
    while (found == 0){
        int nextSize = (mostRecent->size_status >> 2) << 2;//the size of next block
        if ((mostRecent->size_status & 1) == 0 && nextSize >= totalSize){
            found = 1;
            //free block and big enough for allocation
        } else {
            mostRecent += nextSize / 4;//search the next block
            if (mostRecent->size_status == 1) mostRecent = heapStart;
            //if end is reached, warp around to the first block
            if (mostRecent == startBlock) return NULL;
            //all blocks have been searched but no space found
            
        }
    }
    int nextSize = (mostRecent->size_status >> 2) << 2;
    if (nextSize - totalSize >= 8){
        mostRecent->size_status = totalSize + 3;
        //since adjacent free block should be coleasced, the last two digits should be 11, which is 3 in decimal
        blockHeader *splitBlock = mostRecent + totalSize / 4;//the header of the block splitted out
        int leftOutSize = nextSize - totalSize;//the remaining size
        splitBlock->size_status = leftOutSize + 2;//the last two digits should be 10, which is 2 in decimal
        blockHeader *splitBlockFooter = splitBlock + leftOutSize / 4 - 1;//the footer of the block splitted out
        splitBlockFooter->size_status = leftOutSize;
    } else {
        mostRecent->size_status += 1;//update the last digit
        blockHeader *nextHeader = mostRecent + nextSize/4;
        if (nextHeader->size_status != 1) nextHeader->size_status += 2;//update the header of next block
    }
    return mostRecent + 1;
} 
 
/* 
 * Function for freeing up a previously allocated block.
 * Argument ptr: address of the block to be freed up.
 * Returns 0 on success.
 * Returns -1 on failure.
 * This function should:
 * - Return -1 if ptr is NULL.
 * - Return -1 if ptr is not a multiple of 8.
 * - Return -1 if ptr is outside of the heap space.
 * - Return -1 if ptr block is already freed.
 * - USE IMMEDIATE COALESCING if one or both of the adjacent neighbors are free.
 * - Update header(s) and footer as needed.
 */                    
int myFree(void *ptr) {
    blockHeader *curBlock = (blockHeader*)ptr - 1;//the header of the block to be freed 
    //Return -1 if ptr is NULL.
    if (ptr == NULL) {
        return -1;
    }
    //Return -1 if ptr is not a multiple of 8.
    if ((int)ptr % 8 != 0) {
        return -1;
    }
    //Return -1 if ptr is outside of the heap space.
    if (ptr < (void*)heapStart || ptr>= (void*)heapStart + allocsize) {
        return -1;
    }
    //Return -1 if ptr block is already freed.
    if ((curBlock->size_status & 1) == 0) {
        return -1;
    }

    //free the current block
    int curSize = (curBlock->size_status >> 2) << 2;//the size of the block to be freed
    curBlock->size_status -= 1;//update the last digit to 0
    blockHeader *footer = curBlock + curSize / 4 - 1;//add a footer
    footer->size_status = curSize;
    blockHeader *nextBlock = curBlock + curSize / 4;//the next block
    nextBlock->size_status -= 2;//update the second last digit to 0
    //coalesce

    //coalesce with the previous block if it's also a free block
    if ((curBlock->size_status & 2) == 0) {
        blockHeader *prevFooter = curBlock - 1;//the footer of the previous block
        int preSize = prevFooter->size_status;//the size of the previous block
        blockHeader *prevBlock = curBlock - preSize / 4;
        prevBlock->size_status = curSize + preSize + 2;//the last two digits must be 10
        footer->size_status = curSize + preSize;//update footer
        if (mostRecent == curBlock) {
            mostRecent = prevBlock;//update the most reacently allocated block
        }
        curBlock = prevBlock;//update the whole block
        curSize += preSize;//update the size of whole block
    }

    //coalesce with the next block if it's also a free block
    if ((nextBlock->size_status & 1) == 0) {
        int nextSize = (nextBlock->size_status >> 2) << 2;
        curBlock->size_status = curSize + nextSize + 2;//the last two digits must be 10
        blockHeader *nextFooter = nextBlock + nextSize / 4 - 1;//the footer of the next block
        nextFooter->size_status = curSize + nextSize;//update the footer
    }
    return 0;
} 
 
/*
 * Function used to initialize the memory allocator.
 * Intended to be called ONLY once by a program.
 * Argument sizeOfRegion: the size of the heap space to be allocated.
 * Returns 0 on success.
 * Returns -1 on failure.
 */                    
int myInit(int sizeOfRegion) {    
 
    static int allocated_once = 0; //prevent multiple myInit calls
 
    int pagesize;  // page size
    int padsize;   // size of padding when heap size not a multiple of page size
    void* mmap_ptr; // pointer to memory mapped area
    int fd;

    blockHeader* endMark;
  
    if (0 != allocated_once) {
        fprintf(stderr, 
        "Error:mem.c: InitHeap has allocated space during a previous call\n");
        return -1;
    }
    if (sizeOfRegion <= 0) {
        fprintf(stderr, "Error:mem.c: Requested block size is not positive\n");
        return -1;
    }

    // Get the pagesize
    pagesize = getpagesize();

    // Calculate padsize as the padding required to round up sizeOfRegion 
    // to a multiple of pagesize
    padsize = sizeOfRegion % pagesize;
    padsize = (pagesize - padsize) % pagesize;

    allocsize = sizeOfRegion + padsize;

    // Using mmap to allocate memory
    fd = open("/dev/zero", O_RDWR);
    if (-1 == fd) {
        fprintf(stderr, "Error:mem.c: Cannot open /dev/zero\n");
        return -1;
    }
    mmap_ptr = mmap(NULL, allocsize, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
    if (MAP_FAILED == mmap_ptr) {
        fprintf(stderr, "Error:mem.c: mmap cannot allocate space\n");
        allocated_once = 0;
        return -1;
    }
  
    allocated_once = 1;

    // for double word alignment and end mark
    allocsize -= 8;

    // Initially there is only one big free block in the heap.
    // Skip first 4 bytes for double word alignment requirement.
    heapStart = (blockHeader*) mmap_ptr + 1;

    // Set the end mark
    endMark = (blockHeader*)((void*)heapStart + allocsize);
    endMark->size_status = 1;

    // Set size in header
    heapStart->size_status = allocsize;

    // Set p-bit as allocated in header
    // note a-bit left at 0 for free
    heapStart->size_status += 2;

    // Set the footer
    blockHeader *footer = (blockHeader*) ((void*)heapStart + allocsize - 4);
    footer->size_status = allocsize;
  
    return 0;
} 
                  
/* 
 * Function to be used for DEBUGGING to help you visualize your heap structure.
 * Prints out a list of all the blocks including this information:
 * No.      : serial number of the block 
 * Status   : free/used (allocated)
 * Prev     : status of previous block free/used (allocated)
 * t_Begin  : address of the first byte in the block (where the header starts) 
 * t_End    : address of the last byte in the block 
 * t_Size   : size of the block as stored in the block header
 */                     
void dispMem() {     
 
    int counter;
    char status[5];
    char p_status[5];
    char *t_begin = NULL;
    char *t_end   = NULL;
    int t_size;

    blockHeader *current = heapStart;
    counter = 1;

    int used_size = 0;
    int free_size = 0;
    int is_used   = -1;

    fprintf(stdout, "************************************Block list***\
                    ********************************\n");
    fprintf(stdout, "No.\tStatus\tPrev\tt_Begin\t\tt_End\t\tt_Size\n");
    fprintf(stdout, "-------------------------------------------------\
                    --------------------------------\n");
  
    while (current->size_status != 1) {
        t_begin = (char*)current;
        t_size = current->size_status;
    
        if (t_size & 1) {
            // LSB = 1 => used block
            strcpy(status, "used");
            is_used = 1;
            t_size = t_size - 1;
        } else {
            strcpy(status, "Free");
            is_used = 0;
        }

        if (t_size & 2) {
            strcpy(p_status, "used");
            t_size = t_size - 2;
        } else {
            strcpy(p_status, "Free");
        }

        if (is_used) 
            used_size += t_size;
        else 
            free_size += t_size;

        t_end = t_begin + t_size - 1;
    
        fprintf(stdout, "%d\t%s\t%s\t0x%08lx\t0x%08lx\t%d\n", counter, status, 
        p_status, (unsigned long int)t_begin, (unsigned long int)t_end, t_size);
    
        current = (blockHeader*)((char*)current + t_size);
        counter = counter + 1;
    }

    fprintf(stdout, "---------------------------------------------------\
                    ------------------------------\n");
    fprintf(stdout, "***************************************************\
                    ******************************\n");
    fprintf(stdout, "Total used size = %d\n", used_size);
    fprintf(stdout, "Total free size = %d\n", free_size);
    fprintf(stdout, "Total size = %d\n", used_size + free_size);
    fprintf(stdout, "***************************************************\
                    ******************************\n");
    fflush(stdout);

    return;  
} 


// end of myHeap.c (fall 2020)

