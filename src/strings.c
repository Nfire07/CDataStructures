#include "../include/strings.h"
#include "../include/arrays.h"
#include "../include/pointers.h"
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <regex.h>

extern void *memcpy(void *dest, const void *src, size_t n);

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
    
    size_t len = 0;
    while (s[len] != '\0') {
        len++;
    }

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
    if (stringIsNull(s)) return 0;
    int len = (int)s->len;
    if (index >= len || index < -len) return 0;
    if (index >= 0) return (unsigned char)s->data[index];
    return (unsigned char)s->data[len + index];
}

String stringFromChar(const char c) {
    char buf[2] = {c, '\0'};
    return stringNew(buf);
}

String stringSubstring(const String s, const size_t start, const size_t end) {
    if (stringIsNull(s)) return stringNew("");
    size_t len = s->len;
    if (start > end) return stringNew("");
    
    size_t safeStart = (start > len) ? len : start;
    size_t safeEnd = (end > len) ? len : end;

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
    
    for (size_t i = 0; i < s1->len; i++) {
        if (s1->data[i] != s2->data[i]) return false;
    }
    return true;
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
    
    size_t minLen = (s1->len < s2->len) ? s1->len : s2->len;
    for (size_t i = 0; i < minLen; i++) {
        unsigned char c1 = (unsigned char)s1->data[i];
        unsigned char c2 = (unsigned char)s2->data[i];
        if (c1 != c2) {
            return (c1 < c2) ? -1 : 1;
        }
    }
    
    if (s1->len == s2->len) return 0;
    return (s1->len < s2->len) ? -1 : 1;
}

int stringIndexOf(const String s, const String needle) {
    if (stringIsNull(s) || stringIsNull(needle)) return -1;
    if (needle->len == 0) return 0;
    if (needle->len > s->len) return -1;
    
    for (size_t i = 0; i <= s->len - needle->len; i++) {
        bool match = true;
        for (size_t j = 0; j < needle->len; j++) {
            if (s->data[i + j] != needle->data[j]) {
                match = false;
                break;
            }
        }
        if (match) return (int)i;
    }
    return -1;
}

int stringIndexOfChar(const String s, char c) {
    if (stringIsNull(s)) return -1;
    for (size_t i = 0; i < s->len; i++) {
        if (s->data[i] == c) return (int)i;
    }
    return -1;
}

int stringLastIndexOf(const String s, const String needle) {
    if (stringIsNull(s) || stringIsNull(needle)) return -1;
    if (needle->len == 0) return (int)s->len; 
    if (needle->len > s->len) return -1;
    
    for (int i = (int)(s->len - needle->len); i >= 0; i--) {
        bool match = true;
        for (size_t j = 0; j < needle->len; j++) {
            if (s->data[i + j] != needle->data[j]) {
                match = false;
                break;
            }
        }
        if (match) return i;
    }
    return -1;
}

bool stringContains(const String s, const String needle) {
    return stringIndexOf(s, needle) != -1;
}

bool stringStartsWith(const String s, const String prefix) {
    if (stringIsNull(s) || stringIsNull(prefix)) return false;
    if (prefix->len > s->len) return false;
    
    for (size_t i = 0; i < prefix->len; i++) {
        if (s->data[i] != prefix->data[i]) return false;
    }
    return true;
}

bool stringEndsWith(const String s, const String suffix) {
    if (stringIsNull(s) || stringIsNull(suffix)) return false;
    if (suffix->len > s->len) return false;
    
    size_t offset = s->len - suffix->len;
    for (size_t i = 0; i < suffix->len; i++) {
        if (s->data[offset + i] != suffix->data[i]) return false;
    }
    return true;
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
    size_t pos = 0;
    while (pos <= s->len - target->len) {
        bool match = true;
        for (size_t j = 0; j < target->len; j++) {
            if (s->data[pos + j] != target->data[j]) {
                match = false;
                break;
            }
        }
        
        if (match) {
            count++;
            pos += target->len;
        } else {
            pos++;
        }
    }

    if (count == 0) return stringNew(s->data);

    size_t newLen = s->len + count * (replacement->len - target->len);
    String res = stringNewEmpty(newLen);
    if (!res) return NULL;

    size_t srcIdx = 0;
    size_t dstIdx = 0;
    
    while (srcIdx < s->len) {
        if (srcIdx > s->len - target->len) {
            res->data[dstIdx++] = s->data[srcIdx++];
            continue;
        }

        bool match = true;
        for (size_t j = 0; j < target->len; j++) {
            if (s->data[srcIdx + j] != target->data[j]) {
                match = false;
                break;
            }
        }

        if (match) {
            memcpy(res->data + dstIdx, replacement->data, replacement->len);
            dstIdx += replacement->len;
            srcIdx += target->len;
        } else {
            res->data[dstIdx++] = s->data[srcIdx++];
        }
    }
    
    res->data[newLen] = '\0';
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

    size_t delimLen = (delimiter != NULL) ? delimiter->len : 0;
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

    size_t token_count = 1; 
    char* cursor = s->data;
    regmatch_t pmatch[1];

    while (regexec(&regex, cursor, 1, pmatch, 0) == 0) {
        size_t matchLen = pmatch[0].rm_eo - pmatch[0].rm_so;
        cursor += pmatch[0].rm_eo;
        if (matchLen == 0) cursor++;
        
        token_count++;
        if (*cursor == '\0') break;
    }

    String* result = (String*)xMalloc(sizeof(String) * (token_count + 1));
    if (result == NULL) {
        regfree(&regex);
        return NULL;
    }

    size_t current_idx = 0;
    size_t string_offset = 0;
    cursor = s->data;
    
    while (regexec(&regex, cursor, 1, pmatch, 0) == 0) {
        size_t match_start_abs = string_offset + pmatch[0].rm_so;

        result[current_idx] = stringSubstring(s, string_offset, match_start_abs);
        current_idx++;

        size_t advance = pmatch[0].rm_eo;
        if (advance == 0) advance = 1; 

        cursor += advance;
        string_offset += advance;
    }

    result[current_idx] = stringSubstring(s, string_offset, s->len);
    current_idx++;
    result[current_idx] = NULL;

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

char* stringGetData(const String s){
    return s->data;
}

String stringSortChars(const String s, SortComparator cmpf) {
    if (stringIsNull(s)) return NULL;
    if (s->len == 0) return stringNewEmpty(0);

    String res = stringNewEmpty(s->len);
    if (stringIsNull(res)) return NULL;

    memcpy(res->data, s->data, s->len);
    res->data[s->len] = '\0';

    if (cmpf == NULL) {
        cmpf = SORT_CHAR_ASC;
    }

    if (s->len > 1) {
        qsort(res->data, s->len, sizeof(char), cmpf);
    }

    return res;
}

String stringReverse(const String s) {
    if (null(s)) return NULL;
    if (s->len == 0) return stringNewEmpty(0);

    String res = stringNewEmpty(s->len);
    if (null(res)) return NULL;

    for (size_t i = 0; i < s->len; i++) {
        res->data[i] = s->data[s->len - 1 - i];
    }
    
    res->data[s->len] = '\0';
    
    return res;
}