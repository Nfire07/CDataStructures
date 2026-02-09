#include <stdio.h>
#include <time.h>
#include "include/strings.h"

/*
    C Data Structures:
        - File(text - structured)
        - Stack
        - Queue
        - List
        - Tree
        - Map
        - Graph
*/
#define NUM_ELEMENTS 1000000000

int main(void) {
    printf("Avvio benchmark C (Custom Array)...\n");
    clock_t start = clock();

    Array arr = array(sizeof(int));

    for (int i = 0; i < NUM_ELEMENTS; i++) {
        arrayAdd(arr, &i);
    }

    long long sum = 0;
    for (int i = 0; i < NUM_ELEMENTS; i++) {
        int* valPtr = arrayGetRef(arr, i);
        if (valPtr) {
            sum += *valPtr;
        }
    }

    arrayFree(arr);

    clock_t end = clock();
    double time_spent = (double)(end - start) / CLOCKS_PER_SEC;

    printf("Somma (check): %lld\n", sum);
    printf("Tempo totale C: %f s\n", time_spent);

    return 0;
}