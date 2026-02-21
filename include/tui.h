#ifndef TUI_H
#define TUI_H

#include <stdbool.h>
#include <stddef.h>
#include "arrays.h"
#include "files.h"
#include "lists.h"
#include "strings.h"
#include "trees.h"
#include "maps.h"

#define TUI_UTF8

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

#define TUI_KEY_NONE 0
#define TUI_KEY_ENTER 10
#define TUI_KEY_ESC 27
#define TUI_KEY_BACKSPACE 127
#define TUI_KEY_TAB 9
#define TUI_KEY_UP 1001
#define TUI_KEY_DOWN 1002
#define TUI_KEY_LEFT 1003
#define TUI_KEY_RIGHT 1004

typedef struct {
    int id;
    int x;
    int y;
    int width;
    char* buffer;
    size_t bufferSize;
    size_t currentLen;
    bool isFocused;
    bool isPassword;
    const char* label;
} TuiInput;

typedef struct {
    int x;
    int y;
    int width;
    int height;
    const char* title;
} TuiContainer;

typedef struct {
    int id;
    int x;
    int y;
    int width;
    const char* label;
    bool isFocused;
} TuiButton;

typedef struct {
    double minX;
    double maxX;
    double minY;
    double maxY;
} TuiViewport;

void tuiInit();
void tuiClose();
void tuiUpdate();
void tuiSleep(int ms);
void tuiGetTerminalSize(int* rows, int* cols);
int tuiGetTerminalWidth();
int tuiGetTerminalHeight();
void tuiClearScreen();

bool tuiHasResized();
int tuiReadKey();

void tuiGoToXY(int x, int y);
void tuiCursorVisible(bool visible);
void tuiBackground(short color);
void tuiColor(short color);
void tuiColorRGB(short r, short g, short b);
void tuiColorHEX(const char* hexColorCode);
void tuiStyle(short style);

void tuiDrawPixel(int x, int y, const char* c);
void tuiDrawLine(int x0, int y0, int x1, int y1);
void tuiDrawRect(int x, int y, int w, int h);
void tuiDrawStringObject(int x, int y, String s);
void tuiPrintRepeat(const char* s, int times);

void tuiDrawTable(int x, int y, Array headers, Array rows);
TuiInput tuiInputCreate(int id, int x, int y, int width, const char* label, size_t maxLen);
void tuiInputSetPassword(TuiInput* input, bool isPassword);
void tuiInputFree(TuiInput* input);
void tuiDrawInput(TuiInput* input);
void tuiUpdateInput(TuiInput* input, int key);
TuiContainer tuiContainerCreate(int x, int y, int width, int height, const char* title);
void tuiDrawContainer(TuiContainer* container);
TuiButton tuiButtonCreate(int id, int x, int y, int width, const char* label);
void tuiDrawButton(TuiButton* button);

int projectX(double x);
int projectY(double y);

void tuiDrawArray(int x, int y, Array arr, int (*printFunc)(void*, bool), bool showHeader);
void tuiDrawSLinkedList(int x, int y, SLinkedList* list, int (*printFunc)(void*, bool));
void tuiDrawDLinkedList(int x, int y, DLinkedList* list, int (*printFunc)(void*, bool));
void tuiDrawStack(int x, int y, Stack* stack, int (*printFunc)(void*, bool));
void tuiDrawTree(int x, int y, Tree t, int (*printFunc)(void*, bool));
void tuiDrawHashMap(int x, int y, HashMap map, int (*printKey)(void*, bool), int (*printVal)(void*, bool));
void tuiDrawSet(int x, int y, Set set, int (*printKey)(void*, bool));
void tuiDrawFileInfo(int x, int y, File f);

void tuiSetViewport(double minX, double maxX, double minY, double maxY);
TuiViewport tuiGetViewport();
void tuiPlotPoint(double x, double y, const char* c);
void tuiPlotLine(double x1, double y1, double x2, double y2);
void tuiPlotFunc(double (*func)(double), double step);
void tuiPlotAxes();

int tuiPrinterInt(void* data, bool dryRun);
int tuiPrinterUInt(void* data, bool dryRun);
int tuiPrinterSizeT(void* data, bool dryRun);
int tuiPrinterFloat(void* data, bool dryRun);
int tuiPrinterDouble(void* data, bool dryRun);
int tuiPrinterChar(void* data, bool dryRun);
int tuiPrinterBool(void* data, bool dryRun);
int tuiPrinterString(void* data, bool dryRun);
int tuiPrinterCString(void* data, bool dryRun);
void tuiSetSearchTerm(char* term);
int tuiPrinterStringHighlight(void* data, bool dryRun);
int tuiPrinterCStringHighlight(void* data, bool dryRun);
int tuiPrinterJsonKey(void* data, bool dryRun);

#endif