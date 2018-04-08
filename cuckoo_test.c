#include <assert.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "cuckoo.h"

static int
test_basic_func1(int key, struct cuckoo_hash *h) {
    return key;
}

static int
test_basic_func2(int key, struct cuckoo_hash *h) {
    return key + 3 % h->num_buckets;
}

static void
test_basic() {
    struct cuckoo_hash *h;
    int input[][10] = {{0, 4}, {1, 250}, {3, 153}, {4, 175}, {5,200}, {6, 167}, {7, 205}, {8, 103}, {9, 136}};

    printf("\nTEST BASIC \n");
    printf("Basic test which directly puts key in slots \n");
    printf("This test also takes care special zero value \n");
    int size = sizeof(input)/sizeof(input[0]);
    h = (struct cuckoo_hash *)calloc(1, sizeof(*h));
    cuckoo_hash_init(h, test_basic_func1, test_basic_func2, size);

    for (int i=0; i<size; i++) {
        h = cuckoo_hash_insert_key(h, input[i][0], input[i][1]);
    }
    dump_hash(h);
}

static int
test_hash_func1(int key, struct cuckoo_hash *h) {
    return key % h->num_buckets;
}

static int
test_hash_func2(int key, struct cuckoo_hash *h) {
    return key/h->num_buckets % h->num_buckets;
}

void
test_add_delete() {
    struct cuckoo_hash *h;
    int input[][10] = {{20, 120}, {50, 150}, {53, 153}, {75, 175}, {100,200}, {67, 167}, {105, 205}, {3, 103}, {36, 136}, {39, 139}};

    int size = sizeof(input)/sizeof(input[0]);
    h = (struct cuckoo_hash *)calloc(1, sizeof(*h));
    cuckoo_hash_init(h, test_hash_func1, test_hash_func2, size);

    printf("TEST ADD DELETE\n");
    for (int i=0; i<size; i++) {
        h = cuckoo_hash_insert_key(h, input[i][0], input[i][1]);
    }

    for (int i=0; i<size; i++) {
        int value;
        assert(cuckoo_hash_lookup(h, input[i][0], &value));
        assert(input[i][1] == value);
    }
    
    dump_hash(h);
    printf("Now remove all the entries\n");
    for (int i=0; i<size; i++) {
        assert(cuckoo_hash_delete_key(h, input[i][0]));
    }
    printf("Check all the entries are removed from Database\n");
    dump_hash(h);
    cuckoo_hash_destroy(h);
}

static void
test_loop() {
    struct cuckoo_hash *h;
    h = (struct cuckoo_hash *)calloc(1, sizeof(*h));
    int input[][11] ={{20, 120}, {50, 150}, {53, 153}, {75, 175}, {100,200}, {67, 167}, {105, 205}, {3, 103}, {36, 136}, {39, 139}, {6, 106}};
    int size = sizeof(input)/sizeof(input[0]);

    printf("\nTEST LOOP\n");
    printf("This test will cause a cycle when last entry is added\n");
    printf("Algorithm will rebuild the hash\n");
    cuckoo_hash_init(h, test_hash_func1, test_hash_func2, size);

    for (int i = 0; i < size; i++) {
        h = cuckoo_hash_insert_key(h, input[i][0], input[i][1]);
    }
    
    for (int i = 0; i < size; i++) {
        int value;
        assert(cuckoo_hash_lookup(h, input[i][0], &value));
    }
    dump_hash(h);
}

int
main(int argc, char **argv) {
    test_basic();
    test_add_delete();
    test_loop();
 }
