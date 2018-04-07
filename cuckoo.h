#ifndef CUCKOO_H
#define CUCKOO_H

#include <stdint.h>

typedef int (*hash_func)(int, int);
struct bucket {
    int key;
    int value;
};

struct cuckoo_hash
{
    uint32_t count;
    uint32_t num_buckets;
    hash_func func1;
    hash_func func2;
    int ekey;
    int evalue;
    struct bucket **buckets;
};


void cuckoo_hash_init(struct cuckoo_hash *, hash_func func1, hash_func func2, uint32_t num_entries);
void cuckoo_hash_insert_key(struct cuckoo_hash *h, int key, int value);
#endif  /* CUCKOO_H */
