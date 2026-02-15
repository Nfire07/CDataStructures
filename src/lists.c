#include "../include/lists.h"
#include "../include/pointers.h"
#include <string.h>

SLinkedList sLinkedList(size_t esize) {
    SLinkedList list;
    list.head = NULL;
    list.tail = NULL;
    list.esize = esize;
    list.len = 0;
    return list;
}

void sLinkedListPushFront(SLinkedList* list, void* value) {
    SLinkedListNode* node = xMalloc(sizeof(SLinkedListNode));
    node->data = xMalloc(list->esize);
    memcpy(node->data, value, list->esize);
    node->next = list->head;
    list->head = node;
    if (list->tail == NULL)
        list->tail = node;
    list->len++;
}

void sLinkedListPushBack(SLinkedList* list, void* value) {
    SLinkedListNode* node = xMalloc(sizeof(SLinkedListNode));
    node->data = xMalloc(list->esize);
    memcpy(node->data, value, list->esize);
    node->next = NULL;
    if (list->tail == NULL) {
        list->head = node;
        list->tail = node;
    } else {
        list->tail->next = node;
        list->tail = node;
    }
    list->len++;
}

void sLinkedListInsertAt(SLinkedList* list, void* value, size_t index) {
    if (index == 0) {
        sLinkedListPushFront(list, value);
        return;
    }
    if (index >= list->len) {
        sLinkedListPushBack(list, value);
        return;
    }
    SLinkedListNode* current = list->head;
    for (size_t i = 0; i < index - 1; i++)
        current = current->next;
    SLinkedListNode* node = xMalloc(sizeof(SLinkedListNode));
    node->data = xMalloc(list->esize);
    memcpy(node->data, value, list->esize);
    node->next = current->next;
    current->next = node;
    list->len++;
}

void* sLinkedListGet(SLinkedList* list, size_t index) {
    if (index >= list->len)
        return NULL;
    SLinkedListNode* current = list->head;
    for (size_t i = 0; i < index; i++)
        current = current->next;
    return current->data;
}

void sLinkedListFree(SLinkedList* list, void (*freeFn)(void*)) {
    SLinkedListNode* current = list->head;
    while (current) {
        SLinkedListNode* tmp = current;
        current = current->next;
        if (freeFn != NULL) {
            freeFn(tmp->data);
        }
        xFree(tmp->data);
        xFree(tmp);
    }
    list->head = NULL;
    list->tail = NULL;
    list->len = 0;
}

Array sLinkedListToArray(SLinkedList* list) {
    Array arr = array(list->esize);
    SLinkedListNode* current = list->head;
    while (current != NULL) {
        arrayAdd(arr, current->data);
        current = current->next;
    }
    return arr;
}

void* sLinkedListGetMiddle(SLinkedList* list) {
    if (list->head == NULL) return NULL;
    SLinkedListNode* slow = list->head;
    SLinkedListNode* fast = list->head;
    while (fast != NULL && fast->next != NULL) {
        slow = slow->next;
        fast = fast->next->next;
    }
    return slow->data;
}

void sLinkedListReverse(SLinkedList* list) {
    if (list->head == NULL || list->head->next == NULL) return;
    SLinkedListNode* prev = NULL;
    SLinkedListNode* current = list->head;
    SLinkedListNode* next = NULL;
    list->tail = list->head;
    while (current != NULL) {
        next = current->next;
        current->next = prev;
        prev = current;
        current = next;
    }
    list->head = prev;
}

DLinkedList dLinkedList(size_t esize) {
    DLinkedList list;
    list.head = NULL;
    list.tail = NULL;
    list.esize = esize;
    list.len = 0;
    return list;
}

void dLinkedListPushFront(DLinkedList* list, void* value) {
    DLinkedListNode* node = xMalloc(sizeof(DLinkedListNode));
    node->data = xMalloc(list->esize);
    memcpy(node->data, value, list->esize);
    node->previous = NULL;
    node->next = list->head;
    if (list->head)
        list->head->previous = node;
    else
        list->tail = node;
    list->head = node;
    list->len++;
}

