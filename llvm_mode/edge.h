//
// Created by zhangysh1995 on 9/22/19.
//

#ifndef AFL_HT_EDGE_H
#define AFL_HT_EDGE_H

#include "uthash.h"

struct Edge {
    int hash; // key, the hash of the Edge
    int count; // value

    UT_hash_handle hh;
};

struct Edge *new_edge(int hash);
void add_edge(struct Edge *head, struct Edge *e);
struct Edge *find_edge(struct Edge *head, int hash);
void update_count(struct Edge *head, int hash);
void delete_edge(struct Edge *head, struct Edge *e);
void delete_all(struct Edge *head);
void checkThenUpdate(struct Edge *head, uint32_t hash);

#endif //AFL_HT_EDGE_H
