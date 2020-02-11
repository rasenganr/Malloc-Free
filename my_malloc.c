#include "my_malloc.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

// Global variables
Block * fhead = NULL;  // The head of the free-block list
Block * ftail = NULL;  // The tail of the free-block list
Block * head = NULL;   // The head of all blocks

//~~~~~~~~~~~~~~~~~~~~~~~~~
// Malloc a first-fit block
//~~~~~~~~~~~~~~~~~~~~~~~~~
void * ff_malloc(size_t size) {
  // Get the first-fit block
  Block * target = ff_getBlock(size);
  // Provide the user with a pointer to the first byte of the data area
  return (void *)((char *)target + META);
}

//~~~~~~~~~~~~~~~~~~~~~~~~
// Malloc a best-fit block
//~~~~~~~~~~~~~~~~~~~~~~~~
void * bf_malloc(size_t size) {
  // Get the best-fit block
  Block * target = bf_getBlock(size);
  // Provide the user with a pointer to the first byte of the data area
  return (void *)((char *)target + META);
}

//~~~~~~~~~~~~~~~~~~~~~~~~~
// Free the first-fit block
//~~~~~~~~~~~~~~~~~~~~~~~~~
void ff_free(void * ptr) {
  myFree(ptr);
}

//~~~~~~~~~~~~~~~~~~~~~~~~
// Free the best-fit block
//~~~~~~~~~~~~~~~~~~~~~~~~
void bf_free(void * ptr) {
  myFree(ptr);
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Find the first-fit block and return its starting address
// If no such block, create one and return that block
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Block * ff_getBlock(size_t size) {
  Block ** curr = &fhead;

  /*---Search for the first-fit block---*/
  // Skip invalid blocks (occupied or too small)
  while (*curr != NULL && (*curr)->size < size) {
    curr = &(*curr)->fnext;
  }

  /*---Search finished---*/
  // No valid block, create one
  if (*curr == NULL) {
    return newBlock(size);
  }
  // Occupy an existing block
  else {
    Block * ffBlock = *curr;
    // Need splitting
    if (ffBlock->size > 2 * (META + size)) {
      split(*curr, size);
    }
    // Remove this block from the free-block list
    removeFree(ffBlock);
    return ffBlock;
  }
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Find the best-fit block and return its starting address
// If no such block, create one and return that block
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Block * bf_getBlock(size_t size) {
  Block ** curr = &fhead;
  Block * bfBlock = NULL;

  /*---Search for the best-fit block---*/
  while (*curr != NULL) {
    // Best block found, stop searching
    if ((*curr)->size == size) {
      bfBlock = *curr;
      break;
    }
    // Valid block
    else if ((*curr)->size > size) {
      // Update the best-fit block
      if (bfBlock == NULL || (*curr)->size < bfBlock->size) {
        bfBlock = *curr;
      }
    }
    curr = &(*curr)->fnext;
  }

  /*---Search finished---*/
  // No valid block, create one
  if (bfBlock == NULL) {
    bfBlock = newBlock(size);
  }
  // Occupy a valid block
  else {
    // Need splitting
    if (bfBlock->size > 2 * (META + size)) {
      split(bfBlock, size);
    }
    // Remove this block from the free-block list
    removeFree(bfBlock);
  }

  return bfBlock;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Create a new block
// Adjust the break pointer through sbrk()
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Block * newBlock(size_t size) {
  // Position of the current break pointer
  Block * curr = (Block *)sbrk(0);
  // Adjust the break pointer
  // Allocate a piece of memory containing both metadata and data
  sbrk(META + size);
  // Initialize the new block
  curr->size = size;
  curr->fprev = NULL;
  curr->fnext = NULL;
  if (head == NULL) {
    head = curr;
  }
  return curr;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~
// Free the specified block
//~~~~~~~~~~~~~~~~~~~~~~~~~
void myFree(void * ptr) {
  // Get the block the freed data resides in
  Block * toFree = (Block *)((char *)ptr - META);
  // Add the newly freed block to the free list
  addFree(toFree);
  // Merge the next block if it is in the free-block list
  if (toFree->fnext != NULL &&
      toFree->fnext == (Block *)((char *)toFree + META + toFree->size)) {
    merge(toFree, toFree->fnext);
  }
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Merge two adjacent free blocks
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void merge(Block * first, Block * second) {
  /*---Puth both blocks at the back of the free list---*/
  removeFree(first);
  removeFree(second);
  addFree(first);
  addFree(second);

  /*---Adjust the free list---*/
  ftail = first;
  first->size += META + second->size;
  first->fnext = NULL;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Split a free block into two free blocks
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void split(Block * toSplit, size_t size) {
  Block * newBlock = (Block *)((char *)toSplit + META + size);
  // Initialize the new block
  newBlock->fprev = toSplit;
  newBlock->fnext = toSplit->fnext;
  newBlock->size = toSplit->size - META - size;
  // Split the last block
  if (ftail == toSplit) {
    ftail = newBlock;
  }
  // Adjust the next block
  else {
    toSplit->fnext->fprev = newBlock;
  }
  // Adjust the original block
  toSplit->size = size;
  toSplit->fnext = newBlock;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Remove a block from the free list
// Happen only during malloc
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void removeFree(Block * toRemove) {
  /*---Modify the head/previous block---*/
  // Remove the first block
  if (fhead == toRemove) {
    fhead = toRemove->fnext;
  }
  // Modify the previous block
  else {
    toRemove->fprev->fnext = toRemove->fnext;
  }

  /*---Modify the tail/next block---*/
  // Remove the last block
  if (ftail == toRemove) {
    ftail = toRemove->fprev;
  }
  // Modify the next block
  else {
    toRemove->fnext->fprev = toRemove->fprev;
  }
  toRemove->fprev = NULL;
  toRemove->fnext = NULL;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Add a freed block to the free list at back
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void addFree(Block * toAdd) {
  // Empty free-block list
  if (ftail == NULL) {
    toAdd->fprev = NULL;
    toAdd->fnext = NULL;
    fhead = toAdd;
    ftail = toAdd;
  }
  // Modify the previous free block
  else {
    toAdd->fprev = ftail;
    toAdd->fnext = NULL;
    ftail->fnext = toAdd;
    ftail = toAdd;
  }
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Get the size of data segment
// The memory size between current break pointer and the head of the list
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
unsigned long get_data_segment_size() {
  return (unsigned long)((char *)sbrk(0) - (char *)head);
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Get the size of free space
//~~~~~~~~~~~~~~~~~~~~~~~~~~~
unsigned long get_data_segment_free_space_size() {
  unsigned long freeSize = 0;
  Block * curr = fhead;
  // Traverse the free-block list
  // Add each block's [metadata size + data size]
  while (curr != NULL) {
    freeSize += META + curr->size;
    curr = curr->fnext;
  }
  return freeSize;
}
