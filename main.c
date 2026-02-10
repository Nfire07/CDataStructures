#include <stdio.h>
#include <time.h>
#include "include/arrays.h"
#include "include/strings.h"
#include "include/files.h"
#include "include/pointers.h"
#include "include/lists.h"


/*
    C Data Structures:    
        - Tree
        - Map
        - File(structured)
        - Graph
*/

void printIntSList(SLinkedList* list) {
    printf("[SList Len: %zu]: ", list->len);
    for (size_t i = 0; i < list->len; i++) {
        int* val = (int*)sLinkedListGet(list, i);
        if (val) printf("%d -> ", *val);
    }
    printf("NULL\n");
}

void printStringDList(DLinkedList* list) {
    printf("[DList Len: %zu]: ", list->len);
    for (size_t i = 0; i < list->len; i++) {
        char** val = (char**)dLinkedListGet(list, i);
        if (val) printf("'%s' <-> ", *val);
    }
    printf("NULL\n");
}

int main() {
    printf("Benchmarking the lib C (Custom Array)...\n\n");
    clock_t start = clock();
    printf("=== TEST 1: SINGLY LINKED LIST (Interi) ===\n");

    SLinkedList slist = sLinkedList(sizeof(int));

    int nums[] = {10, 50, 20, 40, 30};
    for(int i=0; i<5; i++) {
        sLinkedListPushBack(&slist, &nums[i]);
    }
    printIntSList(&slist);

    int* mid = (int*)sLinkedListGetMiddle(&slist);
    if(mid) printf("Punto medio: %d\n", *mid);

    printf("Inversione lista...\n");
    sLinkedListReverse(&slist);
    printIntSList(&slist);

    printf("Ordinamento (Merge Sort)...\n");
    sLinkedListSort(&slist, SORT_INT_ASC);
    printIntSList(&slist);

    printf("Rimozione elemento a indice 2...\n");
    sLinkedListRemoveAt(&slist, 2);
    printIntSList(&slist);

    Array sArr = sLinkedListToArray(&slist);
    printf("Convertito in Array. Elemento 0 dell'array: %d\n", *(int*)arrayGetRef(sArr, 0));
    
    arrayFree(sArr);
    sLinkedListFree(&slist);
    printf("SList liberata.\n\n");


    printf("=== TEST 2: DOUBLY LINKED LIST (Stringhe) ===\n");

    DLinkedList dlist = dLinkedList(sizeof(char*));

    char* names[] = {"Anna", "Luca", "Marco", "Giulia"};
    dLinkedListPushBack(&dlist, &names[0]);
    dLinkedListPushBack(&dlist, &names[1]);
    dLinkedListPushFront(&dlist, &names[2]);
    dLinkedListInsertAt(&dlist, &names[3], 2);
    
    printStringDList(&dlist);

    printf("Ordinamento alfabetico...\n");
    dLinkedListSort(&dlist, SORT_CSTRING_ASC);
    printStringDList(&dlist);

    printf("Stampa inversa (Tail -> Head):\n");
    DLinkedListNode* curr = dlist.tail;
    while(curr) {
        printf("'%s' <-> ", *(char**)curr->data);
        curr = curr->previous;
    }
    printf("NULL\n");

    dLinkedListFree(&dlist);
    printf("DList liberata.\n\n");


    printf("=== TEST 3: STACK (Pila LIFO) ===\n");

    Stack pila = stack(sizeof(float));

    float f1 = 1.1f, f2 = 2.2f, f3 = 3.3f;

    printf("Push: %.1f\n", f1); stackPush(&pila, &f1);
    printf("Push: %.1f\n", f2); stackPush(&pila, &f2);
    printf("Push: %.1f\n", f3); stackPush(&pila, &f3);

    float* top = (float*)stackPeek(&pila);
    printf("Peek (Top): %.1f\n", *top);

    printf("Pop (Rimuovo %.1f)\n", *(float*)stackPop(&pila));
    

    void* poppedVal = stackPop(&pila); 
    printf("Pop (Rimuovo %.1f)\n", *(float*)poppedVal);
    xFree(poppedVal); 

    printf("Stack è vuoto? %s\n", stackIsEmpty(&pila) ? "Si" : "No");

    stackFree(&pila);
    printf("Stack liberato.\n");

    clock_t end = clock();
    double time_spent = (double)(end - start) / CLOCKS_PER_SEC;

    printf("Execution time: %fs\n", time_spent);

    return 0;
}