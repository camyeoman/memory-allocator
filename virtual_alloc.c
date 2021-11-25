#include "virtual_sbrk.h"
#include "structure.h"
#include "virtual_alloc.h"

// HELPER FUNCTIONS

uint16_t logorithm(size_t n) {
    uint16_t count = 0;
    size_t i = n;
    while (i > 1) {
        i >>= 1;
        count++;
    }

    // round up number
    return count + (n > (1 << count) ? 1 : 0);
}

void allocation_status(struct Heap* heap, uint8_t* node) {
    if (is_valid(heap, node)) {
        if (status(node) == ALLOC || status(node) == FREE) {
            char* status_str = (status(node) == ALLOC) ? "allocated" : "free";
            printf("%s %d\n", status_str, 1 << node_size(heap, node));
        } else {
            allocation_status(heap, node_left(heap, node));
            allocation_status(heap, node_right(heap, node));
        }
    }
}

// FOWARD FACING FUNCTIONS

void init_allocator(void* heapstart, uint8_t initial_size, uint8_t min_size) {
    // set program break to byte after last address
    virtual_sbrk(1);

    // initialise heap
    struct Heap* heap = heapstart;
    heap -> cur_size = initial_size;
    heap -> min_size = min_size;
    heap -> root = 1;

    // update program break to contain buddy data structure
    virtual_sbrk(overhead(heap) + 1);

    // initialise all nodes after root to inactive
    for (int i=3; i < overhead(heap) + 1; i++) {
        uint8_t* byte = heapstart + i;
        if (i > 2) {
            enum status current = INACTIVE;
            *byte = current;
        }
    }

    // update program_break to contain the storage memory
    virtual_sbrk(1 << initial_size);
}

void* virtual_malloc(void* heapstart, uint32_t size) {
    struct Heap* heap = heapstart;
    uint8_t* root = &heap->root;

    if (size < (1 << heap->min_size)) {
        size = 1 << heap->min_size;
    } else if (size > (1 << heap->cur_size)) {
        return NULL;
    }

    // log base two of the size, which rounds up
    uint8_t log_size = logorithm(size);

    // grow tree to required size
    grow_tree(heap, log_size);

    uint8_t* node = find_node(heap, root, log_size);

    if (node != NULL) {
        set_status(node, ALLOC);

        // convert node to pointer to the storage
        void* address = heapstart + overhead(heap);
        struct Parcel parcel = { heap, 0, node };
        address += node_to_address(root, &parcel);

        return address;
    } else {
        return NULL;
    }
}

int virtual_free(void* heapstart, void* ptr) {
    if (!ptr || ptr - heapstart < 0)
        return 1;

    struct Heap* heap = heapstart;
    int64_t byte_offset = ptr - (heapstart + overhead(heap));

    struct Parcel parcel = { heap, 0, &byte_offset };
    uint8_t* node = address_to_node(&heap->root, &parcel);

    if (is_valid(heap, node)) {
        set_status(node, FREE);
        prune_tree(heap, &heap->root);
        return 0;
    } else {
        return 1;
    }

}

void* virtual_realloc(void* heapstart, void* ptr, uint32_t size) {
    // create backup of tree
    struct Heap* heap = heapstart;
    backup_tree(heap, &heap->root);

    int64_t byte_offset = ptr - (heapstart + overhead(heap));
    struct Parcel parcel = { heap, 0, &byte_offset };
    uint8_t* node = address_to_node(&heap->root, &parcel);

    if (virtual_free(heapstart, ptr) == 0) {
        void* address = virtual_malloc(heapstart, size);

        if (address == NULL) {
            // restore backup of the tree
            restore_tree(heap, &heap->root);
        } else {
            // move the data 
            uint64_t old_size = 1 << node_size(heap, node);
            uint64_t min = (old_size < size) ? old_size : size;
            memmove(address, ptr, min);
            return address;
        }
    }

    return NULL;
}

void virtual_info(void* heapstart) {
    struct Heap* heap = heapstart;
    allocation_status(heap, &heap->root);
}