void dLinkedListPushBack(DLinkedList* list, void* value) {
    DLinkedListNode* node = xMalloc(sizeof(DLinkedListNode));
    node->data = xMalloc(list->esize);
    memcpy(node->data, value, list->esize);
    node->next = NULL;
    node->previous = list->tail;
    if (list->tail)
        list->tail->next = node;
    else
        list->head = node;
    list->tail = node;
    list->len++;
}

void dLinkedListInsertAt(DLinkedList* list, void* value, size_t index) {
    if (index == 0) {
        dLinkedListPushFront(list, value);
        return;
    }
    if (index >= list->len) {
        dLinkedListPushBack(list, value);
        return;
    }
    DLinkedListNode* current;
    if (index < list->len / 2) {
        current = list->head;
        for (size_t i = 0; i < index; i++)
            current = current->next;
    } else {
        current = list->tail;
        for (size_t i = list->len - 1; i > index; i--)
            current = current->previous;
    }
    DLinkedListNode* node = xMalloc(sizeof(DLinkedListNode));
    node->data = xMalloc(list->esize);
    memcpy(node->data, value, list->esize);
    node->previous = current->previous;
    node->next = current;
    current->previous->next = node;
    current->previous = node;
    list->len++;
}

void* dLinkedListGet(DLinkedList* list, size_t index) {
    if (index >= list->len)
        return NULL;
    DLinkedListNode* current;
    if (index < list->len / 2) {
        current = list->head;
        for (size_t i = 0; i < index; i++)
            current = current->next;
    } else {
        current = list->tail;
        for (size_t i = list->len - 1; i > index; i--)
            current = current->previous;
    }
    return current->data;
}

void dLinkedListFree(DLinkedList* list, void (*freeFn)(void*)) {
    DLinkedListNode* current = list->head;
    while (current) {
        DLinkedListNode* tmp = current;
        current = current->next;
        if (freeFn != NULL) {
            freeFn(tmp->data);
        }
        xFree(tmp->data);
        xFree(tmp);
    }
    list->head = NULL;
    list->tail = NULL;
    list->len = 0;
}

Array dLinkedListToArray(DLinkedList* list) {
    Array arr = array(list->esize);
    DLinkedListNode* current = list->head;
    while (current != NULL) {
        arrayAdd(arr, current->data);
        current = current->next;
    }
    return arr;
}

void* dLinkedListGetMiddle(DLinkedList* list) {
    if (list->head == NULL) return NULL;
    DLinkedListNode* slow = list->head;
    DLinkedListNode* fast = list->head;
    while (fast != NULL && fast->next != NULL) {
        slow = slow->next;
        fast = fast->next->next;
    }
    return slow->data;
}

void dLinkedListReverse(DLinkedList* list) {
    if (list->head == NULL) return;
    DLinkedListNode* current = list->head;
    DLinkedListNode* temp = NULL;
    while (current != NULL) {
        temp = current->previous;
        current->previous = current->next;
        current->next = temp;
        current = current->previous;
    }
    if (temp != NULL) {
        list->tail = list->head;
        list->head = temp->previous;
    }
}

void sLinkedListRemoveAt(SLinkedList* list, size_t index) {
    if (index >= list->len) return;

    SLinkedListNode* toDelete = NULL;

    if (index == 0) {
        toDelete = list->head;
        list->head = list->head->next;
        if (list->len == 1) list->tail = NULL;
    } else {
        SLinkedListNode* current = list->head;
        for (size_t i = 0; i < index - 1; i++) {
            current = current->next;
        }
        toDelete = current->next;
        current->next = toDelete->next;
        if (index == list->len - 1) {
            list->tail = current;
        }
    }

    xFree(toDelete->data);
    xFree(toDelete);
    list->len--;
}

int sLinkedListIndexOf(SLinkedList* list, void* value, ListComparator cmp) {
    SLinkedListNode* current = list->head;
    int index = 0;
    while (current != NULL) {
        if (cmp(current->data, value) == 0) {
            return index;
        }
        current = current->next;
        index++;
    }
    return -1;
}

