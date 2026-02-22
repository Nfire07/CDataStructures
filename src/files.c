#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <limits.h>

#ifdef _WIN32
    #include <direct.h>
    #define GetCurrentDir _getcwd
    #define realpath(N,R) _fullpath((R),(N),_MAX_PATH)
#else
    #include <unistd.h>
    #define GetCurrentDir getcwd
#endif

#include "../include/files.h"
#include "../include/pointers.h"
#include "../include/strings.h"
#include "../include/arrays.h"

struct FileStruct {
    FILE  *fp;
    size_t size;
    char  *path;
};

static bool fileCheck(File f) {
    return !null(f) && !null(f->fp);
}

static void rowDestructor(void *elem) {
    Array *row = (Array *)elem;
    if (!null(row) && !null(*row)) {
        arrayFree(*row, stringFreeRef);
    }
}

File fileOpen(const char *filename, bool writeMode) {
    if (filename == NULL) return NULL;

    File f = (File)xMalloc(sizeof(struct FileStruct));
    if (f == NULL) return NULL;

    f->path = (char *)xMalloc(strlen(filename) + 1);
    if (f->path == NULL) {
        xFree(f);
        return NULL;
    }
    strcpy(f->path, filename);

    if (writeMode) {
        f->fp   = fopen(filename, "w+b");
        f->size = 0;
    } else {
        f->fp = fopen(filename, "r+b");
    }

    if (f->fp == NULL) {
        xFree(f->path);
        xFree(f);
        return NULL;
    }

    if (!writeMode) {
        if (fseek(f->fp, 0, SEEK_END) != 0) {
            fclose(f->fp);
            xFree(f->path);
            xFree(f);
            return NULL;
        }
        long fsize = ftell(f->fp);
        if (fsize < 0) {
            fclose(f->fp);
            xFree(f->path);
            xFree(f);
            return NULL;
        }
        f->size = (size_t)fsize;
        rewind(f->fp);
    }

    return f;
}

void fileClose(File f) {
    if (null(f)) return;
    if (f->fp != NULL) fclose(f->fp);
    if (f->path != NULL) xFree(f->path);
    xFree(f);
}

bool fileException(File f) {
    return !fileCheck(f);
}

String fileGetText(File f) {
    if (!fileCheck(f)) return NULL;

    fseek(f->fp, 0, SEEK_END);
    f->size = (size_t)ftell(f->fp);
    rewind(f->fp);

    if (f->size == 0) return stringNewEmpty(0);

    String s = stringNewEmpty(f->size);
    if (s == NULL) return NULL;

    size_t readBytes = fread(s->data, 1, f->size, f->fp);
    if (readBytes != f->size) {
        if (ferror(f->fp)) {
            stringFree(s);
            rewind(f->fp);
            return NULL;
        }
        s->len = readBytes;
        s->data[readBytes] = '\0';
    }

    rewind(f->fp);
    return s;
}

Array fileGetLines(File f) {
    String text = fileGetText(f);
    if (stringIsNull(text)) return NULL;

    Array lines = array(sizeof(String));
    char  *raw  = stringGetData(text);
    size_t len  = stringLength(text);
    size_t start = 0;

    for (size_t i = 0; i < len; i++) {
        if (raw[i] == '\n') {
            size_t end = i;
            if (end > start && raw[end - 1] == '\r') end--;
            String line = stringSubstring(text, start, end);
            arrayAdd(lines, &line);
            start = i + 1;
        }
    }

    if (start < len) {
        String line = stringSubstring(text, start, len);
        arrayAdd(lines, &line);
    }

    stringFree(text);
    return lines;
}

Array fileGetCSV(File f, char separator) {
    Array lines = fileGetLines(f);
    if (lines == NULL) return NULL;

    Array grid = array(sizeof(Array));

    for (size_t i = 0; i < lines->len; i++) {
        String *linePtr = (String *)arrayGetRef(lines, i);
        if (linePtr == NULL || *linePtr == NULL) continue;

        String sLine  = *linePtr;
        Array  row    = array(sizeof(String));
        size_t start  = 0;
        size_t slen   = stringLength(sLine);
        char  *raw    = stringGetData(sLine);

        for (size_t j = 0; j < slen; j++) {
            if (raw[j] == separator) {
                String cell = stringSubstring(sLine, start, j);
                arrayAdd(row, &cell);
                start = j + 1;
            }
        }
        String lastCell = stringSubstring(sLine, start, slen);
        arrayAdd(row, &lastCell);
        arrayAdd(grid, &row);
    }

    arrayFree(lines, stringFreeRef);
    return grid;
}

