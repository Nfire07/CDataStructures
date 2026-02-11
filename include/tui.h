#ifndef TUI_H
#define TUI_H

#include "arrays.h"
#include "strings.h"

#define TUI_BLACK 0
#define TUI_RED 1
#define TUI_GREEN 2
#define TUI_YELLOW 3
#define TUI_BLUE 4
#define TUI_MAGENTA 5
#define TUI_CYAN 6
#define TUI_WHITE 7
#define TUI_DEFAULT 9

#define TUI_STYLE_RESET 0
#define TUI_STYLE_BOLD 1
#define TUI_STYLE_DIM 2
#define TUI_STYLE_ITALIC 3
#define TUI_STYLE_UNDERLINE 4
#define TUI_STYLE_BLINK 5
#define TUI_STYLE_REVERSE 7
#define TUI_STYLE_HIDDEN 8

void tuiReset();
void tuiClearScreen();
void tuiGoToXY(int x, int y);
void tuiCursorVisible(bool visible);

void tuiBackground(short color);
void tuiColor(short color);
void tuiColorRGB(short r, short g, short b);
void tuiColorHEX(const char* hexColorCode);
void tuiStyle(short style);

void tuiPrintTable(Array headers, Array rows);

#endif