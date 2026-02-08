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

bool strNull(const String s);
bool empty(const String s);
void freeString(String s);
void freeStrings(String* s);
String emptyString(size_t size);
size_t length(const String s);
String string(const char* s);
String append(const String s1, const String s2);
String clear(String s);
char charAt(const String s, int index);
unsigned char charCodeAt(const String s, int index);
String charAsString(const char c);
String substring(const String s, const size_t start, const size_t end);
String* split(const String s,const String regex);
bool equals(const String s1, const String s2);
bool equalsIgnoreCase(const String s1, const String s2);
int compare(const String s1, const String s2);
int indexOf(const String s, const String needle);  
int indexOfChar(const String s, char c);
int lastIndexOf(const String s, const String needle);
bool contains(const String s, const String needle);
bool startsWith(const String s, const String prefix);
bool endsWith(const String s, const String suffix);
String trim(const String s);
String replace(const String s, const String target, const String replacement);
String toUpperCase(const String s);
String toLowerCase(const String s);
String join(String* parts, const String delimiter);
String fromInt(int value);
String fromFloat(float value);
String sort(const String s, SortComparator cmpf);
String reverse(String s);
int parseInt(const String s);
double parseDouble(const String s);
char* toCharPointer(const String s);
int SORT_ASC(const void *a, const void *b);
int SORT_DESC(const void *a, const void *b);

#endif