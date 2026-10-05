#include "be.h"
#include <setjmp.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdlib.h>

typedef enum {
    BE_TOKEN_ERROR,
    BE_TOKEN_EOF,       // End of file
    BE_TOKEN_LABEL,     // Block label
    BE_TOKEN_LOCAL,     // Local identifier, e.g. `%name`
    BE_TOKEN_GLOBAL,    // Global identifer, e.g. `@name`
    BE_TOKEN_INT,       // Decimal integer, e.g. `1234`
    BE_TOKEN_ADD,       // `add` keyword
    BE_TOKEN_BR,        // `br` keyword
    BE_TOKEN_CBR,       // `cbr` keyword
    BE_TOKEN_EQ,        // `eq` keyword
    BE_TOKEN_FUNC,      // `func` keyword
    BE_TOKEN_RET,       // `ret` keyword
    BE_TOKEN_B,         // `b` keyword
    BE_TOKEN_S,         // `s` keyword
    BE_TOKEN_I,         // `i` keyword
    BE_TOKEN_L,         // `l` keyword
    BE_TOKEN_LPAREN,    // `(` symbol
    BE_TOKEN_RPAREN,    // `)` symbol
    BE_TOKEN_LBRACE,    // `{` symbol
    BE_TOKEN_RBRACE,    // `}` symbol
    BE_TOKEN_COLON,     // `:` symbol
    BE_TOKEN_EQSIGN,    // `=` symbol
    BE_TOKEN_COMMA,     // `,` symbol
} Be_Token;

typedef struct {
    jmp_buf env;
    Be_Arena* const arena;
    const char* const filename;
    const char* const source;
    const char* position;
    Be_Token token;
    Be_String symbol;
    int line;
    int column;
} Be_Parser;

static void be_skip_space(Be_Parser* parser) {
    while (isspace((unsigned char)parser->position[0])) {
        if (parser->position[0] == '\n') {
            parser->line++;
            parser->column = 1;
        } else parser->column++;
        parser->position++;
    }
}

static void be_skip_comment(Be_Parser* parser) {
    do parser->position++; while (parser->position[0] && parser->position[0] != '\n');
}

static void be_skip_junk(Be_Parser* parser) {
    do {
        be_skip_space(parser);
        if (parser->position[0] == ';') be_skip_comment(parser);
    } while (isspace((unsigned char)parser->position[0]));
}

static Be_Token be_token(Be_Parser* parser, Be_Token token, int len) {
    parser->token = token;
    parser->symbol = (Be_String){ .str = parser->position, .len = len };
    parser->position += len;
    return token;
}

static Be_Token be_name(Be_Parser* parser, Be_Token token) {
    int len = 1;
    while (isalnum((unsigned char)parser->position[len])) len++;
    return be_token(parser, token, len);
}

static inline bool be_is_keyword(Be_Parser* parser, int len, const char* keyword, int keyword_len) {
    return be_string_equals((Be_String){parser->position, len}, (Be_String){keyword, keyword_len});
}

static Be_Token be_keyword_kind(Be_Parser* parser, int len) {
    switch (parser->position[0]) {
        case 'a':
            return be_is_keyword(parser, len, "add", 3) ? BE_TOKEN_ADD : BE_TOKEN_LABEL;
        case 'b':
            return be_is_keyword(parser, len, "b", 1) ? BE_TOKEN_B
                : be_is_keyword(parser, len, "br", 2) ? BE_TOKEN_BR : BE_TOKEN_LABEL;
        case 'c':
            return be_is_keyword(parser, len, "cbr", 3) ? BE_TOKEN_CBR : BE_TOKEN_LABEL;
        case 'e':
            return be_is_keyword(parser, len, "eq", 2) ? BE_TOKEN_EQ : BE_TOKEN_LABEL;
        case 'f':
            return be_is_keyword(parser, len, "func", 4) ? BE_TOKEN_FUNC : BE_TOKEN_LABEL;
        case 'i':
            return be_is_keyword(parser, len, "i", 1) ? BE_TOKEN_I : BE_TOKEN_LABEL;
        case 'l':
            return be_is_keyword(parser, len, "l", 1) ? BE_TOKEN_L : BE_TOKEN_LABEL;
        case 'r':
            return be_is_keyword(parser, len, "ret", 3) ? BE_TOKEN_RET: BE_TOKEN_LABEL;
        case 's':
            return be_is_keyword(parser, len, "s", 1) ? BE_TOKEN_S : BE_TOKEN_LABEL;
        default:
            return BE_TOKEN_LABEL;
    }
}

static Be_Token be_keyword(Be_Parser* parser) {
    int len = 1;
    while (isalnum((unsigned char)parser->position[len])) len++;
    Be_Token token = be_keyword_kind(parser, len);
    return be_token(parser, token, len);
}

