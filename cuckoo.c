#include <assert.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

#include "cuckoo.h"

#define FIRST_BUCKET_ID 0
#define SECOND_BUCKET_ID 1
#define EMPTY_BUCKET 0

void
cuckoo_hash_init(struct cuckoo_hash *h, hash_func func1, hash_func func2, uint32_t num_entries) {

    h->buckets = (struct bucket **)calloc(2, sizeof(*h->buckets));

    h->buckets[FIRST_BUCKET_ID] = (struct bucket *)calloc(num_entries, sizeof(struct bucket));
    h->buckets[SECOND_BUCKET_ID] = (struct bucket *)calloc(num_entries, sizeof(struct bucket));

    assert(h->buckets != NULL);
    h->num_buckets = num_entries;
    h->func1 = func1;
    h->func2 = func2;
}

static struct bucket *
cuckoo_hash_get_entry(struct cuckoo_hash *h, int id, uint32_t index) {
        return h->buckets[id] + index;
}

static void
cuckoo_hash_update_bucket(struct bucket *entry, int key, int value) {
    entry->key = key;
    entry->value = value;
}

static int
hash_for_id(struct cuckoo_hash *h, int id, int key) {
    int hash;
    switch(id) {
    case FIRST_BUCKET_ID:
        hash = h->func1(key, h->num_buckets);
    case SECOND_BUCKET_ID:
        hash = h->func2(key, h->num_buckets);
    }
    return hash;
}

static bool
check_update(struct cuckoo_hash *h, int key, int value, int idx1, int idx2) {
    struct bucket *entry;

    entry = cuckoo_hash_get_entry(h, FIRST_BUCKET_ID, idx1);
    if (entry->key == key) {
        entry->value = value;
        return true;
    }
    
    entry = cuckoo_hash_get_entry(h, SECOND_BUCKET_ID, idx2);
    if (entry->key == key) {
        entry->value = value;
        return true;
    }

    return false;
}

static bool
add_entry(struct cuckoo_hash *h, int id, int key, int value) {
    struct bucket *entry;
    uint32_t idx;

    idx = hash_for_id(h, id, key);
    entry = cuckoo_hash_get_entry(h, id, idx);

    if (entry->key == EMPTY_BUCKET) {
        cuckoo_hash_update_bucket(entry, key, value);
        return true;
    }

    return false;
}

static bool
greedy_insert(struct cuckoo_hash *h, int key, int value, int idx) {
    int id = 0; //start with id 0
    
    // We'll end up in a loop if there are multiple connected components with
    // a loop.
    for (int i = 0; i < h->num_buckets; i++) {
        struct bucket *entry;
        int ekey;
        int evalue;

        if (add_entry(h, id, key, value)) {
            return true;
        }
        // get the current entry in the position and replace it.
        entry = cuckoo_hash_get_entry(h, id, idx);
        ekey = entry->key;
        evalue = entry->value;
        cuckoo_hash_update_bucket(entry, key, value);

        // try adding evicted entry in alternate position
        id = (id+1)%2;
        key = ekey;
        value = evalue;
        idx = hash_for_id(h, id, key);
    }
    h->ekey = key;
    h->evalue = value;
    return false;
}

static bool
insert(struct cuckoo_hash *h, int key, int value, int idx1, int idx2) {
    struct bucket *entry;
    
    // check if key is already present we need to just update the value
    // in this case
    if (check_update(h, key, value, idx1, idx2)) {
        return true;
    }

    // check if bucket in first table is free
    entry = cuckoo_hash_get_entry(h, FIRST_BUCKET_ID, idx1);
    if (entry->key == EMPTY_BUCKET) {
        cuckoo_hash_update_bucket(entry, key, value);
        return true;
    }

    // check if bucket in second table is free
    entry = cuckoo_hash_get_entry(h, SECOND_BUCKET_ID, idx2);
    if (entry->key == EMPTY_BUCKET) {
        cuckoo_hash_update_bucket(entry, key, value);
        return true;
    }

    if (greedy_insert(h, key, value, idx1)) {
        return true;
    }
    
    return false;
}

void
cuckoo_hash_insert_key(struct cuckoo_hash *h, int key, int value) {
    // try to run hash only once and resuse value looks ugly but effecient
    // for CPU intensive hashes
    int idx1 = h->func1(key, value);
    int idx2 = h->func2(key, value);
    
    while (true) {
        if (insert(h, key, value, idx1, idx2)) {
            return;
        }

//        check_grow
    }
}


