#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
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
}

void tuiInit() {
    signal(SIGINT, _tuiSignalHandler);

#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD dwMode = 0;
    GetConsoleMode(hOut, &dwMode);
    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(hOut, dwMode | ENABLE_PROCESSED_INPUT);
#else
    enableRawMode();
#endif
    tuiCursorVisible(false);
    tuiClearScreen();
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

void tuiPrintTable(int x, int y, Array headers, Array rows) {
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
    if (read(STDIN_FILENO, &c, 1) == 1) {
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