void fileFreeCSV(Array csv) {
    if (null(csv)) return;
    arrayFree(csv, rowDestructor);
}

bool fileClear(File f) {
    if (!fileCheck(f)) return false;
    if (freopen(NULL, "w+b", f->fp) == NULL) return false;
    f->size = 0;
    return true;
}

bool fileSetText(File f, String s) {
    if (!fileCheck(f) || stringIsNull(s)) return false;
    if (!fileClear(f)) return false;

    size_t written = fwrite(stringGetData(s), 1, stringLength(s), f->fp);
    fflush(f->fp);
    f->size = written;
    rewind(f->fp);
    return written == stringLength(s);
}

bool fileSetBytes(File f, Array bytes) {
    if (!fileCheck(f) || null(bytes)) return false;
    if (!fileClear(f)) return false;

    size_t count = 0;
    for (size_t i = 0; i < bytes->len; i++) {
        void *elem = arrayGetRef(bytes, i);
        if (fwrite(elem, bytes->esize, 1, f->fp) == 1) count++;
    }

    fflush(f->fp);
    f->size = count * bytes->esize;
    rewind(f->fp);
    return count == bytes->len;
}

bool fileAppend(File f, String s) {
    if (!fileCheck(f) || stringIsNull(s)) return false;

    fseek(f->fp, 0, SEEK_END);
    size_t written = fwrite(stringGetData(s), 1, stringLength(s), f->fp);
    f->size += written;
    rewind(f->fp);
    return written == stringLength(s);
}

bool fileWriteLines(File f, Array lines) {
    if (!fileCheck(f) || null(lines)) return false;
    if (!fileClear(f)) return false;

    for (size_t i = 0; i < lines->len; i++) {
        String *linePtr = (String *)arrayGetRef(lines, i);
        if (linePtr && *linePtr) {
            fwrite(stringGetData(*linePtr), 1, stringLength(*linePtr), f->fp);
            f->size += stringLength(*linePtr);
        }
        if (i < lines->len - 1) {
            fputc('\n', f->fp);
            f->size++;
        }
    }

    fflush(f->fp);
    rewind(f->fp);
    return true;
}

bool fileSetLine(File f, String line, size_t lineIndex) {
    if (!fileCheck(f) || stringIsNull(line)) return false;

    Array lines = fileGetLines(f);
    if (null(lines)) return false;

    if (lineIndex >= lines->len) {
        arrayFree(lines, stringFreeRef);
        return false;
    }

    String *target = (String *)arrayGetRef(lines, lineIndex);
    if (target) {
        stringFree(*target);
        *target = stringNew(stringGetData(line));
    }

    bool success = fileWriteLines(f, lines);
    arrayFree(lines, stringFreeRef);
    return success;
}

Array fileGetList(const char *dirPath, bool hidden) {
    if (dirPath == NULL) return NULL;

    DIR *d = opendir(dirPath);
    if (d == NULL) return NULL;

    Array list = array(sizeof(String));
    struct dirent *dir;

    while ((dir = readdir(d)) != NULL) {
        if (strcmp(dir->d_name, ".") == 0 || strcmp(dir->d_name, "..") == 0) continue;
        if (!hidden && dir->d_name[0] == '.') continue;
        String name = stringNew(dir->d_name);
        arrayAdd(list, &name);
    }

    closedir(d);
    return list;
}

String fileGetAbsPath(File f) {
    if (!fileCheck(f) || f->path == NULL) return NULL;

    char buffer[4096];
    if (realpath(f->path, buffer) == NULL) return NULL;
    return stringNew(buffer);
}


static void _bmpWriteU16(FILE *fp, unsigned short v) {
    fputc(v & 0xFF, fp);
    fputc((v >> 8) & 0xFF, fp);
}

static void _bmpWriteU32(FILE *fp, unsigned int v) {
    fputc( v        & 0xFF, fp);
    fputc((v >>  8) & 0xFF, fp);
    fputc((v >> 16) & 0xFF, fp);
    fputc((v >> 24) & 0xFF, fp);
}

static BmpColor _paletteLookup(BmpPalette *p, char ch) {
    for (size_t i = 0; i < p->count; i++) {
        if (p->entries[i].ch == ch) return p->entries[i].color;
    }
    return p->background;
}

