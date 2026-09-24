#include "be.h"
#include <ctype.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

typedef enum {
    BE_TOKEN_ERROR,
    BE_TOKEN_EOF,       // End of file
    BE_TOKEN_LABEL,     // Block label
    BE_TOKEN_LOCAL,     // Local identifier, e.g. `%name`
    BE_TOKEN_GLOBAL,    // Global identifer, e.g. `@name`
    BE_TOKEN_INT,       // Decimal integer, e.g. `1234`
    BE_TOKEN_ADD,       // `add` keyword
    BE_TOKEN_FUNC,      // `func` keyword
    BE_TOKEN_RET,       // `ret` keyword
    BE_TOKEN_I,         // `i` keyword
    BE_TOKEN_LPAREN,    // `(` symbol
    BE_TOKEN_RPAREN,    // `)` symbol
    BE_TOKEN_LBRACE,    // `{` symbol
    BE_TOKEN_RBRACE,    // `}` symbol
    BE_TOKEN_COLON,     // `:` symbol
    BE_TOKEN_EQSIGN,    // `=` symbol
    BE_TOKEN_COMMA,     // `,` symbol
} Be_Token_Kind;

typedef struct {
    Be_Token_Kind kind;
    Be_String symbol;
    int line;
} Be_Token;

typedef struct {
    Be_Arena* const arena;
    const char* const filename;
    const char* const source;
    const char* position;
    int line;
    Be_Token token;
} Be_Parser;

static void be_skip_space(Be_Parser* parser) {
    while (isspace((unsigned char)parser->position[0])) {
        if (parser->position[0] == '\n') parser->line++;
        parser->position++;
    }
}

static void be_skip_comment(Be_Parser* parser) {
    parser->position++;
    while (parser->position[0] && parser->position[0] != '\n') parser->position++;
}

static void be_skip_junk(Be_Parser* parser) {
    do {
        be_skip_space(parser);
        if (parser->position[0] == ';') be_skip_comment(parser);
    } while (isspace((unsigned char)parser->position[0]));
}

static Be_Token be_token(Be_Parser* parser, Be_Token_Kind kind, int len) {
    parser->token = (Be_Token){
        .kind = kind,
        .symbol = (Be_String){ .str = parser->position, .len = len },
        .line = parser->line,
    };
    parser->position += len;
    return parser->token;
}

static Be_Token be_name(Be_Parser* parser, Be_Token_Kind kind) {
    int len = 1;
    while (isalnum((unsigned char)parser->position[len])) len++;
    return be_token(parser, kind, len);
}

static inline bool be_is_keyword(Be_Parser* parser, int len, const char* keyword, int keyword_len) {
    return len == keyword_len && strncmp(parser->position, keyword, len) == 0;
}

static Be_Token_Kind be_keyword_kind(Be_Parser* parser, int len) {
    switch (parser->position[0]) {
        case 'a':
            return be_is_keyword(parser, len, "add", 3) ? BE_TOKEN_ADD : BE_TOKEN_LABEL;
        case 'f':
            return be_is_keyword(parser, len, "func", 4) ? BE_TOKEN_FUNC : BE_TOKEN_LABEL;
        case 'i':
            return be_is_keyword(parser, len, "i", 1) ? BE_TOKEN_I: BE_TOKEN_LABEL;
        case 'r':
            return be_is_keyword(parser, len, "ret", 3) ? BE_TOKEN_RET: BE_TOKEN_LABEL;
        default:
            return BE_TOKEN_LABEL;
    }
}

static Be_Token be_keyword(Be_Parser* parser) {
    int len = 1;
    while (isalnum((unsigned char)parser->position[len])) len++;
    Be_Token_Kind kind = be_keyword_kind(parser, len);
    return be_token(parser, kind, len);
}

static Be_Token be_number(Be_Parser* parser) {
    int len = 1;
    while (isdigit((unsigned char)parser->position[len])) len++;
    return be_token(parser, BE_TOKEN_INT, len);
}

