#ifndef parser_h
#define parser_h

KNOWN_AS(word_extractor, PARSER);

struct word_extractor {
    LPCSTR buffer;
    const char* delimiters;
    BOOL error;
    BOOL eat_quotes;
};

LPCSTR parse_token(LPPARSER p);
LPCSTR parse_segment(LPPARSER p);
LPCSTR peek_token(LPPARSER p);
BOOL eat_token(LPPARSER p, LPCSTR value);
void parser_error(LPPARSER parser) ;

#endif