static Be_Token be_number(Be_Parser* parser) {
    int len = 1;
    while (isdigit((unsigned char)parser->position[len])) len++;
    return be_token(parser, BE_TOKEN_INT, len);
}

static Be_Token be_next_token(Be_Parser* parser) {
    parser->column += parser->symbol.len;
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
    be_error(
        parser->filename,
        parser->line,
        parser->column,
        "unknown symbol `%c`",
        parser->position[0]
    );
    longjmp(parser->env, 1);
}

static bool be_accept(Be_Parser* parser, Be_Token token) {
    if (parser->token == token) {
        be_next_token(parser);
        return true;
    }
    return false;
}

static void be_expect(Be_Parser* parser, Be_Token token, const char* msg) {
    if (parser->token == token) {
        be_next_token(parser);
        return;
    }
    if (parser->token == BE_TOKEN_EOF) be_error(
        parser->filename,
        parser->line,
        parser->column,
        "%s instead of end of file",
        msg
    ); else be_error(
        parser->filename,
        parser->line,
        parser->column,
        "%s instead of `%.*s`",
        msg,
        parser->symbol.len,
        parser->symbol.str
    );
    longjmp(parser->env, 1);
}

static Be_Type be_parse_type(Be_Parser* parser, const char* msg) {
    switch (parser->token) {
        case BE_TOKEN_I:
            be_next_token(parser);
            return BE_TYPE_INT;
        case BE_TOKEN_L:
            be_next_token(parser);
            return BE_TYPE_LONG;
        default:
            be_error(
                parser->filename,
                parser->line,
                parser->column,
                msg
            );
            break;
    }
    longjmp(parser->env, 1);
}

static int be_push_value(Be_Parser* parser, Be_Function* function, bool defined) {
    Be_Value* value;
    for (int i = 0; i < function->values.size; i++) {
        value = function->values.elements[i];
        if (be_string_equals(value->name, parser->symbol)) return i;
    }
    value = be_value(parser->arena);
    value->name = parser->symbol;
    value->id = be_vector_push(&function->values, value);
    value->line = parser->line;
    value->column = parser->column;
    value->defined = defined;
    return value->id;
}

static void be_parse_operand(Be_Parser* parser, Be_Function* function, Be_Operand* operand) {
    char* endptr;
    switch (parser->token) {
        case BE_TOKEN_LOCAL:
            operand->kind = BE_OPERAND_VALUE;
            operand->value_id = be_push_value(parser, function, false);
            break;
        case BE_TOKEN_INT:
            operand->kind = BE_OPERAND_INT;
            operand->int_const = strtol(parser->symbol.str, &endptr, 10);
            if (endptr != parser->position) {
                be_error(
                    parser->filename,
                    parser->line,
                    parser->column,
                    "invalid integer `%.*s`",
                    parser->symbol.len,
                    parser->symbol.str
                );
                longjmp(parser->env, 1);
            }
            break;
        default:
            be_error(parser->filename, parser->line, parser->column, "expected operand");
            longjmp(parser->env, 1);
    }
    be_next_token(parser);
}

static Be_Instruction* be_parse_instruction(Be_Parser* parser, Be_Function* function) {
    Be_Instruction* instruction = be_instruction(parser->arena);
    if (parser->token == BE_TOKEN_LOCAL) {
        instruction->result_id = be_push_value(parser, function, true);
        be_next_token(parser);
        be_expect(parser, BE_TOKEN_EQSIGN, "expected `=`");
        instruction->type = be_parse_type(parser, "expected instruction type");
    } else instruction->type = BE_TYPE_VOID;
    switch (parser->token) {
        case BE_TOKEN_INT:
            instruction->kind = BE_INSTRUCTION_CONST;
            be_parse_operand(parser, function, &instruction->constant.operand);
            break;
        case BE_TOKEN_ADD:
            instruction->kind = BE_INSTRUCTION_ADD;
            be_next_token(parser);
            goto binary;
        case BE_TOKEN_EQ:
            instruction->kind = BE_INSTRUCTION_EQ;
            be_next_token(parser);
            goto binary;
        binary:
            be_parse_operand(parser, function, &instruction->binary.lhs);
            be_expect(parser, BE_TOKEN_COMMA, "expected `,`");
            be_parse_operand(parser, function, &instruction->binary.rhs);
            break;
        default:
            be_error(parser->filename, parser->line, parser->column, "expected instruction");
            longjmp(parser->env, 1);
    }
    return instruction;
}

static bool be_at_terminator(const Be_Parser* parser) {
    switch (parser->token) {
        case BE_TOKEN_RET:
        case BE_TOKEN_BR:
        case BE_TOKEN_CBR:
            return true;
        default:
            return false;
    }
}

