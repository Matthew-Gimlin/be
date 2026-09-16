// Defines the lexer.

#ifndef BE_LEX_H
#define BE_LEX_H

typedef enum {
    BE_TOK_ERROR,
    BE_TOK_EOF,     // End of file
    BE_TOK_IDENT,   // Generic identifier
    BE_TOK_LOCAL,   // Local identifier, e.g. `%name`
    BE_TOK_GLOBAL,  // Global identifer, e.g. `@name`
    BE_TOK_INT,     // Decimal integer, e.g. `1234`
    BE_TOK_FUNC,    // `func` keyword
    BE_TOK_RET,     // `ret` keyword
    BE_TOK_I,       // `i` keyword
    BE_TOK_LPAREN,  // `(` symbol
    BE_TOK_RPAREN,  // `)` symbol
    BE_TOK_LBRACE,  // `{` symbol
    BE_TOK_RBRACE,  // `}` symbol
    BE_TOK_COLON,   // `:` symbol
    BE_TOK_EQSIGN,  // `=` symbol
} Be_Token_Kind;

typedef struct {
    Be_Token_Kind kind;
    const char* str;
    int len;
    int line;
} Be_Token;

typedef struct {
    const char* fname;
    const char* src;
    const char* curr;
    int line;
} Be_Lexer;

void be_lex_init(Be_Lexer* l, const char* fname, const char* src);

// Gets the next token. Advances `l` each time the function is called.
Be_Token be_lex(Be_Lexer* l);

#endif
