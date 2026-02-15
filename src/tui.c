#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h> 
#include <signal.h>

#ifdef _WIN32
    #include <windows.h>
    #include <conio.h>
#else
    #include <unistd.h>
    #include <termios.h>
    #include <sys/ioctl.h>
#endif

#include "../include/tui.h"
#include "../include/pointers.h"
#include "../include/strings.h"
#include "../include/arrays.h"
#include "../include/trees.h"

static int _internalTermWidth = 80;
static int _internalTermHeight = 24;
static volatile sig_atomic_t _resizedFlag = 0;
static TuiViewport _viewport = {-10.0, 10.0, -10.0, 10.0};

#ifndef _WIN32
static struct termios orig_termios;

static void disableRawMode() {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
    printf("\033[?25h"); 
}

static void enableRawMode() {
    tcgetattr(STDIN_FILENO, &orig_termios);
    atexit(disableRawMode);
    
    struct termios raw = orig_termios;
    raw.c_lflag &= ~(ECHO | ICANON | ISIG); 
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}
#endif

static void _tuiSignalHandler(int sig) {
    if (sig == SIGINT) {
        tuiClose();
        exit(0);
    }
#ifndef _WIN32
    else if (sig == SIGWINCH) {
        _resizedFlag = 1;
    }
#endif
}

static void _updateDimensions() {
    int w, h;
    tuiGetTerminalSize(&h, &w);
    _internalTermWidth = w;
    _internalTermHeight = h;
}

void tuiInit() {
    signal(SIGINT, _tuiSignalHandler);
    
#ifndef _WIN32
    signal(SIGWINCH, _tuiSignalHandler);
    enableRawMode();
#else
    SetConsoleOutputCP(CP_UTF8);
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD dwMode = 0;
    GetConsoleMode(hOut, &dwMode);
    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(hOut, dwMode | ENABLE_PROCESSED_INPUT);
#endif

    tuiCursorVisible(false);
    tuiClearScreen();
    
    _updateDimensions();
}

void tuiClose() {
    tuiStyle(TUI_STYLE_RESET);
    tuiColor(TUI_DEFAULT);
    tuiBackground(TUI_DEFAULT);
    tuiClearScreen();
    tuiCursorVisible(true);
#ifndef _WIN32
    disableRawMode();
#endif
}

void tuiUpdate() {
    fflush(stdout);
}

void tuiSleep(int ms) {
#ifdef _WIN32
    Sleep(ms);
#else
    usleep(ms * 1000);
#endif
}

void tuiGetTerminalSize(int* rows, int* cols) {
#ifdef _WIN32
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
    *cols = csbi.srWindow.Right - csbi.srWindow.Left + 1;
    *rows = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
#else
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    *rows = w.ws_row;
    *cols = w.ws_col;
#endif
}

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

void tuiPrintRepeat(const char* s, int times){
    _tuiPrintRepeat(s,times);
}

static void _tuiPrintBorder(int* widths, size_t cols, const char* left, const char* mid, const char* right, const char* dash) {
    printf("%s", left);
    for (size_t i = 0; i < cols; i++) {
        _tuiPrintRepeat(dash, widths[i] + 2);
        if (i < cols - 1) {
            printf("%s", mid);
        }
    }
    printf("%s", right);
}

static int _projectX(double x) {
    double range = _viewport.maxX - _viewport.minX;
    if (range == 0) return 0;
    double percent = (x - _viewport.minX) / range;
    return (int)(percent * _internalTermWidth);
}

static int _projectY(double y) {
    double range = _viewport.maxY - _viewport.minY;
    if (range == 0) return 0;
    double percent = (y - _viewport.minY) / range;
    return (_internalTermHeight - 1) - (int)(percent * _internalTermHeight);
}

