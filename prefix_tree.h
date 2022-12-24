/*
 * Copyright (c) 2022, Jan de Visser <jan@de-visser.net>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#ifndef SCRATCH_PREFIX_TREE_H
#define SCRATCH_PREFIX_TREE_H

#include <stdlib.h>
#include <string.h>

#ifndef NEW
#define NEW(T)	          ((T*) malloc(sizeof(T)))
#endif

typedef struct prefix_tree_node {
    char                    *prefix;
    struct prefix_tree_node *children;
    struct prefix_tree_node *next;
    struct prefix_tree_node *prev;
} PrefixTreeNode;

typedef struct prefix_tree {
    PrefixTreeNode *head;
    PrefixTreeNode *tail;
} PrefixTree;

PrefixTree *     prefix_tree_create();
PrefixTree *     prefix_tree_insert(PrefixTree *, char *tag);
PrefixTreeNode * prefix_tree_match(PrefixTree *, char *text);

#define PREFIX_TREE_IMPL
#ifdef PREFIX_TREE_IMPL

PrefixTree * prefix_tree_create()
{
    PrefixTree *ret = NEW(PrefixTree);
    ret->head = ret->tail = NULL;
}

PrefixTree * prefix_tree_insert(PrefixTree *tree, char *tag)
{
    PrefixTreeNode *node = NEW(PrefixTreeNode);
    node->prefix = strdup(tag);
    node->children = node->next = node->prev = NULL;


    return node;
}

#endif

#endif //SCRATCH_PREFIX_TREE_H
