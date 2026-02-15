#include <stdlib.h>
#include <string.h>
#include "../include/trees.h"
#include "../include/pointers.h"


static TreeNode* _nodeNew(void* data, size_t esize) {
    TreeNode* n = (TreeNode*)xMalloc(sizeof(TreeNode));
    n->data = xMalloc(esize);
    memcpy(n->data, data, esize);
    n->left = NULL;
    n->right = NULL;
    return n;
}

static void _nodeFree(TreeNode* n) {
    if (!n) return;
    _nodeFree(n->left);
    _nodeFree(n->right);
    xFree(n->data);
    xFree(n);
}

static int _treeHeightRecursive(TreeNode* n) {
    if (!n) return 0;
    int l = _treeHeightRecursive(n->left);
    int r = _treeHeightRecursive(n->right);
    return (l > r ? l : r) + 1;
}

static void _treeToArrRecursive(TreeNode* n, Array arr, int mode) {
    if (!n) return;
    if (mode == 1) arrayAdd(arr, n->data);
    _treeToArrRecursive(n->left, arr, mode);
    if (mode == 0) arrayAdd(arr, n->data);
    _treeToArrRecursive(n->right, arr, mode);
    if (mode == 2) arrayAdd(arr, n->data);
}

Tree tree(size_t esize, int (*cmp)(const void*, const void*)) {
    Tree t = (Tree)xMalloc(sizeof(TreeStruct));
    t->root = NULL;
    t->esize = esize;
    t->count = 0;
    t->cmp = cmp;
    return t;
}

void treeInsert(Tree t, void* data) {
    if (!t) return;
    
    if (t->root == NULL) {
        t->root = _nodeNew(data, t->esize);
        t->count++;
        return;
    }

    TreeNode* current = t->root;
    while (true) {
        int r = t->cmp(data, current->data);
        if (r < 0) {
            if (current->left == NULL) {
                current->left = _nodeNew(data, t->esize);
                t->count++;
                break;
            }
            current = current->left;
        } else if (r > 0) {
            if (current->right == NULL) {
                current->right = _nodeNew(data, t->esize);
                t->count++;
                break;
            }
            current = current->right;
        } else {
            break;
        }
    }
}

bool treeContains(Tree t, void* data) {
    return treeSearch(t, data) != NULL;
}

void* treeSearch(Tree t, void* data) {
    if (!t || !t->root) return NULL;
    TreeNode* current = t->root;
    while (current) {
        int r = t->cmp(data, current->data);
        if (r == 0) return current->data;
        else if (r < 0) current = current->left;
        else current = current->right;
    }
    return NULL;
}

static TreeNode* _findMin(TreeNode* n) {
    while (n->left != NULL) n = n->left;
    return n;
}

static TreeNode* _deleteNode(TreeNode* root, void* data, size_t esize, int (*cmp)(const void*, const void*), bool* decreased) {
    if (root == NULL) return root;

    int r = cmp(data, root->data);

    if (r < 0) {
        root->left = _deleteNode(root->left, data, esize, cmp, decreased);
    } else if (r > 0) {
        root->right = _deleteNode(root->right, data, esize, cmp, decreased);
    } else {
        *decreased = true;
        if (root->left == NULL) {
            TreeNode* temp = root->right;
            xFree(root->data);
            xFree(root);
            return temp;
        } else if (root->right == NULL) {
            TreeNode* temp = root->left;
            xFree(root->data);
            xFree(root);
            return temp;
        }
        
        TreeNode* temp = _findMin(root->right);
        memcpy(root->data, temp->data, esize);
        bool dummy;
        root->right = _deleteNode(root->right, temp->data, esize, cmp, &dummy);
    }
    return root;
}

void treeRemove(Tree t, void* data) {
    if (!t || !t->root) return;
    bool decreased = false;
    t->root = _deleteNode(t->root, data, t->esize, t->cmp, &decreased);
    if (decreased) t->count--;
}

void treeClear(Tree t) {
    if (!t) return;
    _nodeFree(t->root);
    t->root = NULL;
    t->count = 0;
}

void treeFree(Tree t) {
    if (!t) return;
    treeClear(t);
    xFree(t);
}

size_t treeSize(Tree t) {
    return t ? t->count : 0;
}

size_t treeHeight(Tree t) {
    return t ? _treeHeightRecursive(t->root) : 0;
}

void* treeMin(Tree t) {
    if (!t || !t->root) return NULL;
    TreeNode* cur = t->root;
    while(cur->left) cur = cur->left;
    return cur->data;
}

void* treeMax(Tree t) {
    if (!t || !t->root) return NULL;
    TreeNode* cur = t->root;
    while(cur->right) cur = cur->right;
    return cur->data;
}

Array treeToInOrder(Tree t) {
    if (!t) return NULL;
    Array arr = array(t->esize);
    _treeToArrRecursive(t->root, arr, 0);
    return arr;
}

Array treeToPreOrder(Tree t) {
    if (!t) return NULL;
    Array arr = array(t->esize);
    _treeToArrRecursive(t->root, arr, 1);
    return arr;
}

Array treeToPostOrder(Tree t) {
    if (!t) return NULL;
    Array arr = array(t->esize);
    _treeToArrRecursive(t->root, arr, 2);
    return arr;
}