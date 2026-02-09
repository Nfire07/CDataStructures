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
void stringsFree(String* s);
String stringNewEmpty(size_t size);
size_t stringLength(const String s);
String stringNew(const char* s);
String stringAppend(const String s1, const String s2);
String stringClear(String s);
char stringCharAt(const String s, int index);
unsigned char stringCharCodeAt(const String s, int index);
String stringFromChar(const char c);
String stringSubstring(const String s, const size_t start, const size_t end);
String* stringSplit(const String s, const String regex);
bool stringEquals(const String s1, const String s2);
bool stringEqualsIgnoreCase(const String s1, const String s2);
int stringCompare(const String s1, const String s2);
int stringIndexOf(const String s, const String needle);  
int stringIndexOfChar(const String s, char c);
int stringLastIndexOf(const String s, const String needle);
bool stringContains(const String s, const String needle);
bool stringStartsWith(const String s, const String prefix);
bool stringEndsWith(const String s, const String suffix);
String stringTrim(const String s);
String stringReplace(const String s, const String target, const String replacement);
String stringToUpperCase(const String s);
String stringToLowerCase(const String s);
String stringJoin(String* parts, const String delimiter);
String stringFromInt(int value);
String stringFromFloat(float value);
String stringSortChars(const String s, SortComparator cmpf);
String stringReverse(String s);
int stringParseInt(const String s);
double stringParseDouble(const String s);
char* stringGetData(const String s);

#endif