static bool _bmpRender(const char *text, const char *outPath, BmpPalette *p) {
    int cols = 0, rows = 1, curCol = 0;
    for (const char *c = text; *c; c++) {
        if (*c == '\n') {
            if (curCol > cols) cols = curCol;
            curCol = 0;
            rows++;
        } else {
            curCol++;
        }
    }
    if (curCol > cols) cols = curCol;
    if (cols == 0 || rows == 0) return false;

    int imgW   = cols * p->charW;
    int imgH   = rows * p->charH;
    int stride = ((imgW * 3 + 3) / 4) * 4;
    int pixSz  = stride * imgH;

    unsigned char *pixels = (unsigned char *)xMalloc((size_t)pixSz);
    if (!pixels) return false;

    for (int y = 0; y < imgH; y++) {
        for (int x = 0; x < imgW; x++) {
            int idx = (imgH - 1 - y) * stride + x * 3;
            pixels[idx + 0] = p->background.b;
            pixels[idx + 1] = p->background.g;
            pixels[idx + 2] = p->background.r;
        }
    }

    int row = 0, col = 0;
    for (const char *c = text; *c; c++) {
        if (*c == '\n') { row++; col = 0; continue; }
        BmpColor color = _paletteLookup(p, *c);
        for (int dy = 0; dy < p->charH; dy++) {
            for (int dx = 0; dx < p->charW; dx++) {
                int px  = col * p->charW + dx;
                int py  = row * p->charH + dy;
                int idx = (imgH - 1 - py) * stride + px * 3;
                pixels[idx + 0] = color.b;
                pixels[idx + 1] = color.g;
                pixels[idx + 2] = color.r;
            }
        }
        col++;
    }

    FILE *fp = fopen(outPath, "wb");
    if (!fp) { xFree(pixels); return false; }

    unsigned int fileSize = 54 + (unsigned int)pixSz;
    fputc('B', fp); fputc('M', fp);
    _bmpWriteU32(fp, fileSize);
    _bmpWriteU32(fp, 0);
    _bmpWriteU32(fp, 54);
    _bmpWriteU32(fp, 40);
    _bmpWriteU32(fp, (unsigned int)imgW);
    _bmpWriteU32(fp, (unsigned int)imgH);
    _bmpWriteU16(fp, 1);
    _bmpWriteU16(fp, 24);
    _bmpWriteU32(fp, 0);
    _bmpWriteU32(fp, (unsigned int)pixSz);
    _bmpWriteU32(fp, 2835);
    _bmpWriteU32(fp, 2835);
    _bmpWriteU32(fp, 0);
    _bmpWriteU32(fp, 0);
    fwrite(pixels, 1, (size_t)pixSz, fp);

    fclose(fp);
    xFree(pixels);
    return true;
}

BmpPalette bmpPaletteCreate(int charW, int charH, BmpColor background) {
    BmpPalette p;
    p.entries    = NULL;
    p.count      = 0;
    p.background = background;
    p.charW      = charW;
    p.charH      = charH;
    return p;
}

void bmpPaletteAdd(BmpPalette *p, char ch, BmpColor color) {
    BmpEntry *newEntries = (BmpEntry *)xRealloc(p->entries, (p->count + 1) * sizeof(BmpEntry));
    if (!newEntries) return;
    p->entries = newEntries;
    p->entries[p->count].ch    = ch;
    p->entries[p->count].color = color;
    p->count++;
}

void bmpPaletteFree(BmpPalette *p) {
    if (p->entries) xFree(p->entries);
    p->entries = NULL;
    p->count   = 0;
}

BmpPalette bmpPaletteLoad(const char *mapPath, int charW, int charH, BmpColor background) {
    BmpPalette p = bmpPaletteCreate(charW, charH, background);
    FILE *fp = fopen(mapPath, "r");
    if (!fp) return p;

    char line[128];
    while (fgets(line, sizeof(line), fp)) {
        if (line[0] == '#' || line[0] == '\n') continue;
        char ch;
        int  r, g, b;
        if (sscanf(line, " %c %d %d %d", &ch, &r, &g, &b) == 4) {
            bmpPaletteAdd(&p, ch, BMP_RGB(
                (unsigned char)r,
                (unsigned char)g,
                (unsigned char)b
            ));
        }
    }

    fclose(fp);
    return p;
}

bool bmpWrite(const char *text, const char *outPath, BmpPalette *p) {
    if (!text || !outPath || !p) return false;
    return _bmpRender(text, outPath, p);
}

bool bmpWriteS(String text, const char *outPath, BmpPalette *p) {
    if (stringIsNull(text)) return false;
    return _bmpRender(stringGetData(text), outPath, p);
}

