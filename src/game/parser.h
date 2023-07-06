#ifndef parser_h
#define parser_h

typedef enum {
    NO_BOM,
    UTF8_BOM_FOUND,
    UTF16LE_BOM_FOUND,
    UTF16BE_BOM_FOUND,
    INVALID_BOM
} BOMStatus;

typedef struct {
    LPCSTR buffer;
    const char* delimiters;
    bool error;
} WordExtractor;

LPCSTR getFirstWord(WordExtractor* p);
LPCSTR getNextSegment(WordExtractor* p);
void removeComments(char *buffer);
BOMStatus removeBOM(char buffer[], size_t length);

#endif
