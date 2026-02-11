#include <stdio.h>
#include <stdlib.h>
#include <../include/strings.h>
#include "../include/tui.h"
#include "../include/pointers.h"

void tuiReset() {
    printf("\033[0m");
}

void tuiClearScreen() {
    printf("\033[2J\033[H");
}

void tuiGoToXY(int x, int y) {
    printf("\033[%d;%dH", y, x);
}

void tuiCursorVisible(bool visible) {
    if (visible)
        printf("\033[?25h");
    else
        printf("\033[?25l");
}

void tuiBackground(short color) {
    if (color >= 0 && color <= TUI_DEFAULT)
        printf("\033[4%dm", color);
}

void tuiColor(short color) {
    if (color >= 0 && color <= TUI_DEFAULT)
        printf("\033[3%dm", color);
}

void tuiColorRGB(short r, short g, short b) {
    printf("\033[38;2;%d;%d;%dm", r, g, b);
}

void tuiColorHEX(const char* hexColorCode) {
    String fullHex = stringNew(hexColorCode);
    if (stringIsNull(fullHex)) return;

    String cleanHex;
    if (stringCharAt(fullHex, 0) == '#') {
        cleanHex = stringSubstring(fullHex, 1, stringLength(fullHex));
    } else {
        cleanHex = stringNew(fullHex->data); 
    }
    
    stringFree(fullHex);

    if (stringLength(cleanHex) < 6) {
        stringFree(cleanHex);
        return;
    }

    String rStr = stringSubstring(cleanHex, 0, 2);
    String gStr = stringSubstring(cleanHex, 2, 4);
    String bStr = stringSubstring(cleanHex, 4, 6);

    int r = stringParseHex(rStr);
    int g = stringParseHex(gStr);
    int b = stringParseHex(bStr);

    tuiColorRGB((short)r, (short)g, (short)b);

    stringFree(cleanHex);
    stringFree(rStr);
    stringFree(gStr);
    stringFree(bStr);
}

void tuiStyle(short style) {
    printf("\033[%dm", style);
}

static void printRepeat(const char* s, int times) {
    for (int i = 0; i < times; i++) {
        printf("%s", s);
    }
}

static int getMaxColWidth(Array headers, Array rows, int colIndex) {
    int maxW = 0;
    
    if (colIndex < headers->len) {
        String* hPtr = (String*)arrayGetRef(headers, colIndex);
        if (hPtr && *hPtr) {
            int len = (int)stringLength(*hPtr);
            if (len > maxW) maxW = len;
        }
    }

    for (size_t i = 0; i < rows->len; i++) {
        Array* rowPtr = (Array*)arrayGetRef(rows, i);
        if (rowPtr && *rowPtr) {
            Array row = *rowPtr;
            if (colIndex < row->len) {
                String* cellPtr = (String*)arrayGetRef(row, colIndex);
                if (cellPtr && *cellPtr) {
                    int len = (int)stringLength(*cellPtr);
                    if (len > maxW) maxW = len;
                }
            }
        }
    }
    return maxW;
}

void tuiPrintTable(Array headers, Array rows) {
    if (!headers || !rows) return;

    size_t cols = headers->len;
    int* widths = (int*)xCalloc(cols, sizeof(int));

    for (size_t i = 0; i < cols; i++) {
        widths[i] = getMaxColWidth(headers, rows, (int)i);
    }

    printf("┌");
    for (size_t i = 0; i < cols; i++) {
        printRepeat("─", widths[i] + 2);
        if (i < cols - 1) printf("┬");
    }
    printf("┐\n");

    printf("│");
    for (size_t i = 0; i < cols; i++) {
        String* hPtr = (String*)arrayGetRef(headers, i);
        String h = (hPtr) ? *hPtr : NULL;
        char* data = (h) ? stringGetData(h) : "";
        int len = (h) ? (int)stringLength(h) : 0;
        
        printf(" \033[1m%s\033[0m ", data); 
        printRepeat(" ", widths[i] - len);
        printf("│");
    }
    printf("\n");

    printf("├");
    for (size_t i = 0; i < cols; i++) {
        printRepeat("─", widths[i] + 2);
        if (i < cols - 1) printf("┼");
    }
    printf("┤\n");

    for (size_t i = 0; i < rows->len; i++) {
        Array* rowPtr = (Array*)arrayGetRef(rows, i);
        Array row = (rowPtr) ? *rowPtr : NULL;
        
        printf("│");
        for (size_t j = 0; j < cols; j++) {
            char* valStr = "";
            int len = 0;

            if (row && j < row->len) {
                String* cellPtr = (String*)arrayGetRef(row, j);
                if (cellPtr && *cellPtr) {
                    valStr = stringGetData(*cellPtr);
                    len = (int)stringLength(*cellPtr);
                }
            }

            printf(" %s ", valStr);
            printRepeat(" ", widths[j] - len);
            printf("│");
        }
        printf("\n");
    }

    printf("└");
    for (size_t i = 0; i < cols; i++) {
        printRepeat("─", widths[i] + 2);
        if (i < cols - 1) printf("┴");
    }
    printf("┘\n");

    xFree(widths);
}