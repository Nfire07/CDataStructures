#include <stdio.h>
#include <math.h>
#include "include/tui.h"

double sigmoid(double x) {
    return 1.0 / (1.0 + exp(-x));
}

double function(double x){
    return x*x*x;
}


void drawScene(double minX, double maxX, double minY, double maxY) {
    tuiClearScreen();
    
    tuiSetViewport(minX, maxX, minY, maxY);
    
    tuiPlotAxes();
    tuiPlotFunc(function, 0); 
    
    tuiGoToXY(0, 0);
    printf("Controls: [Arrows] Move | [W/S] Zoom Y | [A/D] Zoom X | [Q] Exit");
    tuiGoToXY(0, 1);
    printf("Viewport: X[%.2f : %.2f] Y[%.2f : %.2f]", minX, maxX, minY, maxY);
    
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

        else if (key == 'a') { // Zoom In X (Restringe il range)
            minX += stepX; maxX -= stepX;
            changed = true;
        }
        else if (key == 'd') { // Zoom Out X (Allarga il range)
            minX -= stepX; maxX += stepX;
            changed = true;
        }

        else if (key == 'w') { // Zoom In Y
            minY += stepY; maxY -= stepY;
            changed = true;
        }
        else if (key == 's') { // Zoom Out Y
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