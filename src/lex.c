#include "lex.h"
#include <ctype.h>
#include <string.h>

void be_lex_init(Be_Lexer* l, const char* fname, const char* src) {
    l->fname = fname;
    l->src = l->curr = src;
    l->line = 1;
}

static void be_skip_space(Be_Lexer* l) {
    while (isspace((unsigned char)l->curr[0])) {
        if (l->curr[0] == '\n') l->line++;
        l->curr++;
    }
}

static Be_Token be_token(Be_Lexer* l, Be_Token_Kind kind, int len) {
    Be_Token tok = {.kind = kind, .str = l->curr, .len = len, .line = l->line};
    l->curr += len;
    return tok;
}

static Be_Token be_name(Be_Lexer* l, Be_Token_Kind kind) {
    int len = 1;
    while (isalnum((unsigned char)l->curr[len])) len++;
    return be_token(l, kind, len);
}

static bool be_is_keyword(const Be_Token* tok, const char* keyword, int len) {
    return tok->len == len && strncmp(tok->str, keyword, len) == 0;
}

static Be_Token_Kind be_ident_kind(const Be_Token* tok) {
    switch (tok->str[0]) {
        case 'f':
            return be_is_keyword(tok, "func", 4) ? BE_TOK_FUNC : BE_TOK_IDENT;
        case 'i':
            return be_is_keyword(tok, "i", 1) ? BE_TOK_I : BE_TOK_IDENT;
        case 'r':
            return be_is_keyword(tok, "ret", 3) ? BE_TOK_RET : BE_TOK_IDENT;
        default:
            return BE_TOK_IDENT;
    }
}

static Be_Token be_ident(Be_Lexer* l) {
    Be_Token tok = be_name(l, BE_TOK_IDENT);
    tok.kind = be_ident_kind(&tok);
    return tok;
}

static Be_Token be_int(Be_Lexer* l) {
    int len = 1;
    while (isdigit((unsigned char)l->curr[len])) len++;
    return be_token(l, BE_TOK_INT, len);
}

Be_Token be_lex(Be_Lexer* l) {
    be_skip_space(l);
    switch (l->curr[0]) {
        case '\0':
            return be_token(l, BE_TOK_EOF, 0);
        case '%':
            return be_name(l, BE_TOK_LOCAL);
        case '@':
            return be_name(l, BE_TOK_GLOBAL);
        case '(':
            return be_token(l, BE_TOK_LPAREN, 1);
        case ')':
            return be_token(l, BE_TOK_RPAREN, 1);
        case '{':
            return be_token(l, BE_TOK_LBRACE, 1);
        case '}':
            return be_token(l, BE_TOK_RBRACE, 1);
        case ':':
            return be_token(l, BE_TOK_COLON, 1);
        case '=':
            return be_token(l, BE_TOK_EQSIGN, 1);
        default:
            break;
    }
    if (isalpha((unsigned char)l->curr[0])) return be_ident(l);
    if (isdigit((unsigned char)l->curr[0])) return be_int(l);
    return be_token(l, BE_TOK_ERROR, 1);
}
