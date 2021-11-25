#include "structure.h"


// HEAP INFORMATION

uint64_t overhead(struct Heap* heap) {
    return 2 + (1 << (heap->cur_size - heap->min_size + 1));
}


// NODE VERIFICATION

int in_tree(struct Heap* heap, uint8_t* node) {
    uint8_t* last = ((void*) heap) + overhead(heap);
    uint8_t* root = &heap->root;
    return node - root >= 0 && last - node >= 0;
}

int is_valid(struct Heap* heap, uint8_t* node) {
    return in_tree(heap, node) && status(node) > 0;
}


// NODE DATA

uint64_t node_size(struct Heap* heap, uint8_t* node) {
    if (node == &heap->root) {
        return heap->cur_size;
    } else {
        uint8_t* parent_node = node_parent(heap, node);
        return node_size(heap, parent_node) - 1;
    }
}

uint8_t status(uint8_t* node) {
    return (node) ? *node & 0b11 : INACTIVE;
}

uint8_t backup(uint8_t* node) {
    return (node) ? (*node >> 2) & 0b11 : INACTIVE;
}

void set_status(uint8_t* node, uint8_t status) {
    *node = (*node & 0b1100) + (status & 0b11);
}

void set_backup(uint8_t* node, uint8_t status) {
    *node = (*node & 0b0011) + ((status & 0b11) << 2);
}


// NODE RELATIONSHIPS

uint8_t* node_parent(struct Heap* heap, uint8_t* node) {
    void* ptr = &heap->root + ((node - &heap->root) - 1) / 2;
    return (in_tree(heap, ptr)) ? ptr : NULL;
}

uint8_t* node_left(struct Heap* heap, uint8_t* node) {
    void* ptr = &heap->root + (2 * (node - &heap->root) + 1);
    return (in_tree(heap, ptr)) ? ptr : NULL;
}

uint8_t* node_right(struct Heap* heap, uint8_t* node) {
    void* ptr = &heap->root + (2 * (node - &heap->root) + 2);
    return (in_tree(heap, ptr)) ? ptr : NULL;
}


// INTERNAL AND EXTERNAL ADDRESSING

int64_t node_to_address(uint8_t* node, struct Parcel* parcel) {
    struct Heap* heap = parcel->heap;
    uint8_t* target = parcel->target;

    if ((status(node) == ALLOC || status(node) == FREE)
            && in_tree(heap, node)) {
        if (node == target) {
            return parcel->bytes;
        } else {
            parcel->bytes += (1 << node_size(heap, node));
        }
    } else if (is_valid(heap, node)) {
        int64_t left = node_to_address(node_left(heap, node), parcel);
        int64_t right = node_to_address(node_right(heap, node), parcel);
        return (left > right) ? left : right;
    }

    return -1;
}

uint8_t* address_to_node(uint8_t* node, struct Parcel* parcel) {
    struct Heap* heap = parcel->heap;
    int64_t target = *((int64_t*) parcel->target);

    if ((status(node) == ALLOC || status(node) == FREE)
            && in_tree(heap, node)) {
        if (parcel->bytes == target) {
            return node;
        } else {
            parcel->bytes += (1 << node_size(heap, node));
        }
    } else if (is_valid(heap, node)) {
        uint8_t* left = address_to_node(node_left(heap, node), parcel);
        uint8_t* right = address_to_node(node_right(heap, node), parcel);
        return (left) ? left : right;
    }

    return NULL;
}

uint8_t* find_node(struct Heap* heap, uint8_t* node, int8_t target_size) {
    if (!is_valid(heap, node))
        return NULL;

    if (status(node) == FREE && node_size(heap, node) == target_size) {
        return node;
    } else {
        uint8_t* left = find_node(heap, node_left(heap, node), target_size);
        uint8_t* right = find_node(heap, node_right(heap, node), target_size);
        return (left) ? left : right;
    }
}


// MODIFY STRUCTURE

void backup_tree(struct Heap* heap, uint8_t* node) {
    if (is_valid(heap, node)) {
        set_backup(node, status(node));

        backup_tree(heap, node_left(heap, node));
        backup_tree(heap, node_right(heap, node));
    }
}

void restore_tree(struct Heap* heap, uint8_t* node) {
    if (in_tree(heap, node) && backup(node) > 0) {
        set_status(node, backup(node));
        set_backup(node, 0);

        restore_tree(heap, node_left(heap, node));
        restore_tree(heap, node_right(heap, node));
    }
}

void grow_tree(struct Heap* heap, uint8_t size) {
    uint8_t* node = find_node(heap, &heap->root, size);
    int i = 0;
    while (!node && size + i <= heap->cur_size) {
        node = find_node(heap, &heap->root, size + i++);
    }

    if (node != NULL) {
        int curr = node_size(heap, node);
        if (curr > size && curr > heap->min_size) {
            // update parent and child status
            set_status(node_right(heap, node), FREE);
            set_status(node_left(heap, node), FREE);
            set_status(node, PARENT);

            grow_tree(heap, size);
        }
    }
}

void prune_tree(struct Heap* heap, uint8_t* node) {
    uint8_t* right = node_right(heap, node);
    uint8_t* left = node_left(heap, node);

    if (is_valid(heap, left) && is_valid(heap, right)) {
        prune_tree(heap, left);
        prune_tree(heap, right);

        if (status(left) == 1 && status(right) == 1) {
            set_status(node, FREE);
            set_status(left, INACTIVE);
            set_status(right, INACTIVE);
        }
    }
}
