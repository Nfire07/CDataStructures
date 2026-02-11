#include "../include/strings.h"
#include "../include/arrays.h"
#include "../include/pointers.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <regex.h>
#include <stdarg.h>

bool stringIsNull(const String s) {
    return (s == NULL || s->data == NULL);
}

bool stringIsEmpty(const String s) {
    return (stringIsNull(s) || s->len == 0);
}

void stringFree(String s) {
    if (s != NULL) {
        if (s->data != NULL) xFree(s->data);
        xFree(s);
    }
}

void stringsFree(String* s) {
    if (s == NULL) return;
    for (size_t i = 0; s[i] != NULL; i++) {
        stringFree(s[i]);
    }
    xFree(s);
}

String stringNewEmpty(size_t size) {
    String s = (String)xMalloc(sizeof(StringStruct));
    if (s == NULL) return NULL;
    s->data = (char*)xCalloc(size + 1, sizeof(char));
    if (s->data == NULL) {
        xFree(s);
        return NULL;
    }
    s->len = size;
    return s;
}

size_t stringLength(const String s) {
    return stringIsNull(s) ? 0 : s->len;
}

String stringNew(const char* s) {
    if (s == NULL) return NULL;
    
    size_t len = strlen(s);
    String tmp = (String)xMalloc(sizeof(StringStruct));
    if (tmp == NULL) return NULL;
    
    tmp->data = (char*)xMalloc(len + 1);
    if (tmp->data == NULL) {
        xFree(tmp);
        return NULL;
    }
    
    memcpy(tmp->data, s, len);
    tmp->data[len] = '\0';
    tmp->len = len;
    return tmp;
}

String stringAppend(const String s1, const String s2) {
    if (stringIsNull(s1) && stringIsNull(s2)) return NULL;
    if (stringIsNull(s1)) return stringNew(s2->data);
    if (stringIsNull(s2)) return stringNew(s1->data);

    size_t l1 = s1->len;
    size_t l2 = s2->len;
    String tmp = stringNewEmpty(l1 + l2);
    if (stringIsNull(tmp)) return NULL;

    memcpy(tmp->data, s1->data, l1);
    memcpy(tmp->data + l1, s2->data, l2);
    tmp->data[l1 + l2] = '\0';
    tmp->len = l1 + l2;

    return tmp;
}

String stringClear(String s) {
    stringFree(s);
    return stringNew("");
}

char stringCharAt(const String s, int index) {
    if (stringIsNull(s)) return '\0';
    int len = (int)s->len;
    if (index >= len || index < -len) return '\0';
    if (index >= 0) return s->data[index];
    return s->data[len + index];
}

unsigned char stringCharCodeAt(const String s, int index) {
    return (unsigned char)stringCharAt(s, index);
}

String stringFromChar(const char c) {
    char buf[2] = {c, '\0'};
    return stringNew(buf);
}

String stringSubstring(const String s, const size_t start, const size_t end) {
    if (stringIsNull(s)) return stringNew("");
    size_t len = s->len;
    
    size_t safeStart = (start > len) ? len : start;
    size_t safeEnd = (end > len) ? len : end;
    
    if (safeStart > safeEnd) return stringNew("");

    size_t subLen = safeEnd - safeStart;
    String tmp = stringNewEmpty(subLen);
    if (stringIsNull(tmp)) return NULL;

    if (subLen > 0) {
        memcpy(tmp->data, s->data + safeStart, subLen);
    }
    tmp->data[subLen] = '\0';
    tmp->len = subLen;

    return tmp;
}

bool stringEquals(const String s1, const String s2) {
    if (s1 == s2) return true;
    if (stringIsNull(s1) || stringIsNull(s2)) return false;
    if (s1->len != s2->len) return false;
    return memcmp(s1->data, s2->data, s1->len) == 0;
}

bool stringEqualsIgnoreCase(const String s1, const String s2) {
    if (s1 == s2) return true;
    if (stringIsNull(s1) || stringIsNull(s2)) return false;
    if (s1->len != s2->len) return false;
    
    for (size_t i = 0; i < s1->len; i++) {
        if (tolower((unsigned char)s1->data[i]) != tolower((unsigned char)s2->data[i])) {
            return false;
        }
    }
    return true;
}