void* sLinkedListPopFront(SLinkedList* list) {
    if (list->len == 0) return NULL;
    
    SLinkedListNode* toDelete = list->head;
    void* data = toDelete->data; 
    
    list->head = list->head->next;
    if (list->len == 1) list->tail = NULL;
    
    xFree(toDelete); 
    list->len--;
    
    return data; 
}

void dLinkedListRemoveAt(DLinkedList* list, size_t index) {
    if (index >= list->len) return;

    DLinkedListNode* toDelete;

    if (index == 0) {
        toDelete = list->head;
        list->head = list->head->next;
        if (list->head) list->head->previous = NULL;
        else list->tail = NULL;
    } else if (index == list->len - 1) {
        toDelete = list->tail;
        list->tail = list->tail->previous;
        if (list->tail) list->tail->next = NULL;
        else list->head = NULL;
    } else {
        if (index < list->len / 2) {
            toDelete = list->head;
            for (size_t i = 0; i < index; i++) toDelete = toDelete->next;
        } else {
            toDelete = list->tail;
            for (size_t i = list->len - 1; i > index; i--) toDelete = toDelete->previous;
        }
        toDelete->previous->next = toDelete->next;
        toDelete->next->previous = toDelete->previous;
    }

    xFree(toDelete->data);
    xFree(toDelete);
    list->len--;
}

int dLinkedListIndexOf(DLinkedList* list, void* value, ListComparator cmp) {
    DLinkedListNode* current = list->head;
    int index = 0;
    while (current != NULL) {
        if (cmp(current->data, value) == 0) {
            return index;
        }
        current = current->next;
        index++;
    }
    return -1;
}

void* dLinkedListPopFront(DLinkedList* list) {
    if (list->len == 0) return NULL;

    DLinkedListNode* toDelete = list->head;
    void* data = toDelete->data;

    list->head = list->head->next;
    if (list->head) list->head->previous = NULL;
    else list->tail = NULL;

    xFree(toDelete);
    list->len--;

    return data;
}

static SLinkedListNode* sListMerge(SLinkedListNode* a, SLinkedListNode* b, int (*cmp)(const void*, const void*)) {
    if (!a) return b;
    if (!b) return a;
    
    SLinkedListNode* result = NULL;
    if (cmp(a->data, b->data) <= 0) {
        result = a;
        result->next = sListMerge(a->next, b, cmp);
    } else {
        result = b;
        result->next = sListMerge(a, b->next, cmp);
    }
    return result;
}

static void sListSplit(SLinkedListNode* source, SLinkedListNode** front, SLinkedListNode** back) {
    SLinkedListNode* fast;
    SLinkedListNode* slow;
    slow = source;
    fast = source->next;
    while (fast != NULL) {
        fast = fast->next;
        if (fast != NULL) {
            slow = slow->next;
            fast = fast->next;
        }
    }
    *front = source;
    *back = slow->next;
    slow->next = NULL;
}

static void sListMergeSort(SLinkedListNode** headRef, int (*cmp)(const void*, const void*)) {
    SLinkedListNode* head = *headRef;
    SLinkedListNode* a;
    SLinkedListNode* b;
    if ((head == NULL) || (head->next == NULL)) return;
    sListSplit(head, &a, &b);
    sListMergeSort(&a, cmp);
    sListMergeSort(&b, cmp);
    *headRef = sListMerge(a, b, cmp);
}

SLinkedList arrayToSLinkedList(Array arr) {
    SLinkedList list = sLinkedList(arr->esize);
    if (arr->len == 0) return list;

    char* ptr = (char*)arr->data;
    SLinkedListNode* node = xMalloc(sizeof(SLinkedListNode));
    node->data = xMalloc(arr->esize);
    memcpy(node->data, ptr, arr->esize);
    node->next = NULL;
    
    list.head = node;
    list.tail = node;
    list.len++;
    for (size_t i = 1; i < arr->len; i++) {
        ptr += arr->esize;
        SLinkedListNode* newNode = xMalloc(sizeof(SLinkedListNode));
        newNode->data = xMalloc(arr->esize);
        memcpy(newNode->data, ptr, arr->esize);
        newNode->next = NULL;
        
        list.tail->next = newNode;
        list.tail = newNode;
        list.len++;
    }
    return list;
}

