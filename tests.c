#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "virtual_sbrk.h"
#include "structure.h"
#include "virtual_alloc.h"

void* virtual_heap = NULL;
void* program_break = NULL;

void* virtual_sbrk(int32_t increment) {
    if (virtual_heap != NULL) {
        void* previous_break = program_break;
        program_break += increment;

        return previous_break;
    } else {
        fprintf(stderr, "SBRK, memory not initialised\n");
        exit(1);
    }
}

// HELPER FUNCTIONS

void address_tree(struct Heap* heap, uint8_t* node, char* prefix, int last) {
    if (is_valid(heap, node)) {
        char* current_prefix = (last ? "└─ " : "├─ ");
        char* child_prefix = (last ? "   " : "|  ");
        
        if (status(node) == ALLOC || status(node) == FREE) {
            struct Parcel parcel = { heap, 0, node };
            printf("%s%s(%d) %s %d -> %d\n",
                prefix,
                current_prefix,
                (int) node_size(heap, node),
                (status(node) == ALLOC ? "allocated" : "free"),
                (int) 1 << node_size(heap, node),
                (int) node_to_address(&heap->root, &parcel)
            );
        } else {
            printf("%s%s%d\n",
                prefix,
                current_prefix,
                (int) node_size(heap, node)
            );
        }

        char* new_prefix = malloc(100 * sizeof(char));
        strcpy(new_prefix, prefix);
        strcat(new_prefix, child_prefix);

        address_tree(heap, node_left(heap, node), new_prefix, 0);
        address_tree(heap, node_right(heap, node), new_prefix, 1);

        free(new_prefix);
    }
}

void version_tree(struct Heap* heap, uint8_t* node, char* prefix, int last) {
    if (in_tree(heap, node) && (status(node) > 0 || backup(node) > 0)) {
        char* current_prefix = (last ? "└─ " : "├─ ");
        char* child_prefix = (last ? "   " : "|  ");
        
        printf("%s%s%d -> %d\n",
            prefix,
            current_prefix,
            status(node),
            backup(node)
        );

        char* new_prefix = malloc(100 * sizeof(char));
        strcpy(new_prefix, prefix);
        strcat(new_prefix, child_prefix);

        version_tree(heap, node_left(heap, node), new_prefix, 0);
        version_tree(heap, node_right(heap, node), new_prefix, 1);

        free(new_prefix);
    }
}

int assert_temp_file(char* expected) {
    FILE* file = fopen("output_testing", "r");

    fseek(file, 0, SEEK_END);
    size_t num_chars = ftell(file);
    fseek(file, 0, SEEK_SET);

    char* buffer = malloc(num_chars + 1);
    buffer[num_chars] = '\0';

    fread(buffer, sizeof(char), num_chars, file);
    fclose(file);

    if (strcmp(buffer, expected) != 0) {
        printf("\nEXPECTED\n");
        printf("----------\n\n");
        printf("%s\n", expected);
        printf("----------------\n");
        printf("\nOUTPUT\n");
        printf("--------\n\n");
        printf("%s\n", buffer);
        printf("----------------\n");
        return 0;
    }

    free(buffer);
    return 1;
}

int assert_virtual_info(char* expected) {
    freopen("output_testing", "w", stdout);
    virtual_info(virtual_heap);
    freopen("/dev/tty", "w", stdout);
    return assert_temp_file(expected);
}

int assert_version_tree(char* expected) {
    struct Heap* heap = virtual_heap;
    freopen("output_testing", "w", stdout);
    version_tree(heap, &heap->root, "", 1);
    freopen("/dev/tty", "w", stdout);
    return assert_temp_file(expected);
}


// TEST VIRTUAL MALLOC

void malloc_assign_root() {
    printf("Assign root...\n");
    program_break = virtual_heap;

    init_allocator(virtual_heap, 15, 12);
    assert(assert_virtual_info("free 32768\n"));

    assert(virtual_malloc(virtual_heap, 1 << 15));
    assert(assert_virtual_info("allocated 32768\n"));
}

