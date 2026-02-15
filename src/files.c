#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/files.h"
#include "../include/pointers.h"
#include "../include/strings.h"
#include "../include/arrays.h"

struct FileStruct {
    FILE* fp;
    size_t size;
};

static bool fileCheck(File f) {
    return !null(f) && !null(f->fp);
}

static void rowDestructor(void* elem) {
    Array* row = (Array*)elem;
    if (!null(row) && !null(*row)) {
        arrayFree(*row, stringFreeRef);
    }
}

File fileOpen(const char* filename, bool writeMode) {
    if (filename == NULL) return NULL;
    
    File f = (File)xMalloc(sizeof(struct FileStruct));
    if (f == NULL) return NULL;

    if (writeMode) {
        f->fp = fopen(filename, "w+b");
        f->size = 0;
    } else {
        f->fp = fopen(filename, "r+b");
    }

    if (f->fp == NULL) {
        xFree(f);
        return NULL;
    }

    if (!writeMode) {
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
    if (f->fp != NULL) {
        fclose(f->fp);
    }
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

    size_t read = fread(s->data, 1, f->size, f->fp);
    if (read != f->size) {
        if (ferror(f->fp)) {
            stringFree(s);
            rewind(f->fp);
            return NULL;
        }
        s->len = read;
        s->data[read] = '\0';
    }
    
    rewind(f->fp);
    return s;
}

Array fileGetBytes(File f) {
    if (!fileCheck(f)) return NULL;
    
    fseek(f->fp, 0, SEEK_END);
    f->size = (size_t)ftell(f->fp);
    rewind(f->fp);

    if (f->size == 0) return array(1); 

    void* buffer = xMalloc(f->size);
    if (buffer == NULL) return NULL;

    if (fread(buffer, 1, f->size, f->fp) != f->size) {
        xFree(buffer);
        return NULL;
    }
    rewind(f->fp);
    
    Array arr = array(1);
    for(size_t i=0; i<f->size; i++) {
        char b = ((char*)buffer)[i];
        arrayAdd(arr, &b);
    }
    xFree(buffer);
    return arr;
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

    if (start <= len) {
        if (start < len) {
            String line = stringSubstring(text, start, len);
            arrayAdd(lines, &line);
        } else if (len > 0 && raw[len-1] == '\n') {
        }
    }

    stringFree(text);
    return lines;
}

Array fileGetCSV(File f, char separator) {
    Array lines = fileGetLines(f);
    if (lines == NULL) return NULL;

    Array grid = array(sizeof(Array));

    for (size_t i = 0; i < lines->len; i++) {
        String* linePtr = (String*)arrayGetRef(lines, i);
        if (linePtr == NULL || *linePtr == NULL) continue;

        String sLine = *linePtr;
        Array row = array(sizeof(String));
        
        size_t start = 0;
        size_t len = stringLength(sLine);
        char* raw = stringGetData(sLine);

        for (size_t j = 0; j < len; j++) {
            if (raw[j] == separator) {
                String cell = stringSubstring(sLine, start, j);
                arrayAdd(row, &cell);
                start = j + 1;
            }
        }
        String lastCell = stringSubstring(sLine, start, len);
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
    
    size_t written = fwrite(s->data, 1, s->len, f->fp);
    fflush(f->fp);
    f->size = written;
    rewind(f->fp);
    return written == s->len;
}

bool fileSetBytes(File f, Array bytes) {
    if (!fileCheck(f) || null(bytes)) return false;
    if (!fileClear(f)) return false;

    size_t count = 0;
    for (size_t i = 0; i < bytes->len; i++) {
        void* elem = arrayGetRef(bytes, i);
        if (fwrite(elem, bytes->esize, 1, f->fp) == 1) {
            count++;
        }
    }
    
    fflush(f->fp);
    f->size = count * bytes->esize;
    rewind(f->fp);
    return count == bytes->len;
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
        if (linePtr && *linePtr) {
            fwrite((*linePtr)->data, 1, (*linePtr)->len, f->fp);
            f->size += (*linePtr)->len;
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

    String* target = (String*)arrayGetRef(lines, lineIndex);
    if (target) {
        stringFree(*target);
        *target = stringNew(line->data); 
    }

    bool success = fileWriteLines(f, lines);

    arrayFree(lines, stringFreeRef);

    return success;
}