static int _tuiGetMaxColWidth(Array headers, Array rows, size_t colIndex) {
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

void tuiDrawTable(int x, int y, Array headers, Array rows) {
    if (!headers || !rows) return;

    size_t cols = headers->len;
    if (cols == 0) return;

    int* widths = (int*)xCalloc(cols, sizeof(int));
    for (size_t i = 0; i < cols; i++) {
        widths[i] = _tuiGetMaxColWidth(headers, rows, i);
    }

    int currentY = y;

    tuiGoToXY(x, currentY++);
    _tuiPrintBorder(widths, cols, "┌", "┬", "┐", "─");

    tuiGoToXY(x, currentY++);
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

    tuiGoToXY(x, currentY++);
    _tuiPrintBorder(widths, cols, "├", "┼", "┤", "─");

    for (size_t i = 0; i < rows->len; i++) {
        tuiGoToXY(x, currentY++);
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
    }

    tuiGoToXY(x, currentY++);
    _tuiPrintBorder(widths, cols, "└", "┴", "┘", "─");

    xFree(widths);
}

int tuiReadKey() {
    int key = TUI_KEY_NONE;
#ifdef _WIN32
    if (_kbhit()) {
        int ch = _getch();
        if (ch == 0 || ch == 224) { 
            ch = _getch(); 
            switch (ch) {
                case 72: key = TUI_KEY_UP; break;
                case 80: key = TUI_KEY_DOWN; break;
                case 75: key = TUI_KEY_LEFT; break;
                case 77: key = TUI_KEY_RIGHT; break;
                default: key = ch; break;
            }
        } else if (ch == 8) {
            key = TUI_KEY_BACKSPACE;
        } else if (ch == 13) {
            key = TUI_KEY_ENTER;
        } else if (ch == 27) {
            key = TUI_KEY_ESC;
        } else if (ch == 9) {
            key = TUI_KEY_TAB;
        } else {
            key = ch;
        }
    }
#else
    char c;
    ssize_t nread = read(STDIN_FILENO, &c, 1);

    if (nread > 0) {
        if (c == 27) {
            char seq[3];
            if (read(STDIN_FILENO, &seq[0], 1) != 1) return TUI_KEY_ESC;
            if (read(STDIN_FILENO, &seq[1], 1) != 1) return TUI_KEY_ESC;
            
            if (seq[0] == '[') {
                switch (seq[1]) {
                    case 'A': key = TUI_KEY_UP; break;
                    case 'B': key = TUI_KEY_DOWN; break;
                    case 'C': key = TUI_KEY_RIGHT; break;
                    case 'D': key = TUI_KEY_LEFT; break;
                }
            }
        } else if (c == 127 || c == 8) {
            key = TUI_KEY_BACKSPACE;
        } else if (c == 10) {
            key = TUI_KEY_ENTER;
        } else if (c == 9) {
            key = TUI_KEY_TAB;
        } else {
            key = c;
        }
    }
#endif
    return key;
}

TuiInput tuiInputCreate(int id, int x, int y, int width, const char* label, size_t maxLen) {
    TuiInput inp;
    inp.id = id;
    inp.x = x;
    inp.y = y;
    inp.width = width;
    inp.bufferSize = maxLen + 1;
    inp.buffer = (char*)xCalloc(inp.bufferSize, sizeof(char));
    inp.currentLen = 0;
    inp.isFocused = false;
    inp.isPassword = false;
    inp.label = label;
    return inp;
}

void tuiInputSetPassword(TuiInput* input, bool isPassword) {
    if (input) {
        input->isPassword = isPassword;
    }
}

void tuiInputFree(TuiInput* input) {
    if (input && input->buffer) {
        xFree(input->buffer);
        input->buffer = NULL;
    }
}

void tuiDrawInput(TuiInput* input) {
    if (!input) return;

    tuiGoToXY(input->x, input->y);
    tuiStyle(TUI_STYLE_BOLD);
    if (input->isFocused) tuiColor(TUI_CYAN);
    else tuiColor(TUI_WHITE);
    printf("%s", input->label);
    tuiStyle(TUI_STYLE_RESET);

    if (input->isFocused) tuiColor(TUI_CYAN);
    else tuiColor(TUI_WHITE);

    tuiGoToXY(input->x, input->y + 1);
    printf("╭");
    _tuiPrintRepeat("─", input->width);
    printf("╮");

    tuiGoToXY(input->x, input->y + 2);
    printf("│");
    
    tuiStyle(TUI_STYLE_BOLD);
    tuiColor(TUI_WHITE);
    
    if (input->isPassword) {
        for(size_t i = 0; i < input->currentLen; i++) {
            printf("*");
        }
    } else {
        printf("%s", input->buffer);
    }

    tuiStyle(TUI_STYLE_RESET);
    if (input->isFocused) tuiColor(TUI_CYAN);
    else tuiColor(TUI_WHITE);

    int padding = input->width - (int)input->currentLen;
    if (input->isFocused) {
        printf("\033[5m_\033[25m"); 
        padding--;
    }
    
    _tuiPrintRepeat(" ", padding > 0 ? padding : 0);
    printf("│");

    tuiGoToXY(input->x, input->y + 3);
    printf("╰");
    _tuiPrintRepeat("─", input->width);
    printf("╯");

    tuiColor(TUI_DEFAULT);
}

void tuiUpdateInput(TuiInput* input, int key) {
    if (!input || !input->isFocused || key == TUI_KEY_NONE) return;

    if (key == TUI_KEY_BACKSPACE) {
        if (input->currentLen > 0) {
            input->buffer[--input->currentLen] = '\0';
        }
    } else if (key >= 32 && key <= 126) { 
        if (input->currentLen < input->bufferSize - 1 && input->currentLen < (size_t)input->width - 1) {
            input->buffer[input->currentLen++] = (char)key;
            input->buffer[input->currentLen] = '\0';
        }
    }
}

TuiContainer tuiContainerCreate(int x, int y, int width, int height, const char* title) {
    TuiContainer c;
    c.x = x;
    c.y = y;
    c.width = width;
    c.height = height;
    c.title = title;
    return c;
}

void tuiDrawContainer(TuiContainer* container) {
    if (!container) return;

    tuiGoToXY(container->x, container->y);
    tuiColor(TUI_WHITE);
    printf("┌");
    
    int titleLen = (int)strlen(container->title);
    int dashLen = container->width - 2;
    
    if (titleLen > 0 && titleLen < dashLen) {
        printf(" %s ", container->title);
        _tuiPrintRepeat("─", dashLen - titleLen - 2);
    } else {
        _tuiPrintRepeat("─", dashLen);
    }
    printf("┐");

    for (int i = 1; i < container->height - 1; i++) {
        tuiGoToXY(container->x, container->y + i);
        printf("│");
        tuiGoToXY(container->x + container->width - 1, container->y + i);
        printf("│");
    }

    tuiGoToXY(container->x, container->y + container->height - 1);
    printf("└");
    _tuiPrintRepeat("─", container->width - 2);
    printf("┘");
}

TuiButton tuiButtonCreate(int id, int x, int y, int width, const char* label) {
    TuiButton btn;
    btn.id = id;
    btn.x = x;
    btn.y = y;
    btn.width = width;
    btn.label = label;
    btn.isFocused = false;
    return btn;
}

void tuiDrawButton(TuiButton* button) {
    if (!button) return;

    tuiGoToXY(button->x, button->y);
    
    if (button->isFocused) {
        tuiBackground(TUI_BLUE);
        tuiColor(TUI_WHITE);
        tuiStyle(TUI_STYLE_BOLD);
    } else {
        tuiBackground(TUI_DEFAULT);
        tuiColor(TUI_WHITE);
        tuiStyle(TUI_STYLE_RESET);
    }

    printf("[");
    
    int labelLen = (int)strlen(button->label);
    int totalPadding = button->width - 2 - labelLen;
    int padLeft = totalPadding / 2;
    int padRight = totalPadding - padLeft;

    _tuiPrintRepeat(" ", padLeft);
    printf("%s", button->label);
    _tuiPrintRepeat(" ", padRight);
    
    printf("]");

    tuiStyle(TUI_STYLE_RESET);
    tuiBackground(TUI_DEFAULT);
    tuiColor(TUI_DEFAULT);
}


void tuiPrinterInt(void* data) {
    if (!data) return;
    printf("%d", *(int*)data);
}

void tuiPrinterString(void* data) {
    if (!data) return;
    String s = *(String*)data;
    if (s && s->data) {
        printf("%s", s->data);
    } else {
        printf("(null)");
    }
}

void tuiDrawStringObject(int x, int y, String s) {
    tuiGoToXY(x, y);
    if (stringIsNull(s)) {
        tuiColor(TUI_RED);
        printf("String: NULL");
        tuiColor(TUI_DEFAULT);
        return;
    }

    tuiColor(TUI_CYAN);
    printf("String");
    tuiColor(TUI_WHITE);
    printf("[Len:%zu]: ", s->len);
    tuiStyle(TUI_STYLE_BOLD);
    printf("\"%s\"", s->data ? s->data : "");
    tuiStyle(TUI_STYLE_RESET);
}

void tuiDrawArray(int x, int y, Array arr, void (*printFunc)(void*)) {
    if (!arr) return;

    int cellWidth = 6;
    int totalLen = arr->len;
    tuiGoToXY(x, y);
    tuiColor(TUI_YELLOW);
    printf("Array");
    tuiColor(TUI_WHITE);
    printf(" [Len:%zu | Cap:%zu] ", arr->len, arr->capacity);
    
    tuiGoToXY(x, y + 1);
    printf("┌");
    for (int i = 0; i < totalLen; i++) {
        _tuiPrintRepeat("─", cellWidth);
        if (i < totalLen - 1) printf("┬");
        else printf("┐");
    }

    tuiGoToXY(x, y + 2);
    for (int i = 0; i < totalLen; i++) {
        int currentX = x + (i * (cellWidth + 1));
        
        tuiGoToXY(currentX, y + 2);
        printf("│ ");
        
        void* item = arrayGetRef(arr, i);
        if (printFunc) {
            printFunc(item);
        } else {
            printf("?");
        }
    }
    tuiGoToXY(x + (totalLen * (cellWidth + 1)), y + 2);
    printf("│");

    tuiGoToXY(x, y + 3);
    printf("└");
    for (int i = 0; i < totalLen; i++) {
        _tuiPrintRepeat("─", cellWidth);
        if (i < totalLen - 1) printf("┴");
        else printf("┘");
    }
}

void tuiDrawSLinkedList(int x, int y, SLinkedList* list, void (*printFunc)(void*)) {
    if (!list) return;

    tuiGoToXY(x, y);
    tuiColor(TUI_MAGENTA);
    printf("SLinkedList");
    tuiColor(TUI_WHITE);
    printf(" [Len:%zu]", list->len);

    int curX = x;
    int curY = y + 2;
    SLinkedListNode* current = list->head;

    tuiGoToXY(curX, curY);
    printf("HEAD ──> ");
    curX += 9;

    while (current != NULL) {
        printf("[ ");
        if (printFunc) {
            printFunc(current->data);
        }
        printf(" ] ──> ");
        
        current = current->next;
    }
    
    tuiColor(TUI_RED);
    printf("NULL");
    tuiColor(TUI_DEFAULT);
}

void tuiDrawDLinkedList(int x, int y, DLinkedList* list, void (*printFunc)(void*)) {
    if (!list) return;

    tuiGoToXY(x, y);
    tuiColor(TUI_MAGENTA);
    printf("DLinkedList");
    tuiColor(TUI_WHITE);
    printf(" [Len:%zu]", list->len);

    int curX = x;
    int curY = y + 2;
    DLinkedListNode* current = list->head;

    tuiGoToXY(curX, curY);
    printf("HEAD <──> ");
    
    while (current != NULL) {
        printf("[ ");
        if (printFunc) {
            printFunc(current->data);
        }
        printf(" ] <──> ");
        current = current->next;
    }

    tuiColor(TUI_RED);
    printf("NULL");
    tuiColor(TUI_DEFAULT);
}

void tuiDrawStack(int x, int y, Stack* stack, void (*printFunc)(void*)) {
    if (!stack) return;

    tuiGoToXY(x, y);
    tuiColor(TUI_GREEN);
    printf("Stack");
    tuiColor(TUI_WHITE);
    printf(" (Top -> Bottom)");

    SLinkedListNode* current = stack->head;
    int curY = y + 1;

    if (current == NULL) {
        tuiGoToXY(x, curY++);
        printf("┌──────────────┐");
        tuiGoToXY(x, curY++);
        printf("│    EMPTY     │");
        tuiGoToXY(x, curY++);
        printf("└──────────────┘");
        return;
    }

    tuiGoToXY(x, curY++);
    printf("      │  │"); 
    
    while (current != NULL) {
        tuiGoToXY(x, curY++);
        printf("┌─────▼──▼─────┐");
        tuiGoToXY(x, curY++);
        printf("│ ");
        
        if (printFunc) {
            printFunc(current->data);
        } else {
            printf("????");
        }

        tuiGoToXY(x + 15, curY - 1);
        printf("│");
        
        tuiGoToXY(x, curY++);
        printf("└──────────────┘");
        
        current = current->next;
    }
}

void tuiDrawFileInfo(int x, int y, File f) {
    tuiGoToXY(x, y);
    tuiColor(TUI_BLUE);
    printf("File Info");
    tuiStyle(TUI_STYLE_RESET);

    tuiGoToXY(x, y + 1);
    printf("┌──────────────────────┐");
    tuiGoToXY(x, y + 2);
    printf("│ Status: ");
    if (f && f->fp) {
        tuiColor(TUI_GREEN);
        printf("OPEN         ");
    } else {
        tuiColor(TUI_RED);
        printf("CLOSED       ");
    }
    tuiColor(TUI_DEFAULT);
    printf("│");
    
    tuiGoToXY(x, y + 3);
    printf("│ Size:   %-10zu   │", (f ? f->size : 0));
    
    tuiGoToXY(x, y + 4);
    printf("└──────────────────────┘");
}

static void _tuiDrawTreeNode(TreeNode* node, int x, int y, int offset, void (*printFunc)(void*)) {
    if (!node) return;

    tuiGoToXY(x, y);
    
    printf("(");
    tuiStyle(TUI_STYLE_BOLD);
    tuiColor(TUI_CYAN);
    if (printFunc) printFunc(node->data);
    else printf("?");
    tuiColor(TUI_DEFAULT);
    tuiStyle(TUI_STYLE_RESET);
    printf(")");

    if (offset < 2) offset = 2; 

    if (node->left) {
        int childX = x - offset;
        int childY = y + 2;
        
        tuiGoToXY(x - 1, y + 1);
        printf("┌"); 
        int dashLen = (x - 1) - childX;
        for(int k=0; k<dashLen; k++) {
             tuiGoToXY(x - 2 - k, y + 1);
             printf("─");
        }
        tuiGoToXY(childX, y + 1);
        printf("┐");

        _tuiDrawTreeNode(node->left, childX, childY, offset / 2, printFunc);
    }

    if (node->right) {
        int childX = x + offset;
        int childY = y + 2;

        tuiGoToXY(x + 1, y + 1);
        printf("┐");
        int dashLen = childX - (x + 1);
        for(int k=0; k<dashLen; k++) {
             tuiGoToXY(x + 2 + k, y + 1);
             printf("─");
        }
        tuiGoToXY(childX, y + 1);
        printf("┌");

        _tuiDrawTreeNode(node->right, childX, childY, offset / 2, printFunc);
    }
}

void tuiDrawTree(int x, int y, Tree t, void (*printFunc)(void*)) {
    if (!t) return;
    
    tuiGoToXY(x, y);
    tuiColor(TUI_MAGENTA);
    printf("Binary Tree");
    tuiColor(TUI_WHITE);
    printf(" [Count:%zu | H:%zu]", treeSize(t), treeHeight(t));

    if (t->root == NULL) {
        tuiGoToXY(x, y + 2);
        printf("( Empty )");
        return;
    }

    size_t h = treeHeight(t);
    int initialOffset = (int)(1 << (h - 1)) * 3; 
    if (initialOffset < 10) initialOffset = 10;
    if (initialOffset > 40) initialOffset = 40;

    _tuiDrawTreeNode(t->root, x, y + 2, initialOffset, printFunc);
}

void tuiDrawHashMap(int x, int y, HashMap map, void (*printKey)(void*), void (*printVal)(void*)) {
    if (!map) return;

    tuiGoToXY(x, y);
    tuiColor(TUI_CYAN);
    printf("HashMap");
    tuiColor(TUI_WHITE);
    printf(" [Size:%zu | Buckets:%zu]", map->count, map->capacity);

    int currentY = y + 2;

    for (size_t i = 0; i < map->capacity; i++) {
        tuiGoToXY(x, currentY);
        
        tuiColor(TUI_YELLOW);
        printf("[%02zu]", i);
        tuiColor(TUI_DEFAULT);
        
        MapEntry* entry = map->buckets[i];
        
        if (entry == NULL) {
            tuiColor(TUI_WHITE);
            printf(" ─ NULL");
        } else {
            while (entry) {
                printf(" ─> ");
                
                tuiStyle(TUI_STYLE_BOLD);
                printf("[");
                
                tuiColor(TUI_GREEN);
                if (printKey) printKey(entry->key);
                else printf("K");
                
                tuiColor(TUI_WHITE);
                printf(":");
                
                tuiColor(TUI_CYAN);
                if (printVal) printVal(entry->value);
                else printf("V");
                
                tuiColor(TUI_DEFAULT);
                printf("]");
                tuiStyle(TUI_STYLE_RESET);

                entry = entry->next;
            }
        }
        currentY++;
    }
}

void tuiDrawSet(int x, int y, Set set, void (*printKey)(void*)) {
    if (!set) return;

    tuiGoToXY(x, y);
    tuiColor(TUI_CYAN);
    printf("HashSet");
    tuiColor(TUI_WHITE);
    printf(" [Size:%zu | Buckets:%zu]", set->map->count, set->map->capacity);

    int currentY = y + 2;
    HashMap map = set->map;

    for (size_t i = 0; i < map->capacity; i++) {
        tuiGoToXY(x, currentY);
        tuiColor(TUI_YELLOW);
        printf("[%02zu]", i);
        tuiColor(TUI_DEFAULT);
        
        MapEntry* entry = map->buckets[i];
        
        if (entry == NULL) {
            tuiColor(TUI_WHITE); 
            printf(" ─ •");
        } else {
            while (entry) {
                printf(" ─> ");
                tuiStyle(TUI_STYLE_BOLD);
                printf("(");
                tuiColor(TUI_GREEN);
                if (printKey) printKey(entry->key);
                tuiColor(TUI_DEFAULT);
                printf(")");
                tuiStyle(TUI_STYLE_RESET);
                entry = entry->next;
            }
        }
        currentY++;
    }
}

void tuiDrawPixel(int x, int y, const char* c) {
    if (x >= 0 && x < _internalTermWidth && y >= 0 && y < _internalTermHeight) {
        tuiGoToXY(x + 1, y + 1);
        printf("%s", c);
    }
}

void tuiDrawLine(int x0, int y0, int x1, int y1) {
    int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy, e2;

    while (1) {
        tuiDrawPixel(x0, y0, "*"); 
        if (x0 == x1 && y0 == y1) break;
        e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

void tuiDrawRect(int x, int y, int w, int h) {
    if (w <= 0 || h <= 0) return;

    if (w == 1) {
        for (int i = 0; i < h; i++) tuiDrawPixel(x, y + i, "│");
        return;
    }
    if (h == 1) {
        for (int i = 0; i < w; i++) tuiDrawPixel(x + i, y, "─");
        return;
    }

    tuiDrawPixel(x, y, "┌");
    tuiDrawPixel(x + w - 1, y, "┐");
    tuiDrawPixel(x, y + h - 1, "└");
    tuiDrawPixel(x + w - 1, y + h - 1, "┘");

    for (int i = 1; i < w - 1; i++) {
        tuiDrawPixel(x + i, y, "─");
        tuiDrawPixel(x + i, y + h - 1, "─");
    }

    for (int i = 1; i < h - 1; i++) {
        tuiDrawPixel(x, y + i, "│");             
        tuiDrawPixel(x + w - 1, y + i, "│");  
    }
}

void tuiPlotPoint(double x, double y, const char* c) {
    if (x < _viewport.minX || x > _viewport.maxX || 
        y < _viewport.minY || y > _viewport.maxY) return;
        
    tuiDrawPixel(projectX(x), projectY(y), c);
}

void tuiPlotLine(double x1, double y1, double x2, double y2) {
    tuiDrawLine(_projectX(x1), _projectY(y1), _projectX(x2), _projectY(y2));
}

void tuiPlotAxes() {
    if (_viewport.minX <= 0 && _viewport.maxX >= 0) {
        int screenX = projectX(0.0);
        for(int y = 0; y < _internalTermHeight; y++) {
            tuiDrawPixel(screenX, y, "│");
        }
    }
    
    if (_viewport.minY <= 0 && _viewport.maxY >= 0) {
        int screenY = projectY(0.0);
        for(int x = 0; x < _internalTermWidth; x++) {
            tuiDrawPixel(x, screenY, "─");
        }
    }
    
    if (_viewport.minX <= 0 && _viewport.maxX >= 0 && 
        _viewport.minY <= 0 && _viewport.maxY >= 0) {
        tuiDrawPixel(projectX(0), projectY(0), "┼");
    }
}

void tuiPlotFunc(double (*func)(double), double step) {
    if (step <= 0) {
        step = (_viewport.maxX - _viewport.minX) / (double)_internalTermWidth;
    }
    
    double prevX = _viewport.minX;
    double prevY = func(prevX);
    
    for (double x = _viewport.minX + step; x <= _viewport.maxX; x += step) {
        double y = func(x);
        tuiPlotLine(prevX, prevY, x, y);
        prevX = x;
        prevY = y;
    }
}

int tuiGetTerminalWidth() {
    if (_internalTermWidth == 0) _updateDimensions();
    return _internalTermWidth;
}

int tuiGetTerminalHeight() {
    if (_internalTermHeight == 0) _updateDimensions();
    return _internalTermHeight;
}

bool tuiHasResized() {
    bool resized = false;
#ifdef _WIN32
    int currentW, currentH;
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
    currentW = csbi.srWindow.Right - csbi.srWindow.Left + 1;
    currentH = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
    
    if (currentW != _internalTermWidth || currentH != _internalTermHeight) {
        _internalTermWidth = currentW;
        _internalTermHeight = currentH;
        resized = true;
    }
#else
    if (_resizedFlag) {
        _resizedFlag = 0;
        _updateDimensions();
        resized = true;
    }
#endif
    return resized;
}

void tuiSetViewport(double minX, double maxX, double minY, double maxY) {
    _viewport.minX = minX;
    _viewport.maxX = maxX;
    _viewport.minY = minY;
    _viewport.maxY = maxY;
}

TuiViewport tuiGetViewport() {
    return _viewport;
}

int projectX(double x) {
    double range = _viewport.maxX - _viewport.minX;
    if (range == 0) return 0;
    double percent = (x - _viewport.minX) / range;
    return (int)(percent * _internalTermWidth);
}

int projectY(double y) {
    double range = _viewport.maxY - _viewport.minY;
    if (range == 0) return 0;
    double percent = (y - _viewport.minY) / range;
    return (_internalTermHeight - 1) - (int)(percent * _internalTermHeight);
}