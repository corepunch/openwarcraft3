#ifndef parser_h
#define parser_h

KNOWN_AS(word_extractor, PARSER);

typedef enum {
    NO_BOM,
    UTF8_BOM_FOUND,
    UTF16LE_BOM_FOUND,
    UTF16BE_BOM_FOUND,
    INVALID_BOM
} BOMStatus;

struct word_extractor {
    LPCSTR buffer;
    const char* delimiters;
    BOOL error;
    BOOL eat_quotes;
};

LPCSTR parse_token(LPPARSER p);
LPCSTR parse_segment(LPPARSER p);
LPCSTR peek_token(LPPARSER p);
void remove_comments(LPSTR buffer);
BOMStatus remove_bom(char buffer[], size_t length);
void parser_error(LPPARSER parser) ;

#endif
