#include <assert.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "cuckoo.h"

#define FIRST_BUCKET_ID 0
#define SECOND_BUCKET_ID 1
#define EMPTY_BUCKET 0
#define TOTAL_BUCKET_ID 2
#define DEBUG 1
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

struct bucket *
cuckoo_hash_get_first_entry(struct cuckoo_hash *h, int key) {
    struct bucket *entry;
    uint32_t idx;

    idx = hash_for_id(h, FIRST_BUCKET_ID, key);
    printf("get first %d  idx %d\n", key,  idx);
    entry = cuckoo_hash_get_entry(h, FIRST_BUCKET_ID, idx);
    return entry;
}

struct bucket *
cuckoo_hash_get_second_entry(struct cuckoo_hash *h, int key) {
    struct bucket *entry;
    uint32_t idx;

    idx = hash_for_id(h, SECOND_BUCKET_ID, key);
    printf("get second %d  idx %d\n", key,  idx);
    entry = cuckoo_hash_get_entry(h, SECOND_BUCKET_ID, idx);
    return entry;
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
add_entry(struct cuckoo_hash *h, int id, int idx, int key, int value) {
    struct bucket *entry;

    if (key == 0) {
        if (!h->zeroidset) {
            h->zeroidset = true;
            h->zeroidvalue = value;
            return true;
        }
    }

    entry = cuckoo_hash_get_entry(h, id, idx);
    printf("add_entry %d %d %d %d\n", entry->key, entry->value, idx, id);
    if (entry->key == EMPTY_BUCKET) {
        cuckoo_hash_update_bucket(entry, key, value);
        return true;
    }

    return false;
}

static bool
greedy_insert(struct cuckoo_hash *h, int key, int value, int idx) {
    int id = 0; //start with id 0
    printf("start greedy %d\n", idx);
    // We'll end up in a loop if there are multiple connected components with
    // a loop.
    for (int i = 0; i < h->num_buckets; i++) {
        struct bucket *entry;
        int ekey;
        int evalue;

        if (add_entry(h, id, idx,  key, value)) {
            printf("added %d idx %d k %d v %d\n", id,  idx, key, value);
            return true;
        }
        
        // get the current entry in the position and replace it.
        entry = cuckoo_hash_get_entry(h, id, idx);
        printf("current entry %d %d\n", entry->key, entry->value);
        ekey = entry->key;
        evalue = entry->value;

        cuckoo_hash_update_bucket(entry, key, value);
        printf("updated entry %d %d id %d\n", entry->key, entry->value, id);
        // try adding evicted entry in alternate position
        id = (id+1)%2;
        key = ekey;
        value = evalue;

        idx = hash_for_id(h, id, key);
        printf("current entry %d %d try idx %d\n", key, value, idx);
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
    printf("k%d v%d %d %d\n", key, value, idx1, idx2);
    // check if bucket in first table is free
    entry = cuckoo_hash_get_entry(h, FIRST_BUCKET_ID, idx1);
    if (entry->key == EMPTY_BUCKET) {
        cuckoo_hash_update_bucket(entry, key, value);
#ifdef DEBUG
        printf("use first entry for %d (k(%d)/v(%d)", key, entry->key, entry->value);
#endif
        return true;
    }

    // check if bucket in second table is free
    entry = cuckoo_hash_get_entry(h, SECOND_BUCKET_ID, idx2);
    if (entry->key == EMPTY_BUCKET) {
#ifdef DEBUG
        printf("use second entry for %d", key);
#endif
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
    
    new_hash->num_buckets = (*h)->num_buckets;
     
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
                printf("old entry %d %d id %d bucket %d\n", old_entry->key, old_entry->value, j, i);
                new_entry = cuckoo_hash_get_first_entry(new_hash, old_entry->key);
                if (new_entry->key == EMPTY_BUCKET) {
                    cuckoo_hash_update_bucket(new_entry, old_entry->key, old_entry->value);
                    printf("new entry first slot\n");         
                    continue;
                }

                new_entry = cuckoo_hash_get_second_entry(new_hash, old_entry->key);

                if (new_entry->key == EMPTY_BUCKET) {
                    cuckoo_hash_update_bucket(new_entry, old_entry->key, old_entry->value);
                    printf("new entry second slot\n");         
                    continue;
                }
                idx = (*h)->func1(old_entry->key, *h);
                printf("use greedy\n");
                if(greedy_insert(new_hash, old_entry->key, old_entry->value, idx)) {
                    continue;
                }
            }
        }   
    }
    if ((*h)->evicted) {
        
        key = (*h)->ekey;
        value = (*h)->evalue;
        printf("evicted entry %d %d\n", key, value);
        new_entry = cuckoo_hash_get_first_entry(new_hash, key);
        if (new_entry->key == EMPTY_BUCKET) {
            cuckoo_hash_update_bucket(new_entry, key, value);
            printf("evicted entry first slot\n");
            goto success;
        }

        new_entry = cuckoo_hash_get_second_entry(new_hash, key);
        if (new_entry->key == EMPTY_BUCKET) {
            cuckoo_hash_update_bucket(new_entry, key, value);
            printf("new evicted second slot\n");
            goto success;
        }
    
        idx = (*h)->func1(key, *h);
        if (greedy_insert(new_hash, key, value, idx)) {
            goto success;
        }
    }
    // couldn't put in the evicted entry
    if ((*h)->evicted) {
        new_hash->evicted = true;
        new_hash->ekey = key;
        new_hash->evalue = value;
    }
    cuckoo_hash_destroy(*h);
    return false;
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
    printf("idx1 %d, idx2  %d for key %d\n", idx1, idx2, key);
    while (true) {
        if (insert(h, key, value, idx1, idx2)) {
            return h;
        }
        printf("hash before rehash %p", h);
        if (rehash(&h)) {
            printf("hash after rehash %p", h);
            return h;
        }

    }
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
#ifdef DEBUG
    printf("found first entry for %d %d\n", entry->key, entry->value);
#endif
        *value = entry->value;
        return true;
    }
    printf("not found first entry for %d %d\n", entry->key, entry->value);
    entry = cuckoo_hash_get_second_entry(h, key);
    if (entry->key == key) {
#ifdef DEBUG
        printf("use first entry for %d", key);
#endif
        *value = entry->value;
        return true;
    }
    printf("not found sec entry for %d %d\n", entry->key, entry->value);
#ifdef DEBUG
        printf("entry not found for %d", key);
#endif
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
}

