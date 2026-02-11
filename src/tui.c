#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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
    printf(visible ? "\033[?25h" : "\033[?25l");
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
        cleanHex = stringSubstring(fullHex, 1, fullHex->len);
    } else {
        cleanHex = stringNew(fullHex->data);
    }
    stringFree(fullHex);

    if (cleanHex->len < 6) {
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


static void _tuiPrintRepeat(const char* s, int times) {
    for (int i = 0; i < times; i++) {
        printf("%s", s);
    }
}

static void _tuiPrintBorder(int* widths, size_t cols, const char* left, const char* mid, const char* right, const char* dash) {
    printf("%s", left);
    for (size_t i = 0; i < cols; i++) {
        _tuiPrintRepeat(dash, widths[i] + 2);
        if (i < cols - 1) {
            printf("%s", mid);
        }
    }
    printf("%s\n", right);
}

static int _tuiGetMaxColWidth(Array headers, Array rows, int colIndex) {
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
    if (cols == 0) return;

    int* widths = (int*)xCalloc(cols, sizeof(int));
    for (size_t i = 0; i < cols; i++) {
        widths[i] = _tuiGetMaxColWidth(headers, rows, (int)i);
    }

    _tuiPrintBorder(widths, cols, "┌", "┬", "┐", "─");

    printf("│");
    for (size_t i = 0; i < cols; i++) {
        String* hPtr = (String*)arrayGetRef(headers, i);
        String h = (hPtr) ? *hPtr : NULL;
        char* data = (h) ? stringGetData(h) : "";
        int len = (h) ? (int)stringLength(h) : 0;
        int padding = widths[i] - len;

        printf(" \033[1m%s\033[22m ", data);
        _tuiPrintRepeat(" ", padding);
        printf("│");
    }
    printf("\n");

    _tuiPrintBorder(widths, cols, "├", "┼", "┤", "─");

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

            int padding = widths[j] - len;
            
            
            printf(" %s", valStr);
            _tuiPrintRepeat(" ", padding + 1);
            printf("│");
        }
        printf("\n");
    }

    _tuiPrintBorder(widths, cols, "└", "┴", "┘", "─");

    xFree(widths);
}