static Be_Token be_next_token(Be_Parser* parser) {
    be_skip_junk(parser);
    switch (parser->position[0]) {
        case '\0':
            return be_token(parser, BE_TOKEN_EOF, 0);
        case '%':
            return be_name(parser, BE_TOKEN_LOCAL);
        case '@':
            return be_name(parser, BE_TOKEN_GLOBAL);
        case '(':
            return be_token(parser, BE_TOKEN_LPAREN, 1);
        case ')':
            return be_token(parser, BE_TOKEN_RPAREN, 1);
        case '{':
            return be_token(parser, BE_TOKEN_LBRACE, 1);
        case '}':
            return be_token(parser, BE_TOKEN_RBRACE, 1);
        case ':':
            return be_token(parser, BE_TOKEN_COLON, 1);
        case '=':
            return be_token(parser, BE_TOKEN_EQSIGN, 1);
        case ',':
            return be_token(parser, BE_TOKEN_COMMA, 1);
        default:
            break;
    }
    if (isalpha((unsigned char)parser->position[0])) return be_keyword(parser);
    if (isdigit((unsigned char)parser->position[0])) return be_number(parser);
    return be_token(parser, BE_TOKEN_ERROR, 1);
}

static bool be_expect(Be_Parser* parser, Be_Token_Kind kind, const char* msg) {
    if (parser->token.kind == kind) {
        be_next_token(parser);
        return true;
    }
    be_error(parser->filename, parser->line, "%s but got `%.*s`", msg, parser->token.symbol.len, parser->token.symbol.str);
    return false;
}

static Be_Type be_parse_type(Be_Parser* parser) {
    switch (parser->token.kind) {
        case BE_TOKEN_I:
            be_next_token(parser);
            return BE_TYPE_INT;
        default:
            be_error(parser->filename, parser->line, "expected type");
            return BE_TYPE_ERROR;
    }
}

static int be_push_value(Be_Parser* parser, Be_Function* function, Be_String name) {
    Be_Value* value;
    for (int i = 0; i < function->values.size; i++) {
        value = function->values.elements[i];
        if (!value) continue;
        if (value->name.len != name.len) continue;
        if (strncmp(value->name.str, name.str, name.len) == 0) continue;
        return i;
    }
    value = be_arena_alloc(parser->arena, sizeof(Be_Value), alignof(Be_Value));
    value->name = name;
    value->id = be_vector_push(&function->values, value);
    return value->id;
}

static bool be_parse_operand(Be_Parser* parser, Be_Function* function, Be_Operand* operand) {
    char* endptr;
    switch (parser->token.kind) {
        case BE_TOKEN_LOCAL:
            operand->kind = BE_OPERAND_VALUE;
            operand->value_id = be_push_value(parser, function, parser->token.symbol);
            break;
        case BE_TOKEN_INT:
            operand->kind = BE_OPERAND_INT;
            operand->int_const = strtol(parser->token.symbol.str, &endptr, 10);
            if (endptr != parser->position) return NULL;
            break;
        default:
            return false;
    }
    be_next_token(parser);
    return true;
}

static Be_Instruction* be_parse_instruction(Be_Parser* parser, Be_Function* function) {
    Be_Instruction* instruction = be_arena_alloc(parser->arena, sizeof(Be_Instruction), alignof(Be_Instruction));
    if (parser->token.kind == BE_TOKEN_LOCAL) {
        instruction->result_id = be_push_value(parser, function, parser->token.symbol);
        be_next_token(parser);
        if (!be_expect(parser, BE_TOKEN_EQSIGN, "expected `=`")) return NULL;
        if (!(instruction->type = be_parse_type(parser))) return NULL;
    } else {
        instruction->type = BE_TYPE_VOID;
        instruction->result_id = -1;
    }
    switch (parser->token.kind) {
        case BE_TOKEN_INT:
            instruction->kind = BE_INSTRUCTION_CONST;
            if (!be_parse_operand(parser, function, &instruction->constant.operand)) return NULL;
            break;
        case BE_TOKEN_ADD:
            instruction->kind = BE_INSTRUCTION_ADD;
            be_next_token(parser);
            if (!be_parse_operand(parser, function, &instruction->binary.lhs)) return NULL;
            if (!be_expect(parser, BE_TOKEN_COMMA, "expected `,`")) return NULL;
            if (!be_parse_operand(parser, function, &instruction->binary.rhs)) return NULL;
            break;
        default:
            be_error(parser->filename, parser->line, "expected instruction");
            return NULL;
    }
    return instruction;
}

