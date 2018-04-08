#include <assert.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "cuckoo.h"

// ID for first table
#define FIRST_BUCKET_ID 0
// ID for second table
#define SECOND_BUCKET_ID 1
// Key value of 0 indicate bucket is empty
#define EMPTY_BUCKET 0
// Total number of tables
#define TOTAL_BUCKET_ID 2
// Once we get into a loop increase table size this will change the hash function
#define SIZE_INCREASE 2

void
cuckoo_hash_init(struct cuckoo_hash *h, hash_func func1, hash_func func2, uint32_t num_entries) {
    memset(h, 0, sizeof(*h));
    
    h->buckets = (struct bucket **)calloc(2, sizeof(*h->buckets));
    assert(h->buckets != NULL);
    
    h->buckets[FIRST_BUCKET_ID] = (struct bucket *)calloc(num_entries, sizeof(struct bucket));
    assert(h->buckets[FIRST_BUCKET_ID] != NULL);
    
    h->buckets[SECOND_BUCKET_ID] = (struct bucket *)calloc(num_entries, sizeof(struct bucket));
    assert(h->buckets[FIRST_BUCKET_ID] != NULL);
    
    h->num_buckets = num_entries;
    h->func1 = func1;
    h->func2 = func2;
}

void
cuckoo_hash_destroy(struct cuckoo_hash *h) {
    free(h->buckets[FIRST_BUCKET_ID]);
    free(h->buckets[SECOND_BUCKET_ID]);
    free(h->buckets);

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
        hash = h->func1(key, h);
        break;
    case SECOND_BUCKET_ID:
        hash = h->func2(key, h);
        break;
    }
    return hash;
}

// Get the entry from first table
struct bucket *
cuckoo_hash_get_first_entry(struct cuckoo_hash *h, int key) {
    struct bucket *entry;
    int idx;

    idx = hash_for_id(h, FIRST_BUCKET_ID, key);
    entry = cuckoo_hash_get_entry(h, FIRST_BUCKET_ID, idx);
    return entry;
}

// Get the entry from second table
struct bucket *
cuckoo_hash_get_second_entry(struct cuckoo_hash *h, int key) {
    struct bucket *entry;
    int idx;

    idx = hash_for_id(h, SECOND_BUCKET_ID, key);
    entry = cuckoo_hash_get_entry(h, SECOND_BUCKET_ID, idx);
    return entry;
}

