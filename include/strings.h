#ifndef STRINGS_H
#define STRINGS_H

#include <stdbool.h>
#include <stddef.h>
#include "arrays.h"

typedef struct {
    char* data;
    size_t len;
} StringStruct;

typedef StringStruct* String;

bool stringIsNull(const String s);
bool stringIsEmpty(const String s);

void stringFree(String s);
void stringFreeRef(void* elem);
void stringsFree(String* s);

String stringNewEmpty(size_t size);
String stringNew(const char* s);
String stringFromChar(const char c);
String stringFromInt(int value);
String stringFromFloat(float value);

size_t stringLength(const String s);
char* stringGetData(const String s);
char stringCharAt(const String s, int index);
unsigned char stringCharCodeAt(const String s, int index);

String stringAppend(String s1, const String s2);
String stringClear(String s);
String stringSubstring(String s, size_t start, size_t end);
String stringTrim(String s);
String stringReplace(String s, const String target, const String replacement);
String stringReplaceChar(const String s, char base, char replace);
String stringToUpperCase(String s);
String stringToLowerCase(String s);
String stringReverse(String s);
String stringSortChars(String s, SortComparator cmpf);

String* stringSplit(const String s, const String regex);
String stringJoin(String* parts, const String delimiter);

bool stringEquals(const String s1, const String s2);
bool stringEqualsIgnoreCase(const String s1, const String s2);
int stringCompare(const String s1, const String s2);
bool stringContains(const String s, const String needle);
bool stringStartsWith(const String s, const String prefix);
bool stringEndsWith(const String s, const String suffix);
int stringIndexOf(const String s, const String needle);  
int stringIndexOfChar(const String s, char c);
int stringLastIndexOf(const String s, const String needle);

int stringParseInt(const String s);
double stringParseDouble(const String s);
int stringParseHex(const String s);
int stringParseBinary(const String s);
int stringParseOctal(const String s);

#endif