static bool be_at_terminator(const Be_Parser* parser) {
    switch (parser->token.kind) {
        case BE_TOKEN_RET: return true;
        default: return false;
    }
}

static bool be_parse_terminator(Be_Parser* parser, Be_Function* function, Be_Block* block) {
    switch (parser->token.kind) {
        case BE_TOKEN_RET:
            block->terminator.kind = BE_TERMINATOR_RET;
            be_next_token(parser);
            if (!(block->terminator.ret.type = be_parse_type(parser))) return false;
            if (!be_parse_operand(parser, function, &block->terminator.ret.operand)) return false;
            break;
        default:
            be_error(parser->filename, parser->line, "expected terminator");
            return false;
    }
    return true;
}

static Be_Block* be_parse_block(Be_Parser* parser, Be_Function* function) {
    Be_Block* block = be_arena_alloc(parser->arena, sizeof(Be_Block), alignof(Be_Block));
    be_init_vector(&block->instructions, parser->arena);
    block->label = parser->token.symbol;
    if (!be_expect(parser, BE_TOKEN_LABEL, "expected label")) return NULL;
    if (!be_expect(parser, BE_TOKEN_COLON, "expected `:`")) return NULL;
    while (!be_at_terminator(parser)) {
        Be_Instruction* instruction = be_parse_instruction(parser, function);
        if (!instruction) return NULL;
        be_vector_push(&block->instructions, instruction);
    }
    if (!be_parse_terminator(parser, function, block)) return NULL;
    return block;
}

static Be_Function* be_parse_function(Be_Parser* parser) {
    Be_Function* function = be_arena_alloc(parser->arena, sizeof(Be_Function), alignof(Be_Function));
    be_init_vector(&function->parameters, parser->arena);
    be_init_vector(&function->values, parser->arena);
    be_init_vector(&function->blocks, parser->arena);

    if (!(function->type = be_parse_type(parser))) return NULL;
    function->name = parser->token.symbol;
    if (!be_expect(parser, BE_TOKEN_GLOBAL, "expected global name")) return NULL;

    // TODO: Parse function parameters...
    if (!be_expect(parser, BE_TOKEN_LPAREN, "expected `(`")) return NULL;
    if (!be_expect(parser, BE_TOKEN_RPAREN, "expected `)`")) return NULL;

    if (!be_expect(parser, BE_TOKEN_LBRACE, "expected `{`")) return NULL;
    while (parser->token.kind != BE_TOKEN_RBRACE) {
        Be_Block* block = be_parse_block(parser, function);
        if (!block) return NULL;
        block->id = be_vector_push(&function->blocks, block);
    }
    return function;
}

Be_Module* be_parse(Be_Arena* arena, const char* filename, const char* source) {
    Be_Parser parser = {
        .arena = arena,
        .filename = filename,
        .source = source,
        .position = source,
        .line = 1,
    };
    be_next_token(&parser);
    Be_Module* module = be_arena_alloc(parser.arena, sizeof(Be_Module), alignof(Be_Module));
    be_init_vector(&module->functions, parser.arena);
    while (parser.token.kind != BE_TOKEN_EOF) {
        if (!be_expect(&parser, BE_TOKEN_FUNC, "expected `func`")) return NULL;
        Be_Function* function = be_parse_function(&parser);
        if (!function) return NULL;
        be_vector_push(&module->functions, function);
        be_next_token(&parser);
    }
    return module;
}
