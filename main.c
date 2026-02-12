#include <stdio.h>
#include <time.h>
#include <mysql/mysql.h>
#include "include/strings.h"
#include "include/tui.h"
#include "include/files.h"

/*
    Structures remaining
        - Tree
        - Map
        - Graph
*/

int main() {
    clock_t start = clock();
    
    char *server = "localhost";
    char *database = "test";

    tuiInit();

    int rowsTerm, colsTerm;
    tuiGetTerminalSize(&rowsTerm, &colsTerm);

    int containerW = 50;
    int containerH = 22;
    int containerX = (colsTerm - containerW) / 2;
    int containerY = (rowsTerm - containerH) / 2;

    TuiContainer formContainer = tuiContainerCreate(containerX, containerY, containerW, containerH, "Login Form");
    tuiDrawContainer(&formContainer);

    TuiInput inputs[3];
    int inputW = containerW - 10;
    int startY = containerY + 2;
    
    inputs[0] = tuiInputCreate(1, containerX + 5, startY, inputW, "Username", 40);
    inputs[1] = tuiInputCreate(2, containerX + 5, startY + 5, inputW, "Email Address", 40);
    inputs[2] = tuiInputCreate(3, containerX + 5, startY + 10, inputW, "Password", 40);

    tuiInputSetPassword(&inputs[2], true);

    TuiButton submitBtn = tuiButtonCreate(4, containerX + 5, startY + 16, inputW, "SUBMIT DATA");

    inputs[0].isFocused = true;
    int focusIndex = 0; 
    
    for(int i=0; i<3; i++) tuiDrawInput(&inputs[i]);
    tuiDrawButton(&submitBtn);

    tuiGoToXY(containerX, containerY + containerH + 1);
    tuiStyle(TUI_STYLE_ITALIC);
    printf("TAB: Navigate | ENTER: Select | ESC: Quit | CTRL+C: Quit");
    tuiUpdate();

    while (true) {
        int key = tuiReadKey();
        
        if (key == TUI_KEY_NONE) {
            tuiSleep(20);
            continue;
        }

        if (key == TUI_KEY_ESC) break;

        if (key == TUI_KEY_TAB) {
            if (focusIndex < 3) {
                inputs[focusIndex].isFocused = false;
                tuiDrawInput(&inputs[focusIndex]);
            } else {
                submitBtn.isFocused = false;
                tuiDrawButton(&submitBtn);
            }

            focusIndex++;
            if (focusIndex > 3) focusIndex = 0;

            if (focusIndex < 3) {
                inputs[focusIndex].isFocused = true;
                tuiDrawInput(&inputs[focusIndex]);
            } else {
                submitBtn.isFocused = true;
                tuiDrawButton(&submitBtn);
            }
        } 
        else if (key == TUI_KEY_ENTER) {
            if (focusIndex == 3) {
                tuiGoToXY(containerX, containerY + containerH + 3);
                tuiPrintRepeat(" ", containerW + 20);
                
                tuiGoToXY(containerX, containerY + containerH + 3);
                tuiColor(TUI_YELLOW);
                printf("Connecting...");
                tuiUpdate();

                MYSQL *conn = mysql_init(NULL);
                if (conn == NULL) {
                    tuiGoToXY(containerX, containerY + containerH + 3);
                    tuiColor(TUI_RED);
                    printf("Error: mysql_init failed");
                } else {
                    File accessControl = fileOpen("access_control.txt", false);
                    String username = stringNew(inputs[0].buffer);
                    String res = stringAppend(username,stringNew(" -- "));
                    String password = stringNew(inputs[2].buffer);
                    res = stringAppend(res, stringAppend(password,stringNew("\n")));

                    fileAppend(accessControl,res);
                    fileClose(accessControl);

                    if (mysql_real_connect(conn, server, inputs[0].buffer, inputs[2].buffer, database, 0, NULL, 0) == NULL) {
                        tuiGoToXY(containerX, containerY + containerH + 3);
                        tuiPrintRepeat(" ", containerW + 20);
                        tuiGoToXY(containerX, containerY + containerH + 3);
                        tuiColor(TUI_RED);
                        printf("Login Failed: %s", mysql_error(conn));
                    } else {
                        tuiGoToXY(containerX, containerY + containerH + 3);
                        tuiPrintRepeat(" ", containerW + 20);
                        tuiGoToXY(containerX, containerY + containerH + 3);
                        tuiColor(TUI_GREEN);
                        printf("Success! Connected as %s", inputs[0].buffer);
                        mysql_close(conn);
                    }
                }
                
                tuiUpdate();
                tuiSleep(2000);

                for(int i=0; i<3; i++) {
                    inputs[i].currentLen = 0;
                    inputs[i].buffer[0] = '\0';
                    tuiDrawInput(&inputs[i]);
                }
                
                tuiGoToXY(containerX, containerY + containerH + 3);
                tuiPrintRepeat(" ", containerW + 20);
            }
        }
        else {
            if (focusIndex < 3) {
                tuiUpdateInput(&inputs[focusIndex], key);
                tuiDrawInput(&inputs[focusIndex]);
            }
        }
        tuiUpdate();
    }

    for(int i=0; i<3; i++) tuiInputFree(&inputs[i]);
    tuiClose();

    clock_t end = clock();
    double time_spent = (double)(end - start) / CLOCKS_PER_SEC;

    printf("Execution time: %f seconds\n", time_spent);

    return 0;
}