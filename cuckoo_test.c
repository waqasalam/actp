#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cuckoo.h"

int
test_hash_func1(int key, int size) {
    return key % size;
}

int
test_hash_func2(int key, int size) {
    return key/size % size;
}

int
main(int argc, char **argv) {
    struct cuckoo_hash h;
    memset(&h, 0, sizeof(h));
    cuckoo_hash_init(&h, test_hash_func1, test_hash_func2, 32);
}
