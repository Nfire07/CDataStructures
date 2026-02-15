#ifndef TREES_H
#define TREES_H

#include <stddef.h>
#include <stdbool.h>
#include "../include/arrays.h"

typedef struct TreeNode {
    void* data;
    struct TreeNode* left;
    struct TreeNode* right;
} TreeNode;

typedef struct TreeStruct {
    TreeNode* root;
    size_t esize;
    size_t count;
    int (*cmp)(const void*, const void*);
} TreeStruct;

typedef TreeStruct* Tree;

Tree treeCreate(size_t esize, int (*cmp)(const void*, const void*));

void treeInsert(Tree t, void* data);
bool treeContains(Tree t, void* data);
void* treeSearch(Tree t, void* data);
void treeRemove(Tree t, void* data, void (*freeFn)(void*));
void treeClear(Tree t, void (*freeFn)(void*));
void treeFree(Tree t, void (*freeFn)(void*));

size_t treeSize(Tree t);
size_t treeHeight(Tree t);
void* treeMin(Tree t);
void* treeMax(Tree t);

Array treeToInOrder(Tree t);
Array treeToPreOrder(Tree t);
Array treeToPostOrder(Tree t);

#endif