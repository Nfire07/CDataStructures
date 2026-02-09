#include <stdio.h>
#include <stdbool.h>
#include "../include/files.h"
#include "../include/pointers.h"
#include "../include/strings.h"

static bool fileCheck(File f) {
    return !null(f) && !null(f->fp);
}

File fileOpen(const char* filename, bool empty) {
    if (null((void*)filename)) return NULL;
    
    File f = xMalloc(sizeof(FileStruct));

    if (empty) {
        f->fp = fopen(filename, "w+b");
        f->size = 0;
    } else {
        f->fp = fopen(filename, "r+b");
    }

    if (f->fp == NULL) {
        xFree(f);
        return NULL;
    }

    if (!empty) {
        if (fseek(f->fp, 0, SEEK_END) != 0) {
            fclose(f->fp);
            xFree(f);
            return NULL;
        }
        long fsize = ftell(f->fp);
        if (fsize < 0) {
            fclose(f->fp);
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
    if (!null(f->fp)) fclose(f->fp);
    xFree(f);
}

bool fileException(File f) {
    return !fileCheck(f);
}

String fileGetText(File f) {
    if (!fileCheck(f)) return NULL;
    
    if (f->size == 0) return stringNewEmpty(0);

    String s = stringNewEmpty(f->size);
    if (fread(s->data, 1, f->size, f->fp) != f->size) {
        stringFree(s);
        return NULL;
    }
    rewind(f->fp);
    return s;
}

Array fileGetBytes(File f) {
    if (!fileCheck(f)) return NULL;
    
    void* buffer = xMalloc(f->size);
    if (fread(buffer, 1, f->size, f->fp) != f->size) {
        xFree(buffer);
        return NULL;
    }
    rewind(f->fp);
    return arrayFromPtr(buffer, f->size, 1);
}

Array fileGetLines(File f) {
    String text = fileGetText(f);
    if (stringIsNull(text)) return NULL;

    Array lines = array(sizeof(String));
    char* raw = stringGetData(text);
    size_t len = stringLength(text);
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
        size_t end = len;
        if (end > start && raw[end - 1] == '\r') end--;
        
        String line = stringSubstring(text, start, end);
        arrayAdd(lines, &line);
    }

    stringFree(text);
    return lines;
}

Array fileGetCSV(File f, char separator) {
    String text = fileGetText(f);
    if (stringIsNull(text)) return NULL;

    Array grid = array(sizeof(Array));
    Array currentRow = array(sizeof(String));

    char* raw = stringGetData(text);
    size_t len = stringLength(text);
    size_t start = 0;

    for (size_t i = 0; i < len; i++) {
        char c = raw[i];

        if (c == separator) {
            String cell = stringSubstring(text, start, i);
            arrayAdd(currentRow, &cell);
            start = i + 1;
        } 
        else if (c == '\n') {
            size_t end = i;
            if (end > start && raw[end - 1] == '\r') end--;

            String cell = stringSubstring(text, start, end);
            arrayAdd(currentRow, &cell);
            arrayAdd(grid, &currentRow);

            currentRow = array(sizeof(String));
            start = i + 1;
        }
    }

    if (start < len) {
        String cell = stringSubstring(text, start, len);
        arrayAdd(currentRow, &cell);
        arrayAdd(grid, &currentRow);
    } else {
        if (currentRow->len == 0) {
            arrayFree(currentRow);
        } else {
            String emptyCell = stringNew("");
            arrayAdd(currentRow, &emptyCell);
            arrayAdd(grid, &currentRow);
        }
    }

    stringFree(text);
    return grid;
}

void fileFreeCSV(Array csv) {
    if (null(csv)) return;
    for (size_t i = 0; i < csv->len; i++) {
        Array* rowPtr = (Array*)arrayGetRef(csv, i);
        if (rowPtr && *rowPtr) {
            Array row = *rowPtr;
            for (size_t j = 0; j < row->len; j++) {
                String* cellPtr = (String*)arrayGetRef(row, j);
                if (cellPtr) stringFree(*cellPtr);
            }
            arrayFree(row);
        }
    }
    arrayFree(csv);
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
    
    size_t written = fwrite(s->data, 1, s->len, f->fp);
    f->size = written;
    rewind(f->fp);
    return written == s->len;
}

bool fileSetBytes(File f, Array bytes) {
    if (!fileCheck(f) || null(bytes)) return false;
    if (!fileClear(f)) return false;

    size_t written = fwrite(bytes->data, bytes->esize, bytes->len, f->fp);
    f->size = written * bytes->esize;
    rewind(f->fp);
    return written == bytes->len;
}

bool fileAppend(File f, String s) {
    if (!fileCheck(f) || stringIsNull(s)) return false;
    
    fseek(f->fp, 0, SEEK_END);
    size_t written = fwrite(s->data, 1, s->len, f->fp);
    f->size += written;
    rewind(f->fp);
    return written == s->len;
}

bool fileWriteLines(File f, Array lines) {
    if (!fileCheck(f) || null(lines)) return false;
    if (!fileClear(f)) return false;

    for (size_t i = 0; i < lines->len; i++) {
        String* linePtr = (String*)arrayGetRef(lines, i);
        fwrite((*linePtr)->data, 1, (*linePtr)->len, f->fp);
        f->size += (*linePtr)->len;
        
        if (i < lines->len - 1) {
            fputc('\n', f->fp);
            f->size++;
        }
    }
    rewind(f->fp);
    return true;
}

bool fileSetLine(File f, String line, size_t lineIndex) {
    if (!fileCheck(f) || stringIsNull(line)) return false;
    
    Array lines = fileGetLines(f);
    if (null(lines)) return false;

    if (lineIndex >= lines->len) {
        for(size_t i=0; i<lines->len; i++) stringFree(*(String*)arrayGetRef(lines, i));
        arrayFree(lines);
        return false;
    }

    String* target = (String*)arrayGetRef(lines, lineIndex);
    stringFree(*target);
    *target = stringNew(line->data);

    bool success = fileWriteLines(f, lines);

    for(size_t i=0; i<lines->len; i++) stringFree(*(String*)arrayGetRef(lines, i));
    arrayFree(lines);

    return success;
}