#ifndef FILES_H
#define FILES_H

#include <stdio.h>
#include <stdbool.h>
#include "strings.h"
#include "arrays.h"

typedef struct FileStruct FileStruct;
typedef FileStruct *File;

File   fileOpen(const char *filename, bool empty);
void   fileClose(File f);
bool   fileException(File f);
String fileGetText(File f);
Array  fileGetLines(File f);
Array  fileGetCSV(File f, char separator);
void   fileFreeCSV(Array csv);
bool   fileClear(File f);
bool   fileSetText(File f, String s);
bool   fileSetBytes(File f, Array bytes);
bool   fileAppend(File f, String s);
bool   fileWriteLines(File f, Array lines);
bool   fileSetLine(File f, String line, size_t lineIndex);
Array  fileGetList(const char *dir, bool hidden);
String fileGetAbsPath(File f);

typedef struct {
    unsigned char r, g, b;
} BmpColor;

#define BMP_RGB(r, g, b) ((BmpColor){(r), (g), (b)})
#define BMP_BLACK        ((BmpColor){  0,   0,   0})
#define BMP_WHITE        ((BmpColor){255, 255, 255})
#define BMP_RED          ((BmpColor){255,   0,   0})
#define BMP_GREEN        ((BmpColor){  0, 255,   0})
#define BMP_BLUE         ((BmpColor){  0,   0, 255})

typedef struct {
    char     ch;
    BmpColor color;
} BmpEntry;

typedef struct {
    BmpEntry *entries;
    size_t    count;
    BmpColor  background;
    int       charW;
    int       charH;
} BmpPalette;

BmpPalette bmpPaletteCreate(int charW, int charH, BmpColor background);
void       bmpPaletteAdd(BmpPalette *p, char ch, BmpColor color);
void       bmpPaletteFree(BmpPalette *p);
BmpPalette bmpPaletteLoad(const char *mapPath, int charW, int charH, BmpColor background);
String bmpRead(const char *bmpPath, int charW, int charH,const char *preferred, size_t preferredLen);

bool bmpWrite(const char *text, const char *outPath, BmpPalette *p);
bool bmpWriteS(String text,     const char *outPath, BmpPalette *p);
bool bmpWriteF(File f,          const char *outPath, BmpPalette *p);

#endif