void malloc_varied_sizes() {
    printf("Varied sizes...\n");
    program_break = virtual_heap;
    struct Heap* heap = virtual_heap;
    void* storage;

    init_allocator(virtual_heap, 15, 1);
    storage = virtual_heap + overhead(heap);

    assert(virtual_malloc(virtual_heap, 4095) == storage + 0);
    assert(virtual_malloc(virtual_heap, 1948) == storage + 4096);
    assert(virtual_malloc(virtual_heap, 1500) == storage + 6144);
    assert(virtual_malloc(virtual_heap, 16300) == storage + 16384);

    assert(assert_virtual_info(
        "allocated 4096\n"
        "allocated 2048\n"
        "allocated 2048\n"
        "free 8192\n"
        "allocated 16384\n"
    ));

    program_break = virtual_heap;

    init_allocator(virtual_heap, 5, 2);
    storage = virtual_heap + overhead(heap);

    assert(virtual_malloc(virtual_heap, 7) == storage + 0);
    assert(virtual_malloc(virtual_heap, 16) == storage + 16);

    assert(assert_virtual_info(
        "allocated 8\n"
        "free 8\n"
        "allocated 16\n"
    ));
}

void malloc_splitting() {
    printf("Split nodes...\n");
    program_break = virtual_heap;

    init_allocator(virtual_heap, 10, 1);

    assert(virtual_malloc(virtual_heap, 7));
    assert(assert_virtual_info(
        "allocated 8\n"
        "free 8\n"
        "free 16\n"
        "free 32\n"
        "free 64\n"
        "free 128\n"
        "free 256\n"
        "free 512\n"
    ));

    assert(virtual_malloc(virtual_heap, 1 << 9));
    assert(assert_virtual_info(
        "allocated 8\n"
        "free 8\n"
        "free 16\n"
        "free 32\n"
        "free 64\n"
        "free 128\n"
        "free 256\n"
        "allocated 512\n"
    ));
}

void malloc_assigning() {
    printf("Can assign to virtual malloc return address...\n");
    program_break = virtual_heap;

    init_allocator(virtual_heap, 5, 1);

    char* a = virtual_malloc(virtual_heap, 1 << 4);
    char* b = virtual_malloc(virtual_heap, 1 << 4);

    // when no memory to allocate
    assert(!virtual_malloc(virtual_heap, 1 << 4));


    // test writing to memory and memory structure
    strcpy(a, "aaaaaaaaaaaaaaaaabc");
    assert(strcmp(a, "aaaaaaaaaaaaaaaaabc") == 0);
    assert(strcmp(b, "abc") == 0);
    assert(virtual_malloc(virtual_heap, 1) == NULL);
}

void malloc_lower_bound() {
    printf("Can obey lowerbound...\n");
    program_break = virtual_heap;

    init_allocator(virtual_heap, 10, 9);
    virtual_malloc(virtual_heap, 1);
    virtual_malloc(virtual_heap, 2);

    assert(assert_virtual_info(
        "allocated 512\n"
        "allocated 512\n"
    ));
}

void malloc_invalid_requests() {
    printf("Can handle invalid request...\n");
    program_break = virtual_heap;

    init_allocator(virtual_heap, 15, 12);

    // try and request block which is too big
    assert(!virtual_malloc(virtual_heap, (1 << 15) + 1));

    char* expected = "free 32768\n";
    assert(assert_virtual_info(expected));

    assert(virtual_malloc(virtual_heap, 1 << 14));
    assert(virtual_malloc(virtual_heap, 1 << 14));

    // try and request memory when none left to allocate
    assert(!virtual_malloc(virtual_heap, 5));
}

