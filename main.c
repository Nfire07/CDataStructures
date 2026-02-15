#include <stdio.h>
#include <math.h>
#include "include/tui.h"

double sigmoid(double x) {
    return 1.0 / (1.0 + exp(-x));
}

double function(double x){
    return tanh(x);
}


void drawScene(double minX, double maxX, double minY, double maxY) {
    tuiClearScreen();
    tuiSetViewport(minX, maxX, minY, maxY);
    
    tuiPlotAxes();

    tuiColor(TUI_MAGENTA);
    tuiDrawRect(projectX(2), projectY(1.7), tuiGetTerminalWidth()/2, tuiGetTerminalHeight()/2);
    tuiStyle(TUI_STYLE_RESET);

    tuiGoToXY(0, 0);
    printf("Viewport: X[%.2f : %.2f]", minX, maxX);
    tuiUpdate();
}

int main() {
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
    return 0;
}