void sLinkedListSort(SLinkedList* list, int (*cmp)(const void*, const void*)) {
    if (list->len < 2) return;
    
    sListMergeSort(&(list->head), cmp);
    SLinkedListNode* current = list->head;
    while (current->next != NULL) {
        current = current->next;
    }
    list->tail = current;
}

static DLinkedListNode* dListMerge(DLinkedListNode* a, DLinkedListNode* b, int (*cmp)(const void*, const void*)) {
    if (!a) return b;
    if (!b) return a;
    
    if (cmp(a->data, b->data) <= 0) {
        a->next = dListMerge(a->next, b, cmp);
        if (a->next) a->next->previous = a;
        a->previous = NULL;
        return a;
    } else {
        b->next = dListMerge(a, b->next, cmp);
        if (b->next) b->next->previous = b;
        b->previous = NULL;
        return b;
    }
}

static void dListSplit(DLinkedListNode* source, DLinkedListNode** front, DLinkedListNode** back) {
    DLinkedListNode* fast;
    DLinkedListNode* slow;
    slow = source;
    fast = source->next;
    while (fast != NULL) {
        fast = fast->next;
        if (fast != NULL) {
            slow = slow->next;
            fast = fast->next;
        }
    }
    *front = source;
    *back = slow->next;
    slow->next = NULL;
}

static void dListMergeSort(DLinkedListNode** headRef, int (*cmp)(const void*, const void*)) {
    DLinkedListNode* head = *headRef;
    DLinkedListNode* a;
    DLinkedListNode* b;
    if ((head == NULL) || (head->next == NULL)) return;
    dListSplit(head, &a, &b);
    dListMergeSort(&a, cmp);
    dListMergeSort(&b, cmp);
    *headRef = dListMerge(a, b, cmp);
}

DLinkedList arrayToDLinkedList(Array arr) {
    DLinkedList list = dLinkedList(arr->esize);
    if (arr->len == 0) return list;

    char* ptr = (char*)arr->data;

    DLinkedListNode* node = xMalloc(sizeof(DLinkedListNode));
    node->data = xMalloc(arr->esize);
    memcpy(node->data, ptr, arr->esize);
    node->next = NULL;
    node->previous = NULL;

    list.head = node;
    list.tail = node;
    list.len++;

    for (size_t i = 1; i < arr->len; i++) {
        ptr += arr->esize;
        DLinkedListNode* newNode = xMalloc(sizeof(DLinkedListNode));
        newNode->data = xMalloc(arr->esize);
        memcpy(newNode->data, ptr, arr->esize);
        
        newNode->next = NULL;
        newNode->previous = list.tail;
        
        list.tail->next = newNode;
        list.tail = newNode;
        list.len++;
    }
    return list;
}

void dLinkedListSort(DLinkedList* list, int (*cmp)(const void*, const void*)) {
    if (list->len < 2) return;

    dListMergeSort(&(list->head), cmp);

    if (list->head) list->head->previous = NULL;
    
    DLinkedListNode* current = list->head;
    while (current->next != NULL) {
        current = current->next;
    }
    list->tail = current;
}

Stack stack(size_t esize) {
    return sLinkedList(esize);
}

void stackPush(Stack* s, void* data) {
    sLinkedListPushFront(s, data);
}

void* stackPop(Stack* s) {
    return sLinkedListPopFront(s);
}

void* stackPeek(Stack* s) {
    return sLinkedListGet(s, 0);
}

int stackIsEmpty(Stack* s) {
    return s->len == 0;
}

void stackFree(Stack* s, void (*freeFn)(void*)) {
    sLinkedListFree(s, freeFn);
}