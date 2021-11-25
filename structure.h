#include <stdint.h>
#include <stdio.h>

// DATA STRUCTURE AND REPRESENTATION

/**
 * Buddy allocation data structure, storing information on
 * the size of the heap and the root of the tree which
 * represents the layout of the the memory structure.
 */
struct Heap {
    // information about heap
    uint8_t min_size;
    uint8_t cur_size;

    // buddy data structure
    uint8_t root;
};


// HEAP INFORMATION

/**
 * Calculates and returns the number of bytes used to store the
 * buddy allocation data structure. This allows the start of the
 * memory storage to be found.
 */
uint64_t overhead(struct Heap* heap);


// NODE VERIFICATION

/**
 * Returns whether a given node is in the 'tree' of the buddy
 * allocation algorithm.
 */
int in_tree(struct Heap* heap, uint8_t* node);

/**
 * Returns if the node's value is greater than zero, i.e it is
 * an active node, and whether it is in the tree. Functionally
 * is used as a stricter version of in_tree.
 */
int is_valid(struct Heap* heap, uint8_t* node);


// NODE DATA

/**
 * Returns a the size, in bytes, of the supplied arguement
 * node based on the depth of the node in the tree.
 */
uint64_t node_size(struct Heap* heap, uint8_t* node);

/**
 * Describes the status of a node using its value.
 */
enum status {
    INACTIVE = 0,
    FREE     = 1,
    ALLOC    = 2,
    PARENT   = 3
};

/**
 * Returns the status stored in the first and second bit.
 */
uint8_t status(uint8_t* node);

/**
 * Returns the backup stored in third and fourth bit.
 */
uint8_t backup(uint8_t* node);

/**
 * Sets the node's status.
 */
void set_status(uint8_t* node, uint8_t status);

/**
 * Sets the node's backup.
 */
void set_backup(uint8_t* node, uint8_t status);


// NODE RELATIONSHIPS

/**
 * Returns a pointer to the parent node of the supplied arguement
 * node if it exists.
 */
uint8_t* node_parent(struct Heap* heap, uint8_t* node);

/**
 * Returns a pointer to the left child node of the supplied
 * arguement node if it exists.
 */
uint8_t* node_left(struct Heap* heap, uint8_t* node);

/**
 * Returns a pointer to the right child node of the supplied
 * arguement node if it exists.
 */
uint8_t* node_right(struct Heap* heap, uint8_t* node);


// INTERNAL AND EXTERNAL ADDRESSING

/**
 * Stores common data ergonomically for recursive functions.
 */
struct Parcel {
    struct Heap* heap;
    uint64_t bytes;
    void* target;
};

/**
 * Returns the offset from the start of the memory storage in
 * bytes for a given node in the layout 'tree'.
 */
int64_t node_to_address(uint8_t* node, struct Parcel* parcel);

/**
 * Returns the associated 'node' from the layout 'tree' using
 * a specific byte offset.
 */
uint8_t* address_to_node(uint8_t* node, struct Parcel* parcel);

/**
 * Returns a pointer to the leftmost node in the tree of target_size.
 */
uint8_t* find_node(struct Heap* heap, uint8_t* node, int8_t target_size);


// MODIFY STRUCTURE

/**
 * Sets the backup of each node to the current status.
 */
void backup_tree(struct Heap* heap, uint8_t* node);

/**
 * Sets the status of each node to the backup and resets the backup.
 */
void restore_tree(struct Heap* heap, uint8_t* node);

/**
 * Grows the tree to a given 'size' by recursively splitting
 * nodes until an un-allocated node of 'size' is available.
 */
void grow_tree(struct Heap* heap, uint8_t size);

/**
 * If a node has two children which are both un-allocated, then
 * collapse that node. This is peformed recursively throughout
 * the entire tree from the root.
 */
void prune_tree(struct Heap* heap, uint8_t* node);
