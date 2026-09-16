#include <stdio.h>
#include <getopt.h>
#include <stdlib.h>
#include "error.h"
#include "lex.h"

static const char* const VERSION = "0.0.0";
static const char* const HELP =
    "Usage:\n"
    "  be [-v] [-h] [-e target] [-o output] input\n"
    "\n"
    "Options:\n"
    "  -v           Show the version and exit\n"
    "  -h           Show the help message and exit\n"
    "  -e target    Emit assembly for target\n"
    "  -o output    Write the output to a file\n"
;

static char* load(const char* fname) {
    FILE* f = fopen(fname, "rb");
    if (!f) return NULL;
    if (fseek(f, 0, SEEK_END) != 0) {
        fclose(f);
        return NULL;
    }
    long n = ftell(f);
    if (n < 0) {
        fclose(f);
        return NULL;
    }
    rewind(f);
    char* s = malloc(n + 1);
    if (!s) {
        fclose(f);
        return NULL;
    }
    if (fread(s, sizeof(*s), n, f) != (size_t)n) {
        fclose(f);
        free(s);
        return NULL;
    }
    s[n] = '\0';
    fclose(f);
    return s;
}

int main(int argc, char** argv) {
    opterr = 0;
    int opt;
    FILE* output = stdout;
    while ((opt = getopt(argc, argv, ":vhe:o:")) != -1) {
        switch (opt) {
            case 'v':
                puts(VERSION);
                return 0;
            case 'h':
                fputs(HELP, stdout);
                return 0;
            case 'e':
                break;
            case 'o':
                output = fopen(optarg, "wb");
                if (!output) be_fatal("Could not open %s", optarg);
                break;
            case ':':
                be_fatal("Missing argument for -%c option", optopt);
            case '?':
                be_fatal("Unknown option -%c", optopt);
        }
    }
    if (optind + 1 != argc) {
        fputs(HELP, stderr);
        return 1;
    }
    const char* fname = argv[optind];
    char* src = load(fname);
    if (!src) be_fatal("Could not read %s", fname);

    Be_Lexer l;
    be_lex_init(&l, fname, src);
    Be_Token tok;
    while ((tok = be_lex(&l)).kind != BE_TOK_EOF) {
        if (tok.kind == BE_TOK_ERROR)
            be_error(fname, tok.line, "Unknown symbol near %.*s", tok.len, tok.str);
        printf("%02d %.*s\n", tok.kind, tok.len, tok.str);
    }

    if (output != stdout) fclose(output);
    free(src);
    return 0;
}
