#include <stdio.h>
#include "include/arrays.h"
#include "include/tui.h"
#include "include/strings.h"

void rowDestructor(void* elem) {
    Array inner = *(Array*)elem;
    arrayFree(inner, stringFreeRef);
}

Array createRow(String n1, String n2, String n3, String n4, String n5) {
    Array row = array(sizeof(String));
    arrayAdd(row, &n1);
    arrayAdd(row, &n2);
    arrayAdd(row, &n3);
    arrayAdd(row, &n4);
    arrayAdd(row, &n5);
    return row;
}

void drawScene(double minX, double maxX, double minY, double maxY, Array header, Array rows) {
    tuiClearScreen();
    
    tuiSetViewport(minX, maxX, minY, maxY);

    tuiColor(TUI_BLUE);
    tuiDrawTable(projectX(-2),projectY(-2), header, rows);
    tuiColor(TUI_STYLE_RESET);

    tuiGoToXY(0, 0);
    tuiStyle(TUI_STYLE_BOLD);
    printf("Viewport: X[%.2f : %.2f] Y[%.2f : %.2f]", minX, maxX, minY, maxY);
    tuiStyle(TUI_STYLE_RESET);
    tuiUpdate();
}

int main() {
    tuiInit();
    Array header = array(sizeof(String));
    Array rows = array(sizeof(Array)); 

    String h1 = stringNew("First Name");
    String h2 = stringNew("Last Name");
    String h3 = stringNew("Age");
    String h4 = stringNew("Height");
    String h5 = stringNew("Weight");

    arrayAdd(header, &h1);
    arrayAdd(header, &h2);
    arrayAdd(header, &h3);
    arrayAdd(header, &h4);
    arrayAdd(header, &h5);

    Array row1 = createRow(stringNew("Nicolo' Emanuele"), stringNew("Mele"), stringNew("18"), stringNew("175cm"), stringNew("77.5kg"));
    Array row2 = createRow(stringNew("Martin"), stringNew("Garcia"), stringNew("37"), stringNew("195cm"), stringNew("90.75kg"));
    Array row3 = createRow(stringNew("Galliver"), stringNew("Reds"), stringNew("69"), stringNew("167cm"), stringNew("69.67kg"));

    arrayAdd(rows, &row1);
    arrayAdd(rows, &row2);
    arrayAdd(rows, &row3);

    double minX = -10.0;
    double maxX = 10.0;
    double minY = -5.0;
    double maxY = 5.0;

    drawScene(minX, maxX, minY, maxY, header, rows);

    while(1) {
        int key = tuiReadKey();

        bool changed = false;
        double rangeY = maxY - minY;
        double stepY = rangeY * 0.05;
        
        if (key == 'q' || key == TUI_KEY_ESC) {
            break;
        }
        else if (key == TUI_KEY_DOWN) {
            minY -= stepY; maxY -= stepY;
            changed = true;
        }
        else if (key == TUI_KEY_UP) {
            minY += stepY; maxY += stepY;
            changed = true;
        }

        if (tuiHasResized() || changed) {
            drawScene(minX, maxX, minY, maxY, header, rows);
        }
    }

    tuiClose();
    
    arrayFree(header, stringFreeRef);
    arrayFree(rows, rowDestructor);

    return 0;
}