int stringCompare(const String s1, const String s2) {
    if (stringIsNull(s1) && stringIsNull(s2)) return 0;
    if (stringIsNull(s1)) return -1;
    if (stringIsNull(s2)) return 1;
    
    return strcmp(s1->data, s2->data);
}

int stringIndexOf(const String s, const String needle) {
    if (stringIsNull(s) || stringIsNull(needle)) return -1;
    if (needle->len == 0) return 0;
    
    char* found = strstr(s->data, needle->data);
    if (found == NULL) return -1;
    
    return (int)(found - s->data);
}

int stringIndexOfChar(const String s, char c) {
    if (stringIsNull(s)) return -1;
    char* found = strchr(s->data, c);
    if (found == NULL) return -1;
    return (int)(found - s->data);
}

int stringLastIndexOf(const String s, const String needle) {
    if (stringIsNull(s) || stringIsNull(needle)) return -1;
    if (needle->len == 0) return (int)s->len; 
    if (needle->len > s->len) return -1;
    
    for (int i = (int)(s->len - needle->len); i >= 0; i--) {
        if (memcmp(s->data + i, needle->data, needle->len) == 0) {
            return i;
        }
    }
    return -1;
}

bool stringContains(const String s, const String needle) {
    return stringIndexOf(s, needle) != -1;
}

bool stringStartsWith(const String s, const String prefix) {
    if (stringIsNull(s) || stringIsNull(prefix)) return false;
    if (prefix->len > s->len) return false;
    return memcmp(s->data, prefix->data, prefix->len) == 0;
}

bool stringEndsWith(const String s, const String suffix) {
    if (stringIsNull(s) || stringIsNull(suffix)) return false;
    if (suffix->len > s->len) return false;
    return memcmp(s->data + (s->len - suffix->len), suffix->data, suffix->len) == 0;
}

String stringTrim(const String s) {
    if (stringIsNull(s)) return NULL;
    if (s->len == 0) return stringNewEmpty(0);

    size_t start = 0;
    while (start < s->len && isspace((unsigned char)s->data[start])) {
        start++;
    }

    size_t end = s->len;
    while (end > start && isspace((unsigned char)s->data[end - 1])) {
        end--;
    }

    return stringSubstring(s, start, end);
}

String stringReplace(const String s, const String target, const String replacement) {
    if (stringIsNull(s) || stringIsNull(target) || stringIsNull(replacement)) return NULL;
    if (target->len == 0) return stringNew(s->data);

    int count = 0;
    char* p = s->data;
    while ((p = strstr(p, target->data)) != NULL) {
        count++;
        p += target->len;
    }

    if (count == 0) return stringNew(s->data);

    size_t newLen = s->len + count * (replacement->len - target->len);
    String res = stringNewEmpty(newLen);
    if (!res) return NULL;

    char* dest = res->data;
    char* src = s->data;
    char* found;

    while ((found = strstr(src, target->data)) != NULL) {
        size_t segmentLen = found - src;
        memcpy(dest, src, segmentLen);
        dest += segmentLen;
        
        memcpy(dest, replacement->data, replacement->len);
        dest += replacement->len;
        
        src = found + target->len;
    }
    
    strcpy(dest, src);
    res->len = newLen;
    
    return res;
}

String stringToUpperCase(const String s) {
    if (stringIsNull(s)) return NULL;
    String res = stringNewEmpty(s->len);
    if (!res) return NULL;

    for (size_t i = 0; i < s->len; i++) {
        res->data[i] = toupper((unsigned char)s->data[i]);
    }
    return res;
}

String stringToLowerCase(const String s) {
    if (stringIsNull(s)) return NULL;
    String res = stringNewEmpty(s->len);
    if (!res) return NULL;

    for (size_t i = 0; i < s->len; i++) {
        res->data[i] = tolower((unsigned char)s->data[i]);
    }
    return res;
}