void malloc_complex() {
    printf("Can do complex malloc...\n");
    program_break = virtual_heap;

    init_allocator(virtual_heap, 15, 5);

    virtual_malloc(virtual_heap, 1 << 12);
    assert(assert_virtual_info(
        "allocated 4096\n"
        "free 4096\n"
        "free 8192\n"
        "free 16384\n"
    ));

    virtual_malloc(virtual_heap, 1 << 11);
    assert(assert_virtual_info(
        "allocated 4096\n"
        "allocated 2048\n"
        "free 2048\n"
        "free 8192\n"
        "free 16384\n"
    ));

    virtual_malloc(virtual_heap, 1 << 13);
    assert(assert_virtual_info(
        "allocated 4096\n"
        "allocated 2048\n"
        "free 2048\n"
        "allocated 8192\n"
        "free 16384\n"
    ));

    virtual_malloc(virtual_heap, 1 << 13);
    assert(assert_virtual_info(
        "allocated 4096\n"
        "allocated 2048\n"
        "free 2048\n"
        "allocated 8192\n"
        "allocated 8192\n"
        "free 8192\n"
    ));

    virtual_malloc(virtual_heap, 1 << 13);
    assert(assert_virtual_info(
        "allocated 4096\n"
        "allocated 2048\n"
        "free 2048\n"
        "allocated 8192\n"
        "allocated 8192\n"
        "allocated 8192\n"
    ));

    virtual_malloc(virtual_heap, 1 << 11);
    assert(assert_virtual_info(
        "allocated 4096\n"
        "allocated 2048\n"
        "allocated 2048\n"
        "allocated 8192\n"
        "allocated 8192\n"
        "allocated 8192\n"
    ));
}


// TEST VIRTUAL FREE

void free_simple() {
    printf("Can peform a simple request...\n");
    program_break = virtual_heap;

    struct Heap* heap = virtual_heap;
    init_allocator(virtual_heap, 18, 12);

    void* storage = virtual_heap + overhead(heap);

    virtual_malloc(virtual_heap, 1 << 18);

    assert(assert_virtual_info("allocated 262144\n"));
    assert(virtual_free(virtual_heap, storage) == 0);
    assert(assert_virtual_info("free 262144\n"));
}

void free_invalid_address() {
    printf("can handle invalid address...\n");
    program_break = virtual_heap;

    init_allocator(virtual_heap, 15, 12);

    assert(virtual_free(virtual_heap, NULL) != 0);
    assert(virtual_free(virtual_heap, virtual_heap) != 0);

    assert(assert_virtual_info("free 32768\n"));
}

void free_prune_tree() {
    printf("Can peform a complex request...\n");
    struct Heap* heap = virtual_heap;
    program_break = virtual_heap;

    init_allocator(virtual_heap, 19, 10);

    void* storage = virtual_heap + overhead(heap);

    virtual_malloc(virtual_heap, 32000);
    assert(assert_virtual_info(
        "allocated 32768\n"
        "free 32768\n"
        "free 65536\n"
        "free 131072\n"
        "free 262144\n"
    ));

    assert(virtual_free(virtual_heap, storage) == 0);
    assert(assert_virtual_info("free 524288\n"));
}


// TEST VIRTUAL REALLOC

void realloc_tree_versions() {
    printf("Version control...\n");
    struct Heap* heap = virtual_heap;
    program_break = virtual_heap;

    // CREATE INITIAL TREE

    init_allocator(virtual_heap, 19, 10);
    virtual_malloc(virtual_heap, 32000);

    assert(assert_version_tree(
        "└─ 3 -> 0"              "\n"
        "   ├─ 3 -> 0"           "\n"
        "   |  ├─ 3 -> 0"        "\n"
        "   |  |  ├─ 3 -> 0"     "\n"
        "   |  |  |  ├─ 2 -> 0"  "\n"
        "   |  |  |  └─ 1 -> 0"  "\n"
        "   |  |  └─ 1 -> 0"     "\n"
        "   |  └─ 1 -> 0"        "\n"
        "   └─ 1 -> 0"           "\n"
    ));

    // CREATE BACKUP

    backup_tree(heap, &heap->root);

    assert(assert_version_tree(
        "└─ 3 -> 3"              "\n"
        "   ├─ 3 -> 3"           "\n"
        "   |  ├─ 3 -> 3"        "\n"
        "   |  |  ├─ 3 -> 3"     "\n"
        "   |  |  |  ├─ 2 -> 2"  "\n"
        "   |  |  |  └─ 1 -> 1"  "\n"
        "   |  |  └─ 1 -> 1"     "\n"
        "   |  └─ 1 -> 1"        "\n"
        "   └─ 1 -> 1"           "\n"
    ));

    // MODIFY TREE

    virtual_free(virtual_heap, virtual_heap + overhead(heap));

    assert(assert_version_tree(
        "└─ 1 -> 3"              "\n"
        "   ├─ 0 -> 3"           "\n"
        "   |  ├─ 0 -> 3"        "\n"
        "   |  |  ├─ 0 -> 3"     "\n"
        "   |  |  |  ├─ 0 -> 2"  "\n"
        "   |  |  |  └─ 0 -> 1"  "\n"
        "   |  |  └─ 0 -> 1"     "\n"
        "   |  └─ 0 -> 1"        "\n"
        "   └─ 0 -> 1"           "\n"
    ));

    // RESTORE TREE BACKUP

    restore_tree(heap, &heap->root);

    assert(assert_version_tree(
        "└─ 3 -> 0"              "\n"
        "   ├─ 3 -> 0"           "\n"
        "   |  ├─ 3 -> 0"        "\n"
        "   |  |  ├─ 3 -> 0"     "\n"
        "   |  |  |  ├─ 2 -> 0"  "\n"
        "   |  |  |  └─ 1 -> 0"  "\n"
        "   |  |  └─ 1 -> 0"     "\n"
        "   |  └─ 1 -> 0"        "\n"
        "   └─ 1 -> 0"           "\n"
    ));
}

