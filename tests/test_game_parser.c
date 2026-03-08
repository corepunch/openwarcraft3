/*
 * test_game_parser.c — Unit tests for the game/parser.c tokenizer.
 *
 * Tests: parse_token, peek_token, eat_token, parse_segment,
 *        parse_segment2, parser_error, find_in_array.
 *
 * Build: see Makefile `test` target.
 */
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include "../src/common/shared.h"
#include "../src/game/parser.h"
#include "test_harness.h"

/* Bring in the parser implementation (compiled directly so we don't
 * need the full libgame.so link). The g_local.h include inside
 * parser.c is satisfied by tests/stubs/g_local.h via -Itests/stubs. */
#include "../src/game/parser.c"

/* ------------------------------------------------------------------ */
/* Helpers                                                              */
/* ------------------------------------------------------------------ */
static PARSER make_parser(const char *buf, const char *delims) {
    PARSER p;
    p.buffer    = buf;
    p.delimiters = delims;
    p.error     = false;
    p.eat_quotes = false;
    return p;
}

/* ------------------------------------------------------------------ */
/* parse_token                                                          */
/* ------------------------------------------------------------------ */
static void test_parse_token_simple_word(void) {
    PARSER p = make_parser("hello world", "");
    ASSERT_STR_EQ(parse_token(&p), "hello");
    ASSERT_STR_EQ(parse_token(&p), "world");
}

static void test_parse_token_leading_whitespace(void) {
    PARSER p = make_parser("   foo", "");
    ASSERT_STR_EQ(parse_token(&p), "foo");
}

static void test_parse_token_quoted_string_keep_quotes(void) {
    PARSER p = make_parser("\"hello world\"", "");
    p.eat_quotes = false;
    LPCSTR tok = parse_token(&p);
    ASSERT_STR_EQ(tok, "\"hello world\"");
}

static void test_parse_token_quoted_string_eat_quotes(void) {
    PARSER p = make_parser("\"hello world\"", "");
    p.eat_quotes = true;
    LPCSTR tok = parse_token(&p);
    ASSERT_STR_EQ(tok, "hello world");
}

static void test_parse_token_delimiter(void) {
    PARSER p = make_parser("{key}", "{}");
    ASSERT_STR_EQ(parse_token(&p), "{");
    ASSERT_STR_EQ(parse_token(&p), "key");
    ASSERT_STR_EQ(parse_token(&p), "}");
}

static void test_parse_token_multiple_spaces(void) {
    PARSER p = make_parser("a  b  c", "");
    ASSERT_STR_EQ(parse_token(&p), "a");
    ASSERT_STR_EQ(parse_token(&p), "b");
    ASSERT_STR_EQ(parse_token(&p), "c");
}

/* ------------------------------------------------------------------ */
/* peek_token                                                           */
/* ------------------------------------------------------------------ */
static void test_peek_token_does_not_advance(void) {
    PARSER p = make_parser("first second", "");
    LPCSTR first_peek  = peek_token(&p);
    LPCSTR second_peek = peek_token(&p);
    ASSERT_STR_EQ(first_peek, "first");
    ASSERT_STR_EQ(second_peek, "first");
    /* Consuming via parse_token should still yield "first" */
    ASSERT_STR_EQ(parse_token(&p), "first");
}

/* ------------------------------------------------------------------ */
/* eat_token                                                            */
/* ------------------------------------------------------------------ */
static void test_eat_token_match(void) {
    PARSER p = make_parser("keyword rest", "");
    ASSERT(eat_token(&p, "keyword") == true);
    ASSERT_STR_EQ(parse_token(&p), "rest");
}

static void test_eat_token_no_match(void) {
    PARSER p = make_parser("other rest", "");
    ASSERT(eat_token(&p, "keyword") == false);
    /* Buffer must not have advanced on a failed eat */
    ASSERT_STR_EQ(parse_token(&p), "other");
}

/* ------------------------------------------------------------------ */
/* parse_segment                                                        */
/* ------------------------------------------------------------------ */
static void test_parse_segment_csv(void) {
    PARSER p = make_parser("alpha,beta,gamma", ",");
    ASSERT_STR_EQ(parse_segment(&p), "alpha");
    ASSERT_STR_EQ(parse_segment(&p), "beta");
    ASSERT_STR_EQ(parse_segment(&p), "gamma");
    ASSERT(parse_segment(&p) == NULL);
}

static void test_parse_segment_quoted(void) {
    PARSER p = make_parser("\"hello world\",next", ",");
    ASSERT_STR_EQ(parse_segment(&p), "hello world");
    ASSERT_STR_EQ(parse_segment(&p), "next");
}

static void test_parse_segment_empty_input(void) {
    PARSER p = make_parser("", ",");
    ASSERT(parse_segment(&p) == NULL);
}

/* ------------------------------------------------------------------ */
/* parse_segment2                                                       */
/* ------------------------------------------------------------------ */
static void test_parse_segment2_csv(void) {
    PARSER p = make_parser("one,two,three", ",");
    ASSERT_STR_EQ(parse_segment2(&p), "one");
    ASSERT_STR_EQ(parse_segment2(&p), "two");
    ASSERT_STR_EQ(parse_segment2(&p), "three");
    ASSERT(parse_segment2(&p) == NULL);
}

/* ------------------------------------------------------------------ */
/* parser_error                                                         */
/* ------------------------------------------------------------------ */
static void test_parser_error_sets_flag(void) {
    PARSER p = make_parser("data", "");
    ASSERT(p.error == false);
    parser_error(&p);
    ASSERT(p.error == true);
}

/* ------------------------------------------------------------------ */
/* find_in_array                                                        */
/* ------------------------------------------------------------------ */
static void test_find_in_array(void) {
    /* Array of LPCSTR entries, terminated by a NULL pointer. */
    const char *table[] = { "alpha", "beta", "gamma", NULL };
    const char **result = (const char **)find_in_array(
        (void *)table, sizeof(const char *), "beta");
    ASSERT(result != NULL);
    ASSERT_STR_EQ(*result, "beta");

    const char **missing = (const char **)find_in_array(
        (void *)table, sizeof(const char *), "delta");
    ASSERT(missing == NULL);
}

/* ------------------------------------------------------------------ */
/* main                                                                 */
/* ------------------------------------------------------------------ */
int main(void) {
    test_parse_token_simple_word();
    test_parse_token_leading_whitespace();
    test_parse_token_quoted_string_keep_quotes();
    test_parse_token_quoted_string_eat_quotes();
    test_parse_token_delimiter();
    test_parse_token_multiple_spaces();

    test_peek_token_does_not_advance();

    test_eat_token_match();
    test_eat_token_no_match();

    test_parse_segment_csv();
    test_parse_segment_quoted();
    test_parse_segment_empty_input();

    test_parse_segment2_csv();

    test_parser_error_sets_flag();

    test_find_in_array();

    TEST_SUMMARY();
    return TEST_RESULT();
}
