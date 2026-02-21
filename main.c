#include <stdio.h>
#include <math.h>
#include "include/tui.h"
#include <time.h>

double function(double x){
    return tanh(x);
}

static clock_t start;

void drawScene(double minX, double maxX, double minY, double maxY) {
    tuiClearScreen();
    tuiSetViewport(minX, maxX, minY, maxY);
    
    tuiPlotAxes();

    tuiColor(TUI_MAGENTA);
    tuiDrawRect(projectX(2), projectY(1.7), tuiGetTerminalWidth()/2, tuiGetTerminalHeight()/2);
    tuiStyle(TUI_STYLE_RESET);

    tuiGoToXY(0, 0);
    printf("Viewport: X[%.2f : %.2f]", minX, maxX);

    clock_t now = clock();
    double elapsed = (double)(now - start) / CLOCKS_PER_SEC;

    int col = tuiGetTerminalWidth() - 20; 
    tuiGoToXY(col, 0);
    printf("Time: %.0fs", elapsed);

    tuiUpdate();
}

int main() {
    start = clock();

    tuiInit();

    double minX = -10.0;
    double maxX = 10.0;
    double minY = -2.0;
    double maxY = 2.0;

    drawScene(minX, maxX, minY, maxY);

    while(1) {
        int key = tuiReadKey();
        
        if (key == 'q' || key == TUI_KEY_ESC) {
            break;
        }

        bool changed = false;
        double rangeX = maxX - minX;
        double rangeY = maxY - minY;
        double stepX = rangeX * 0.05; 
        double stepY = rangeY * 0.05;

        if (key == TUI_KEY_LEFT) {
            minX -= stepX; maxX -= stepX;
            changed = true;
        }
        else if (key == TUI_KEY_RIGHT) {
            minX += stepX; maxX += stepX;
            changed = true;
        }
        else if (key == TUI_KEY_DOWN) {
            minY -= stepY; maxY -= stepY;
            changed = true;
        }
        else if (key == TUI_KEY_UP) {
            minY += stepY; maxY += stepY;
            changed = true;
        }
        else if(key == '+'){
            minX += stepX; maxX -= stepX;
            minY += stepY; maxY -= stepY;
            changed = true;
        }
        else if(key == '-'){
            minX -= stepX; maxX += stepX;
            minY -= stepY; maxY += stepY;
            changed = true;
        }

        if (tuiHasResized() || changed) {
            drawScene(minX, maxX, minY, maxY);
        }
    }

    tuiClose();

    clock_t end = clock();
    double time_spent = (double)(end - start) / CLOCKS_PER_SEC;
    printf("Execution time: %.2f seconds\n", time_spent);

    return 0;
}