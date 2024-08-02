#pragma once

#define uint64_bdd unsigned long int

typedef struct node
{
    uint64_bdd addrlog;
    uint64_bdd addrphys;
    struct node *left;
    struct node *right;
} node;

node *insert(node *curr, node *new);
void insert_b(node *curr, node *new);
node *create_node(uint64_bdd l, uint64_bdd p);