#ifndef LISTS_H
#define LISTS_H

#include <stddef.h>
#include "../include/arrays.h"

typedef struct SLinkedListNode{
    void* data;
    struct SLinkedListNode* next;
}SLinkedListNode;

typedef struct DLinkedListNode{
    void* data;
    struct DLinkedListNode* next;
    struct DLinkedListNode* previous;
}DLinkedListNode;

typedef struct{
    SLinkedListNode* head;
    SLinkedListNode* tail;
    size_t esize;
    size_t len;
}SLinkedList;

typedef struct{
    DLinkedListNode* head;
    DLinkedListNode* tail;
    size_t esize;
    size_t len;
}DLinkedList;

typedef SLinkedList Stack;

typedef int (*ListComparator)(const void* a, const void* b);

SLinkedList sLinkedList(size_t esize);
void sLinkedListPushFront(SLinkedList* list, void* value);
void sLinkedListPushBack(SLinkedList* list, void* value);
void sLinkedListInsertAt(SLinkedList* list, void* value, size_t index);
void* sLinkedListGet(SLinkedList* list, size_t index);
void sLinkedListFree(SLinkedList* list);
Array sLinkedListToArray(SLinkedList* list);
void* sLinkedListGetMiddle(SLinkedList* list);
void sLinkedListReverse(SLinkedList* list);
void sLinkedListRemoveAt(SLinkedList* list, size_t index);
int sLinkedListIndexOf(SLinkedList* list, void* value, ListComparator cmp);
void* sLinkedListPopFront(SLinkedList* list);
SLinkedList arrayToSLinkedList(Array arr);
void sLinkedListSort(SLinkedList* list, int (*cmp)(const void*, const void*));

DLinkedList dLinkedList(size_t esize);
void dLinkedListPushFront(DLinkedList* list, void* value);
void dLinkedListPushBack(DLinkedList* list, void* value);
void dLinkedListInsertAt(DLinkedList* list, void* value, size_t index);
void* dLinkedListGet(DLinkedList* list, size_t index);
void dLinkedListFree(DLinkedList* list);
Array dLinkedListToArray(DLinkedList* list);
void* dLinkedListGetMiddle(DLinkedList* list);
void dLinkedListReverse(DLinkedList* list);
void dLinkedListRemoveAt(DLinkedList* list, size_t index);
int dLinkedListIndexOf(DLinkedList* list, void* value, ListComparator cmp);
void* dLinkedListPopFront(DLinkedList* list);
DLinkedList arrayToDLinkedList(Array arr);
void dLinkedListSort(DLinkedList* list, int (*cmp)(const void*, const void*));

Stack stack(size_t esize);
void stackPush(Stack* s, void* data);
void* stackPop(Stack* s);
void* stackPeek(Stack* s);
int stackIsEmpty(Stack* s);
void stackFree(Stack* s);

#endif