static void be_parse_terminator(Be_Parser* parser, Be_Function* function, Be_Block* block) {
    switch (parser->token) {
        case BE_TOKEN_RET:
            block->terminator.kind = BE_TERMINATOR_RET;
            block->terminator.ret.line = parser->line;
            block->terminator.ret.column = parser->column;
            be_next_token(parser);
            block->terminator.ret.type = be_parse_type(parser, "expected return type");
            be_parse_operand(parser, function, &block->terminator.ret.operand);
            break;
        case BE_TOKEN_BR:
            block->terminator.kind = BE_TERMINATOR_BR;
            be_next_token(parser);
            block->terminator.br.block = (Be_Block_Reference){
                .label = parser->symbol,
                .block_id = -1,
                .line = parser->line,
                .column = parser->column,
            };
            be_expect(parser, BE_TOKEN_LABEL, "expected label");
            break;
        case BE_TOKEN_CBR:
            block->terminator.kind = BE_TERMINATOR_CBR;
            be_next_token(parser);
            block->terminator.cbr.type = be_parse_type(parser, "expected condition type");
            be_parse_operand(parser, function, &block->terminator.cbr.condition);
            be_expect(parser, BE_TOKEN_COMMA, "expected `,`");
            block->terminator.cbr.true_block = (Be_Block_Reference){
                .label = parser->symbol,
                .block_id = -1,
                .line = parser->line,
                .column = parser->column,
            };
            be_expect(parser, BE_TOKEN_LABEL, "expected label");
            be_expect(parser, BE_TOKEN_COMMA, "expected `,`");
            block->terminator.cbr.false_block = (Be_Block_Reference){
                .label = parser->symbol,
                .block_id = -1,
                .line = parser->line,
                .column = parser->column,
            };
            be_expect(parser, BE_TOKEN_LABEL, "expected label");
            break;
        default:
            be_error(parser->filename, parser->line, parser->column, "expected terminator");
            longjmp(parser->env, 1);
    }
}

static Be_Block* be_parse_block(Be_Parser* parser, Be_Function* function) {
    Be_Block* block = be_block(parser->arena);
    block->label = parser->symbol;
    be_expect(parser, BE_TOKEN_LABEL, "expected label");
    be_expect(parser, BE_TOKEN_COLON, "expected `:`");
    while (!be_at_terminator(parser)) {
        Be_Instruction* instruction = be_parse_instruction(parser, function);
        be_vector_push(&block->instructions, instruction);
    }
    be_parse_terminator(parser, function, block);
    return block;
}

static void be_parse_parameters(Be_Parser* parser, Be_Function* function) {
    be_expect(parser, BE_TOKEN_LPAREN, "expected `(`");
    while (!be_accept(parser, BE_TOKEN_RPAREN)) {
        Be_Value* value = be_value(parser->arena);
        value->defined = true;
        value->type = be_parse_type(parser, "expected parameter type");
        value->name = parser->symbol;
        value->line = parser->line;
        value->column = parser->column;
        be_expect(parser, BE_TOKEN_LOCAL, "expected local name");
        be_vector_push(&function->parameters, value);
        value->id = be_vector_push(&function->values, value);
        be_accept(parser, BE_TOKEN_COMMA);
    }
}

static Be_Function* be_parse_function(Be_Parser* parser) {
    Be_Function* function = be_function(parser->arena);
    function->type = be_parse_type(parser, "expected function type");
    function->name = parser->symbol;
    be_expect(parser, BE_TOKEN_GLOBAL, "expected global name for function");
    be_parse_parameters(parser, function);
    be_expect(parser, BE_TOKEN_LBRACE, "expected `{` after function parameters");
    while (parser->token != BE_TOKEN_RBRACE) {
        Be_Block* block = be_parse_block(parser, function);
        block->id = be_vector_push(&function->blocks, block);
    }
    return function;
}

Be_Module* be_parse_module(Be_Parser* parser) {
    Be_Module* module = be_module(parser->arena);
    module->filename = parser->filename;
    be_next_token(parser);
    while (parser->token != BE_TOKEN_EOF) {
        be_expect(parser, BE_TOKEN_FUNC, "expected `func`");
        Be_Function* function = be_parse_function(parser);
        be_vector_push(&module->functions, function);
        be_next_token(parser);
    }
    return module;
}

Be_Module* be_parse(Be_Arena* arena, const char* filename, const char* source) {
    Be_Parser parser = {
        .arena = arena,
        .filename = filename,
        .source = source,
        .position = source,
        .symbol = (Be_String){0},
        .line = 1,
        .column = 1,
    };
    if (setjmp(parser.env) == 0) return be_parse_module(&parser);
    else return NULL;
}
