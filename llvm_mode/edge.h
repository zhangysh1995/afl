//
// Created by zhangysh1995 on 9/17/19.
//

#ifndef AFL_HT_edge_H
#define AFL_HT_edge_H


#include "uthash.h"


struct Edge {
    int hash; // key, the hash of the Edge
    int count; // value

    UT_hash_handle hh;
};
struct Edge *map = NULL;

// create
struct Edge *new_edge(int hash) {
    struct Edge* edge = (struct Edge *)malloc(sizeof(struct Edge));
    *edge = (struct Edge){hash, 1};
    return edge;
}

// add an element
void add_edge(struct Edge *e) {
    HASH_ADD_INT(map, hash, e);
}

// looking up the Edge
struct Edge *find_edge(int hash) {
    struct Edge *e;

    HASH_FIND_INT(map, &hash, e);
    return e;
}

// update count
void update_count(int hash) {
    struct Edge *e = find_edge(hash);
    e->count++;
}

// replace the item
struct Edge *update_edge(int hash, struct Edge *e) {
    struct Edge *s;

    // `s` points to the deleted element
    HASH_REPLACE_INT(map, hash, e, s);
    return s;
}

// delete the Edge
void delete_edge(struct Edge *e) {
    HASH_DEL(map, e);
    free(e);
}

// clean the structure
void delete_all() {
    struct Edge *current, *tmp;

    HASH_ITER(hh, map, current, tmp) {
        HASH_DEL(map, current);
        free(current);
    }
}

#endif //AFL_HT_edge_H
