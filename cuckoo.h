#ifndef CUCKOO_H
#define CUCKOO_H

#include <stdint.h>

struct cuckoo_hash;

typedef int (*hash_func)(int, struct cuckoo_hash *);

struct bucket {
    int key;
    int value;
};

struct cuckoo_hash
{
    uint32_t count;
    int num_buckets;
    hash_func func1;
    hash_func func2;
    bool evicted;
    int ekey;
    int evalue;
    bool zeroidset;
    bool zeroidvalue;
    struct bucket **buckets;
};


void cuckoo_hash_init(struct cuckoo_hash *, hash_func func1, hash_func func2, uint32_t num_entries);
struct cuckoo_hash * cuckoo_hash_insert_key(struct cuckoo_hash *h, int key, int value);
struct bucket *cuckoo_hash_get_first_entry(struct cuckoo_hash *h, int key);
struct bucket *cuckoo_hash_get_second_entry(struct cuckoo_hash *h, int key);
bool cuckoo_hash_lookup(struct cuckoo_hash *h, int key, int *value);
void dump_hash(struct cuckoo_hash *h);
void cuckoo_hash_destroy(struct cuckoo_hash *h);
#endif  /* CUCKOO_H */
