#include <stdio.h>
#include <getopt.h>
#include "be.h"

static const char* const VERSION = "0.0.0";
static const char* const HELP =
    "usage:\n"
    "  be [-v] [-h] input\n"
    "\n"
    "options:\n"
    "  -v   show the version and exit\n"
    "  -h   show the help message and exit\n"
;

int main(int argc, char** argv) {
    opterr = 0;
    int opt;
    while ((opt = getopt(argc, argv, ":vh")) != -1) {
        switch (opt) {
            case 'v':
                puts(VERSION);
                return 0;
            case 'h':
                fputs(HELP, stdout);
                return 0;
            case ':':
                be_fatal("missing argument for -%c option", optopt);
            case '?':
                be_fatal("unknown option -%c", optopt);
        }
    }
    if (optind + 1 != argc) {
        fputs(HELP, stderr);
        return 1;
    }
    Be_Arena arena;
    be_init_arena(&arena);
    const char* filename = argv[optind];
    char* source = be_arena_alloc_file(&arena, filename);
    if (!source) be_fatal("could not read %s", filename);
    Be_Module* module = be_parse(&arena, filename, source);
    be_dump(module);
    be_free_arena(&arena);
    return 0;
}
