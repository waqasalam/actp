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
    int num_buckets;
    hash_func func1;
    hash_func func2;
    bool evicted;  // Indicates we have an outstanding evicted entry from
                   // previous greedy insertion.
    int ekey;      // Key associated with evicted entry.
    int evalue;    // Value associated with evicted entry.
    bool zeroidset;  // Key with value zero indicated empty slot. The actual
                     // key with zero value is stored in this location.
    int zeroidvalue;
    struct bucket **buckets;
};

// Initialize the hash datastructure.
// Hash is organized with two tables and each table has it's own hash function
void cuckoo_hash_init(struct cuckoo_hash *, hash_func func1, hash_func func2, int num_entries);

// This functions tries adding an entry to hash table.
// (1) Try first hash function.
// (2) If 1 fails, try second hash function.
// (3) If 2 fails, try greedy strategy. Evict the entry in slot associated
// with first hash function and add the new entry in it's position. Try adding
// the evicted entry in the other slot and continue (3) until all entries have
// been tried or an empty slot is found.
// (4) If empty slot still not found increase size of hash table which results
// in new hash entries and retry adding all the entries in the hash table.
struct cuckoo_hash * cuckoo_hash_insert_key(struct cuckoo_hash *h, int key, int value);

// Delete key from hash table.
bool cuckoo_hash_delete_key(struct cuckoo_hash *h, int key);

// Lookup the key in hash table 
bool cuckoo_hash_lookup(struct cuckoo_hash *h, int key, int *value);

// Dump all entries in the hash table.
void dump_hash(struct cuckoo_hash *h);

// Destroy the hash table
void cuckoo_hash_destroy(struct cuckoo_hash *h);

#endif  /* CUCKOO_H */
