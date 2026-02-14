#include <stdio.h>
#include <time.h>
#include <mysql/mysql.h>
#include "include/strings.h"
#include "include/tui.h"
#include "include/files.h"

/*
    Structures remaining
        - Tree
        - Map
        - Graph
*/

int main() {
    clock_t start = clock();

    tuiInit();

    Array myArr = array(sizeof(int));
    int a=10, b=20, c=30,d=40,e=50;
    arrayAdd(myArr, &a);
    arrayAdd(myArr, &b);
    arrayAdd(myArr, &c);
    arrayAdd(myArr, &d);
    arrayAdd(myArr, &e);

    tuiDrawArray(5, 5, myArr, tuiPrinterInt);
    
    tuiGoToXY(0, tuiGetTerminalHeight());
    tuiStyle(TUI_STYLE_BOLD);
    printf("[ANY]  EXIT");
    tuiStyle(TUI_STYLE_RESET);

    tuiUpdate();

    tuiReadKey();

    arrayFree(myArr);

    tuiClose();

    clock_t end = clock();
    double time_spent = (double)(end - start) / CLOCKS_PER_SEC;

    

    printf("Execution time: %f seconds\n", time_spent);

    return 0;
}