#ifndef FILES_H
#define FILES_H

#include <stdio.h>
#include <stdbool.h>
#include "strings.h"
#include "arrays.h"

typedef struct {
    FILE* fp;
    size_t size;
} FileStruct;

typedef FileStruct* File;

File fileOpen(const char* filename, bool empty);
void fileClose(File f);
bool fileException(File f);
String fileGetText(File f);
Array fileGetLines(File f);
Array fileGetCSV(File f, char separator);
void fileFreeCSV(Array csv);
bool fileClear(File f);
bool fileSetText(File f, String s);
bool fileSetBytes(File f, Array bytes);
bool fileAppend(File f, String s);
bool fileWriteLines(File f, Array lines);
bool fileSetLine(File f, String line, size_t lineIndex);

#endif