void realloc_simple() {
    printf("Simple request...\n");
    struct Heap* heap = virtual_heap;

    init_allocator(virtual_heap, 18, 12);
    void* storage = virtual_heap + overhead(heap);

    assert(assert_version_tree(
        "└─ 1 -> 0" "\n"
    ));


    virtual_malloc(virtual_heap, 1 << 18);

    assert(assert_version_tree(
        "└─ 2 -> 0" "\n"
    ));


    virtual_realloc(virtual_heap, storage, 8123);

    assert(assert_version_tree(
        "└─ 3 -> 2"                "\n"
        "   ├─ 3 -> 0"             "\n"
        "   |  ├─ 3 -> 0"          "\n"
        "   |  |  ├─ 3 -> 0"       "\n"
        "   |  |  |  ├─ 3 -> 0"    "\n"
        "   |  |  |  |  ├─ 2 -> 0" "\n"
        "   |  |  |  |  └─ 1 -> 0" "\n"
        "   |  |  |  └─ 1 -> 0"    "\n"
        "   |  |  └─ 1 -> 0"       "\n"
        "   |  └─ 1 -> 0"          "\n"
        "   └─ 1 -> 0"             "\n"
    ));
}

void execute(void (**funcs)(), int size, char* arg, char* msg) {
    if (strcmp(arg, "0") == 0) 
        return;

    printf("\n%s\n\n", msg);

    if (strcmp(arg, "all") == 0) {
        for (int i=0; i < size; i++) {
            (*funcs[i])();
        }
    } else {
        int test_num = atoi(arg)-1;
        if (test_num >= 0 && test_num < size) {
            (*funcs[test_num])();
        } else {
            char* msg = "Invalid number %d for tests\n\n";
            fprintf(stderr, msg, test_num);
        }
    }

    printf("\n");
}

int main(int argc, char** argv) {
    if (argc < 4) {
        char* msg = "\nRequires 3 command line arguments, got %d\n"
                    "\nExample: ./tests all all all to run all\n\n";
        fprintf(stderr, msg, argc);
        return 0;
    }

    virtual_heap = malloc(600000);

    // MALLOC TESTS

    void (*malloc_tests[])() = {
        malloc_assign_root,
        malloc_splitting,
        malloc_varied_sizes,
        malloc_assigning,
        malloc_lower_bound,
        malloc_invalid_requests,
        malloc_complex
    };

    int len = sizeof(malloc_tests)/sizeof(malloc_tests[0]);
    execute(malloc_tests, len, argv[1], "VIRTUAL MALLOC TESTING");
    
    // FREE TESTS

    void (*free_tests[])() = {
        free_simple,
        free_invalid_address,
        free_prune_tree
    };

    len = sizeof(free_tests)/sizeof(free_tests[0]);
    execute(free_tests, len, argv[2], "VIRTUAL FREE TESTING");

    // REALLOC TESTS

    void (*realloc_tests[])() = {
        realloc_tree_versions,
        realloc_simple
    };

    len = sizeof(realloc_tests)/sizeof(realloc_tests[0]);
    execute(realloc_tests, len, argv[3], "VIRTUAL REALLOC TESTING");

    // clean files and memory
    remove("output_testing");
    free(virtual_heap);

    printf("\nFinished Unit Tests\n");

    return 0;
}
