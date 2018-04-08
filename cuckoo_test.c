#include <assert.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "cuckoo.h"

static int
test_hash_func1(int key, struct cuckoo_hash *h) {
    return key % h->num_buckets;
}

static int
test_hash_func2(int key, struct cuckoo_hash *h) {
    return key/h->num_buckets % h->num_buckets;
}


/*
static int
test_hash_init_func1(int key, struct cuckoo_hash *h) {
    return key;
}

static int
test_hash_init_func2(int key, struct cuckoo_hash *h) {
    int hash;
    printf("func 2 key is %d\n", key);
    if ((key-h->num_buckets/2) > 0) {
        printf("func 2 key is %d", key);
        hash = key;   
        printf("func2 one %d %d %d\n", hash, key-h->num_buckets/2, h->num_buckets/2);
    } else {
        if (key+h->num_buckets/2 < h->num_buckets) {
            hash = (key + h->num_buckets/2);
            printf("func2 two %d %d %d\n", hash, key-h->num_buckets/2, h->num_buckets/2);
        }
    }
    return hash;
    
}

void
test_hash() {
    struct cuckoo_hash h;
    int max_size = 10;
    
    cuckoo_hash_init(&h, test_hash_func1, test_hash_func2, max_size);
    

}
*/
void
test_init() {
    struct cuckoo_hash h;
    int input[][10] = {{20, 120}, {50, 150}, {53, 153}, {75, 175}, {100,200}, {67, 167}, {105, 205}, {3, 103}, {36, 136}, {39, 139}};

    int size = sizeof(input)/sizeof(input[0]);

    cuckoo_hash_init(&h, test_hash_func1, test_hash_func2, size);


    for (int i = 0; i < size; i++) {
        cuckoo_hash_insert_key(&h, input[i][0], input[i][1]);
    }

    for (int i = 0; i < size; i++) {
        int value;
        int ret = cuckoo_hash_lookup(&h, input[i][0], &value);
        printf("-----RESULTS is %d %d/%d\n", ret,  value, input[i][1]);
    }
    dump_hash(&h);
    cuckoo_hash_destroy(&h);
}

static void
test_first() {
    struct cuckoo_hash *h;
    //
    h = (struct cuckoo_hash *)calloc(1, sizeof(*h));
    int input[][10] = {{20, 120}, {50, 150}, {53, 153}, {75, 175}, {100,200}, {67, 167}, {105, 205}, {3, 103}, {36, 136}, {39, 139}};

    int size = sizeof(input)/sizeof(input[0]);

    cuckoo_hash_init(h, test_hash_func1, test_hash_func2, size);


    for (int i = 0; i < size; i++) {
        h = cuckoo_hash_insert_key(h, input[i][0], input[i][1]);
    }

    for (int i = 0; i < size; i++) {
        int value;
        int ret = cuckoo_hash_lookup(h, input[i][0], &value);
        printf("\n-----RESULTS is %d %d", ret, input[i][0]);
    }
    dump_hash(h);
    cuckoo_hash_destroy(h);
}
static void
test_second() {
    struct cuckoo_hash *h;
    h = (struct cuckoo_hash *)calloc(1, sizeof(*h));
    int input[][11] ={{20, 120}, {50, 150}, {53, 153}, {75, 175}, {100,200}, {67, 167}, {105, 205}, {3, 103}, {36, 136}, {39, 139}, {6, 106}};
    int size = sizeof(input)/sizeof(input[0]);

    cuckoo_hash_init(h, test_hash_func1, test_hash_func2, size);


    for (int i = 0; i < size; i++) {
        h = cuckoo_hash_insert_key(h, input[i][0], input[i][1]);
    }
    dump_hash(h);
    for (int i = 0; i < size; i++) {
        int value;
        int ret = cuckoo_hash_lookup(h, input[i][0], &value);
        printf("-----RESULTS is %d %d\n", ret, input[i][0]);
    }
}

int
main(int argc, char **argv) {

    //  test_first();
    test_second();
}