// Check if entry already present and we need to update
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
add_entry(struct cuckoo_hash *h, int id, int idx, int key, int value) {
    struct bucket *entry;

    if (key == EMPTY_BUCKET) {
        if (!h->zeroidset) {
            h->zeroidset = true;
            h->zeroidvalue = value;
            return true;
        }
    }

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

        if (add_entry(h, id, idx,  key, value)) {
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
    h->evicted = true;
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

static bool
rehash(struct cuckoo_hash **h) {
    struct cuckoo_hash *new_hash;
    struct bucket *old_entry, *new_entry;
    int key, value, idx;
    
    new_hash = (struct cuckoo_hash *)calloc(1, sizeof(*new_hash));
    
    new_hash->num_buckets = (*h)->num_buckets + SIZE_INCREASE;
     
    new_hash->buckets = (struct bucket **)calloc(2, sizeof(*new_hash->buckets));
    assert(new_hash->buckets != NULL);
    
    new_hash->buckets[FIRST_BUCKET_ID] = (struct bucket *)calloc(new_hash->num_buckets, sizeof(struct bucket));
    assert(new_hash->buckets[FIRST_BUCKET_ID] != NULL);
    
    new_hash->buckets[SECOND_BUCKET_ID] = (struct bucket *)calloc(new_hash->num_buckets, sizeof(struct bucket));
    assert(new_hash->buckets[FIRST_BUCKET_ID] != NULL);
    
    new_hash->func1 = (*h)->func1;
    new_hash->func2 = (*h)->func2;

    for (int j=0; j<TOTAL_BUCKET_ID; j++) {
        for(int i=0; i < (*h)->num_buckets; i++) {
            old_entry = &((*h)->buckets[j][i]);
            if (old_entry->key != EMPTY_BUCKET) {
                new_entry = cuckoo_hash_get_first_entry(new_hash, old_entry->key);
                if (new_entry->key == EMPTY_BUCKET) {
                    cuckoo_hash_update_bucket(new_entry, old_entry->key, old_entry->value);
                    continue;
                }
                new_entry = cuckoo_hash_get_second_entry(new_hash, old_entry->key);

                if (new_entry->key == EMPTY_BUCKET) {
                    cuckoo_hash_update_bucket(new_entry, old_entry->key, old_entry->value);
                    continue;
                }
                idx = (*h)->func1(old_entry->key, *h);
                if(greedy_insert(new_hash, old_entry->key, old_entry->value, idx)) {
                    continue;
                }
            }
        }   
    }
    if ((*h)->evicted) {
        key = (*h)->ekey;
        value = (*h)->evalue;
        new_entry = cuckoo_hash_get_first_entry(new_hash, key);
        if (new_entry->key == EMPTY_BUCKET) {
            cuckoo_hash_update_bucket(new_entry, key, value);
            goto success;
        }

        new_entry = cuckoo_hash_get_second_entry(new_hash, key);
        if (new_entry->key == EMPTY_BUCKET) {
            cuckoo_hash_update_bucket(new_entry, key, value);
            goto success;
        }
    
        idx = (*h)->func1(key, *h);
        if (greedy_insert(new_hash, key, value, idx)) {
            goto success;
        }
    }
    // we still have a left over entry...
    if ((*h)->evicted) {
        new_hash->evicted = true;
        new_hash->ekey = key;
        new_hash->evalue = value;
    }
    cuckoo_hash_destroy(*h);
    *h = new_hash;
    // keep rehashing...
    if (rehash(&new_hash)) {
        *h = new_hash;
        return true;
    }

success:
    cuckoo_hash_destroy(*h);
    *h = new_hash;
    return true;

}

struct cuckoo_hash *
cuckoo_hash_insert_key(struct cuckoo_hash *h, int key, int value) {
    // As key with 0 value indicates empty slot. key with zero value are
    // handled as a special entry. 

    if (key == 0) {
        h->zeroidset = true;
        h->zeroidvalue = value;
        return h;
    }
    // try to run hash only once and resuse value looks ugly but effecient
    // for CPU intensive hashes    
    int idx1 = h->func1(key, h);
    int idx2 = h->func2(key, h);

    while (true) {
        if (insert(h, key, value, idx1, idx2)) {
            return h;
        }
        if (rehash(&h)) {
            return h;
        }

    }
}

bool
cuckoo_hash_delete_key(struct cuckoo_hash *h, int key) {
    struct bucket *entry;

    if (key == EMPTY_BUCKET) {
        if (!h->zeroidset) {
            return false;
        } else {
            h->zeroidset = false;
            return true;
        }
    }

    entry = cuckoo_hash_get_first_entry(h, key);
    if (entry->key == key) {
        cuckoo_hash_update_bucket(entry, EMPTY_BUCKET, 0);
        return true;
    }

    entry = cuckoo_hash_get_second_entry(h, key);
    if (entry->key == key) {
        cuckoo_hash_update_bucket(entry, EMPTY_BUCKET, 0);
        return true;
    }

    return false;
}

bool
cuckoo_hash_lookup(struct cuckoo_hash *h, int key, int *value) {
    struct bucket *entry;

    if (key == 0) {
        if (h->zeroidset) {
            *value = h->zeroidvalue;
            return true;
        }

        return false;
    }
    entry = cuckoo_hash_get_first_entry(h, key);
    if (entry->key == key) {
        *value = entry->value;
        return true;
    }
    entry = cuckoo_hash_get_second_entry(h, key);
    if (entry->key == key) {
        *value = entry->value;
        return true;
    }
    return false;
}

void
dump_hash(struct cuckoo_hash *h) {
    struct bucket *entry;
    printf("table    slot       key         value\n");
    for (int j=0; j<TOTAL_BUCKET_ID; j++) {
        for(int i=0; i < h->num_buckets; i++) {
            entry = &(h->buckets[j][i]);
            if (entry->key != EMPTY_BUCKET) {
                printf("%-8d   %-6d   %-10d   %-7d\n", j, i, entry->key, entry->value);
            }
        }
    }
    if (h->zeroidset) {
        printf("\n Value of Key 0: %d\n", h->zeroidvalue);
    }
}