String stringJoin(String* parts, const String delimiter) {
    if (parts == NULL) return NULL;

    size_t totalLen = 0;
    int count = 0;
    while (parts[count] != NULL) {
        totalLen += parts[count]->len;
        count++;
    }

    if (count == 0) return stringNewEmpty(0);

    size_t delimLen = (stringIsNull(delimiter)) ? 0 : delimiter->len;
    if (count > 1) {
        totalLen += (count - 1) * delimLen;
    }

    String res = stringNewEmpty(totalLen);
    if (res == NULL) return NULL;

    char* cursor = res->data;
    for (int i = 0; i < count; i++) {
        memcpy(cursor, parts[i]->data, parts[i]->len);
        cursor += parts[i]->len;

        if (i < count - 1 && delimLen > 0) {
            memcpy(cursor, delimiter->data, delimLen);
            cursor += delimLen;
        }
    }
    *cursor = '\0';
    
    return res;
}

String* stringSplit(const String s, const String regexPattern) {
    if (stringIsNull(s) || stringIsNull(regexPattern)) return NULL;

    regex_t regex;
    if (regcomp(&regex, regexPattern->data, REG_EXTENDED) != 0) {
        return NULL;
    }

    size_t tokenCount = 1;
    const char* cursor = s->data;
    regmatch_t pmatch[1];

    while (regexec(&regex, cursor, 1, pmatch, 0) == 0) {
        if (pmatch[0].rm_eo == pmatch[0].rm_so) { 
             cursor++;
        } else {
             cursor += pmatch[0].rm_eo;
        }
        tokenCount++;
    }

    String* result = (String*)xCalloc(tokenCount + 1, sizeof(String));
    if (result == NULL) {
        regfree(&regex);
        return NULL;
    }

    cursor = s->data;
    size_t idx = 0;
    size_t offset = 0;

    while (regexec(&regex, cursor, 1, pmatch, 0) == 0) {
        size_t matchStart = pmatch[0].rm_so;
        size_t matchEnd = pmatch[0].rm_eo;
        
        result[idx] = stringSubstring(s, offset, offset + matchStart);
        idx++;

        size_t advance = matchEnd;
        if (advance == 0) advance = 1;

        cursor += advance;
        offset += advance;
    }

    result[idx] = stringSubstring(s, offset, s->len);
    idx++;
    result[idx] = NULL; 

    regfree(&regex);
    return result;
}

String stringFromInt(int value) {
    char buffer[32];
    snprintf(buffer, 32, "%d", value);
    return stringNew(buffer);
}

String stringFromFloat(float value) {
    char buffer[64];
    snprintf(buffer, 64, "%f", value);
    return stringNew(buffer);
}

int stringParseInt(const String s) {
    if (stringIsNull(s)) return 0;
    return atoi(s->data);
}

double stringParseDouble(const String s) {
    if (stringIsNull(s)) return 0.0;
    return atof(s->data);
}

int stringParseHex(const String s) {
    if (stringIsNull(s)) return 0;
    return (int)strtol(s->data, NULL, 16);
}

int stringParseBinary(const String s) {
    if (stringIsNull(s)) return 0;
    return (int)strtol(s->data, NULL, 2);
}

int stringParseOctal(const String s) {
    if (stringIsNull(s)) return 0;
    return (int)strtol(s->data, NULL, 8);
}

char* stringGetData(const String s){
    return stringIsNull(s) ? NULL : s->data;
}

String stringSortChars(const String s, SortComparator cmpf) {
    if (stringIsNull(s)) return NULL;
    String res = stringNew(s->data); 
    
    if (res->len > 1) {
        if (cmpf == NULL) cmpf = SORT_CHAR_ASC;
        qsort(res->data, res->len, sizeof(char), cmpf);
    }
    return res;
}

String stringReverse(String s) {
    if (stringIsNull(s)) return NULL;
    String res = stringNewEmpty(s->len);
    
    for (size_t i = 0; i < s->len; i++) {
        res->data[i] = s->data[s->len - 1 - i];
    }
    res->data[s->len] = '\0';
    return res;
}