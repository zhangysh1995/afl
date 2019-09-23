//
// Created by zhangysh1995 on 9/17/19.
//


#include "edge.h"
#include <stdio.h>
// create
struct Edge *new_edge(uint32_t hash) {
    struct Edge* edge;
    edge = malloc(sizeof(struct Edge));
    edge->hash = hash;
    edge->count = 1;

    return edge;
}

// add an element
void add_edge(struct Edge *head, struct Edge *e) {
    HASH_ADD_INT(head, hash, e);
}

// looking up the Edge
struct Edge *find_edge(struct Edge *head, uint32_t hash) {
    struct Edge *e;

    HASH_FIND_INT(head, &hash, e);
    return e;
}

// update count
void update_count(struct Edge *head, uint32_t hash) {
    struct Edge *e = find_edge(head, hash);
    e->count++;
}

//// replace the item
//struct Edge *update_edge(struct Edge *head, int hash, struct Edge *e) {
//    struct Edge *s;
//
//    // `s` points to the deleted element
//    HASH_REPLACE_INT(head, hash, e, s);
//    return s;
//}

// delete the Edge
void delete_edge(struct Edge *head, struct Edge *e) {
    HASH_DEL(head, e);
    free(e);
}

// clean the structure
void delete_all(struct Edge *head) {
    struct Edge *current, *tmp;

    HASH_ITER(hh, head, current, tmp) {
        HASH_DEL(head, current);
        free(current);
    }
}

// all-in-one
void checkThenUpdate(struct Edge *head, uint32_t hash) {
    head = NULL;
//    printf("checkThenUpdate %ld\n", head);
//
//    if (head != NULL) {
//        printf("  head is not null\n");
//    }


    if(find_edge(head, hash) != NULL)
        update_count(head, hash);
    else
        add_edge(head, new_edge(hash));
}
