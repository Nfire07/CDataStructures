#include <stdio.h>
#include <time.h>
#include "include/arrays.h"
#include "include/strings.h"
#include "include/files.h"

/*
    C Data Structures:    
        - List
        - Stack
        - Queue
        - Tree
        - Map
        - File(structured)
        - Graph
*/

int main(void) {
    printf("Benchmarking the lib C (Custom Array)...\n\n");
    clock_t start = clock();

    File f = fileOpen("main.c",false);
    if(fileException(f)) return 1;

    Array csv = fileGetCSV(f,';');

    for(size_t i = 0;i<csv->len;i++){
        Array row = *(Array*)(arrayGetRef(csv,i));
        for(size_t j=0;j<row->len;j++){
            String cell = *(String*)arrayGetRef(row,j);
            printf("%s\n",cell->data);
        }
        printf("\n");
    }


    fileClose(f);
    fileFreeCSV(csv);

    clock_t end = clock();
    double time_spent = (double)(end - start) / CLOCKS_PER_SEC;

    printf("Execution time: %fs\n", time_spent);

    return 0;
}