#include "g_local.h"

#define MAX_SEGMENT_SIZE 1024

LPCSTR getFirstWord(WordExtractor* p) {
    static char word[MAX_SEGMENT_SIZE];
    while (isspace(*p->buffer)) ++p->buffer;
    if (*p->buffer == '\"') {
        LPCSTR closingQuote = strchr(++p->buffer, '"');
        size_t stringLength = closingQuote - p->buffer;
        memcpy(word, p->buffer, stringLength);
        word[stringLength] = '\0';
        p->buffer = ++closingQuote;
        return word;
    } else if (strchr(p->delimiters, *p->buffer)) {
        word[0] = *(p->buffer++);
        word[1] = '\0';
        return word;
    } else {
        size_t segmentLength = 0;
        while (*p->buffer &&
           (!isspace(*p->buffer) && strchr(p->delimiters, *p->buffer) == NULL) &&
               segmentLength < MAX_SEGMENT_SIZE - 1) {
            word[segmentLength++] = *(p->buffer++);
        }
        word[segmentLength] = '\0'; // Null-terminate the segment
        return word;
    }
}

LPCSTR getNextSegment(WordExtractor* p) {
    static char segment[MAX_SEGMENT_SIZE];
    if (*p->buffer == '\0') {
        return NULL;
    }
    while (isspace(*p->buffer)) {
        ++p->buffer;
    }
    LPCSTR start = p->buffer;
    if (*p->buffer == '\"') {
        ++start;
        p->buffer = strchr(start, '\"');
        memcpy(segment, start, p->buffer - start);
        segment[p->buffer - start] = '\0';
        p->buffer = strchr(p->buffer, ',');
    } else {
        p->buffer = strchr(p->buffer, ',');
        if (p->buffer) {
            memcpy(segment, start, p->buffer - start);
            segment[p->buffer - start] = '\0'; // Null-terminate the segment
        } else {
            strcpy(segment, start);
            p->buffer = start + strlen(start);
            return segment;
        }
    }
    ++p->buffer;
    return segment;
}

void removeComments(char *buffer) {
    BOOL in_single_line_comment = false;
    BOOL in_block_comment = false;
    char *src = buffer;
    char *dest = buffer;
    while (*src != '\0') {
        if (!in_single_line_comment && !in_block_comment) {
            if (*src == '/' && *(src + 1) == '/') {
                in_single_line_comment = true;
                src += 2;
            } else if (*src == '/' && *(src + 1) == '*') {
                in_block_comment = true;
                src += 2;
            } else {
                *dest++ = *src++;
            }
        } else if (in_single_line_comment && *src == '\n') {
            in_single_line_comment = false;
            *dest++ = *src++;
        } else if (in_block_comment && *src == '*' && *(src + 1) == '/') {
            in_block_comment = false;
            src += 2;
        } else {
            src++;
        }
    }
    *dest = '\0';  // Null-terminate the modified buffer
}

BOMStatus removeBOM(char buffer[], size_t length) {
    unsigned char utf8BOM[] = { 0xEF, 0xBB, 0xBF };
    unsigned char utf16LEBOM[] = { 0xFF, 0xFE };
    unsigned char utf16BEBOM[] = { 0xFE, 0xFF };

    if (length >= 3 && memcmp(buffer, utf8BOM, 3) == 0) {
        memmove(buffer, buffer + 3, length - 3);
        return UTF8_BOM_FOUND;
    } else if (length >= 2 && memcmp(buffer, utf16LEBOM, 2) == 0) {
        memmove(buffer, buffer + 2, length - 2);
        return UTF16LE_BOM_FOUND;
    } else if (length >= 2 && memcmp(buffer, utf16BEBOM, 2) == 0) {
        memmove(buffer, buffer + 2, length - 2);
        return UTF16BE_BOM_FOUND;
    } else {
        return NO_BOM;
    }
}

