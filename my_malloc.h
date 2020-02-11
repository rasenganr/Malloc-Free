/**--
   This header file is the definition of functions and structs
--**/

#include <stddef.h>

// Size of the meta data part
#define META (sizeof(struct MyBlock))

// Free the specified block
void myFree(void * ptr);

// First-Fit malloc/free
void * ff_malloc(size_t size);
void ff_free(void * ptr);

// Best-Fit malloc/free
void * bf_malloc(size_t size);
void bf_free(void * ptr);

// A single block unit
struct MyBlock {
  size_t size;             // The size of this data block
  struct MyBlock * fprev;  // A pointer to the previous free block
  struct MyBlock * fnext;  // A pointer to the next free block
};

typedef struct MyBlock Block;

// Find the first-fit block and return its starting address
// If no such block, create one and return that block
Block * ff_getBlock(size_t size);

// Find the best-fit block
// If no such block, create one and return that block
Block * bf_getBlock(size_t size);

// Create a new block
// Adjust the break pointer through sbrk()
Block * newBlock(size_t size);

// Merge two adjacent free blocks
void merge(Block * first, Block * second);

// Split a block into two
void split(Block * toSplit, size_t size);

// Remove a block from the free list
// Happen only during malloc
void removeFree(Block * toRemove);

// Add a freed block to the free list at back
void addFree(Block * toAdd);

// Get the size of data segment
unsigned long get_data_segment_size();

// Get the size of free space
unsigned long get_data_segment_free_space_size();
