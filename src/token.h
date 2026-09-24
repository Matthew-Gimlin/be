#ifndef BE_TOKEN_H
#define BE_TOKEN_H

#include "be.h"

typedef enum {
    BE_TOKEN_ERROR,
    BE_TOKEN_EOF,       // End of file
    BE_TOKEN_LABEL,     // Block label
    BE_TOKEN_LOCAL,     // Local identifier, e.g. `%name`
    BE_TOKEN_GLOBAL,    // Global identifer, e.g. `@name`
    BE_TOKEN_INT,       // Decimal integer, e.g. `1234`
    BE_TOKEN_FUNC,      // `func` keyword
    BE_TOKEN_RET,       // `ret` keyword
    BE_TOKEN_I,         // `i` keyword
    BE_TOKEN_LPAREN,    // `(` symbol
    BE_TOKEN_RPAREN,    // `)` symbol
    BE_TOKEN_LBRACE,    // `{` symbol
    BE_TOKEN_RBRACE,    // `}` symbol
    BE_TOKEN_COLON,     // `:` symbol
    BE_TOKEN_EQSIGN,    // `=` symbol
} Be_Token_Kind;

typedef struct {
    Be_Token_Kind kind;
    Be_String symbol;
    int line;
} Be_Token;

#endif