bool bmpWriteF(File f, const char *outPath, BmpPalette *p) {
    if (!fileCheck(f)) return false;
    String text = fileGetText(f);
    if (stringIsNull(text)) return false;
    bool ok = _bmpRender(stringGetData(text), outPath, p);
    stringFree(text);
    return ok;
}

static bool _colorEquals(BmpColor a, BmpColor b) {
    return a.r == b.r && a.g == b.g && a.b == b.b;
}

static char _pickChar(bool *used, const char *preferred, size_t preferredLen, size_t *prefIdx) {
    if (preferred && *prefIdx < preferredLen) {
        char c = preferred[(*prefIdx)++];
        if (c >= 33 && c <= 126 && !used[(unsigned char)c]) {
            used[(unsigned char)c] = true;
            return c;
        }
    }
    for (int c = 33; c <= 126; c++) {
        if (!used[c]) {
            used[c] = true;
            return (char)c;
        }
    }
    return '?';
}

String bmpRead(const char *bmpPath, int charW, int charH,
               const char *preferred, size_t preferredLen) {
    if (!bmpPath || charW <= 0 || charH <= 0) return NULL;

    FILE *fp = fopen(bmpPath, "rb");
    if (!fp) return NULL;

    unsigned char header[54];
    if (fread(header, 1, 54, fp) != 54 || header[0] != 'B' || header[1] != 'M') {
        fclose(fp);
        return NULL;
    }

    unsigned int   dataOffset = *(unsigned int   *)&header[10];
    unsigned int   width      = *(unsigned int   *)&header[18];
    unsigned int   height     = *(unsigned int   *)&header[22];
    unsigned short bpp        = *(unsigned short *)&header[28];

    if (bpp != 24) { fclose(fp); return NULL; }

    int stride = (((int)width * 3 + 3) / 4) * 4;
    unsigned char *pixels = (unsigned char *)xMalloc((size_t)(stride * (int)height));
    if (!pixels) { fclose(fp); return NULL; }

    fseek(fp, (long)dataOffset, SEEK_SET);
    if (fread(pixels, 1, (size_t)(stride * (int)height), fp) != (size_t)(stride * (int)height)) {
        xFree(pixels);
        fclose(fp);
        return NULL;
    }
    fclose(fp);

    size_t cols = (size_t)width  / (size_t)charW;
    size_t rows = (size_t)height / (size_t)charH;

    size_t maxColors = cols * rows;
    if (maxColors == 0) { xFree(pixels); return NULL; }

    BmpColor *colorMap = (BmpColor *)xMalloc(maxColors * sizeof(BmpColor));
    char     *charMap  = (char     *)xMalloc(maxColors * sizeof(char));
    if (!colorMap || !charMap) {
        xFree(colorMap);
        xFree(charMap);
        xFree(pixels);
        return NULL;
    }
    size_t colorCount = 0;

    bool   used[128] = {false};
    size_t prefIdx   = 0;

    char *rowBuf = (char *)xMalloc(cols + 1);
    if (!rowBuf) {
        xFree(colorMap);
        xFree(charMap);
        xFree(pixels);
        return NULL;
    }

    String out = stringNewEmpty(0);
    if (stringIsNull(out)) {
        xFree(rowBuf);
        xFree(colorMap);
        xFree(charMap);
        xFree(pixels);
        return NULL;
    }

    for (size_t r = 0; r < rows; r++) {
        for (size_t c = 0; c < cols; c++) {
            size_t px  = c * (size_t)charW;
            size_t py  = r * (size_t)charH;
            int    idx = ((int)height - 1 - (int)py) * stride + (int)px * 3;

            BmpColor col = BMP_RGB(pixels[idx + 2], pixels[idx + 1], pixels[idx + 0]);

            char ch    = '?';
            bool found = false;
            for (size_t i = 0; i < colorCount; i++) {
                if (_colorEquals(colorMap[i], col)) {
                    ch    = charMap[i];
                    found = true;
                    break;
                }
            }

            if (!found) {
                ch                   = _pickChar(used, preferred, preferredLen, &prefIdx);
                colorMap[colorCount] = col;
                charMap[colorCount]  = ch;
                colorCount++;
            }

            rowBuf[c] = ch;
        }
        rowBuf[cols] = '\0';

        String rowStr = stringNew(rowBuf);
        out = stringAppend(out, rowStr);
        stringFree(rowStr);

        if (r < rows - 1) {
            String nl = stringFromChar('\n');
            out = stringAppend(out, nl);
            stringFree(nl);
        }
    }

    xFree(rowBuf);
    xFree(colorMap);
    xFree(charMap);
    xFree(pixels);
    return out;
}