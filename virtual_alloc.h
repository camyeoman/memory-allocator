#include <stdint.h>
#include <stdio.h>
#include <string.h>

/**
 * Initialise memory allocator and the internal buddy allocation data
 * structure with initial_size bytes total memory and a minimum size
 * for allocation of min_size.
 */
void init_allocator(void* heapstart, uint8_t initial_size, uint8_t min_size);

/**
 * Request a block of 'size' bytes from the memory allocator. On success
 * returns a pointer to this block of allocated memory, else on failure
 * returns NULL.
 */
void* virtual_malloc(void* heapstart, uint32_t size);

/**
 * Free a previously allocated block of memory, if successful returns 0,
 * else returns a non zero number.
 */
int virtual_free(void* heapstart, void* ptr);

/**
 * Reallocate a previously allocated block of memory to a block of a
 * different size. On success returns a pointer to the new location,
 * and on failure return NULL.
 */
void* virtual_realloc(void* heapstart, void* ptr, uint32_t size);

/**
 * Prints out the current status of the memory.
 */
void virtual_info(void* heapstart);
