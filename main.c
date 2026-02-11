#include <stdio.h>
#include <time.h>
#include "include/tui.h"
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

Array createRow(const char* id, const char* name, const char* role, const char* status) {
    Array row = array(sizeof(String));
    
    String s1 = stringNew(id);
    String s2 = stringNew(name);
    String s3 = stringNew(role);
    String s4 = stringNew(status);

    arrayAdd(row, &s1);
    arrayAdd(row, &s2);
    arrayAdd(row, &s3);
    arrayAdd(row, &s4);

    return row;
}

int main() {
    printf("Benchmarking the lib C (Custom Array)...\n\n");
    clock_t start = clock();
    
    tuiClearScreen();
    tuiCursorVisible(false);

    tuiGoToXY(5, 2);
    tuiStyle(TUI_STYLE_BOLD);
    tuiColorHEX("#00FF99");
    printf("=== SYSTEM DASHBOARD v1.0 ===");
    
    tuiGoToXY(5, 3);
    tuiStyle(TUI_STYLE_RESET);
    tuiColorRGB(150, 150, 150);
    printf("Gestione Utenti e Risorse");

    Array headers = array(sizeof(String));
    String h1 = stringNew("ID");
    String h2 = stringNew("NOME UTENTE");
    String h3 = stringNew("RUOLO");
    String h4 = stringNew("STATUS");

    arrayAdd(headers, &h1);
    arrayAdd(headers, &h2);
    arrayAdd(headers, &h3);
    arrayAdd(headers, &h4);

    Array rows = array(sizeof(Array));

    Array r1 = createRow("001", "Mario Rossi", "Admin", "ONLINE");
    Array r2 = createRow("002", "Luigi Verdi", "Developer", "OFFLINE");
    Array r3 = createRow("003", "Peach Toadstool", "Manager", "BUSY");
    Array r4 = createRow("004", "Bowser Koopa", "Security", "BANNED");

    arrayAdd(rows, &r1);
    arrayAdd(rows, &r2);
    arrayAdd(rows, &r3);
    arrayAdd(rows, &r4);

    tuiGoToXY(5, 5);
    tuiColor(TUI_CYAN);
    tuiPrintTable(headers, rows);

    tuiGoToXY(5, 14);
    tuiColorHEX("#FF5733");
    printf("Colore HEX Custom (#FF5733)");

    tuiGoToXY(5, 15);
    tuiColorRGB(100, 149, 237);
    printf("Colore RGB Cornflower Blue (100, 149, 237)");

    tuiGoToXY(5, 17);
    tuiStyle(TUI_STYLE_ITALIC);
    tuiColor(TUI_WHITE);
    printf("Premi INVIO per uscire...");

    getchar();

    arrayFree(headers); 
    arrayFree(rows);

    tuiReset();
    tuiClearScreen();
    tuiCursorVisible(true);

    clock_t end = clock();
    double time_spent = (double)(end - start) / CLOCKS_PER_SEC;

    printf("Execution time: %fs\n", time_spent);

    return 0;
}