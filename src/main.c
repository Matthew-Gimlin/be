#include <stdio.h>
#include <getopt.h>
#include "be.h"

static const char* const VERSION = "0.0.0";
static const char* const HELP =
    "usage:\n"
    "  be [options] input\n"
    "\n"
    "options:\n"
    "  -v, --version\n"
    "    show the version and exit\n"
    "  -h, --help\n"
    "    show the help message and exit\n"
;

static struct option long_opts[] = {
    {"version", no_argument, NULL, 'v'},
    {"help", no_argument, NULL, 'h'},
    {NULL, 0, NULL, 0},
};

int main(int argc, char** argv) {
    opterr = 0;
    int opt;
    while ((opt = getopt_long(argc, argv, ":vh", long_opts, NULL)) != -1) {
        switch (opt) {
            case 'v':
                puts(VERSION);
                return 0;
            case 'h':
                fputs(HELP, stdout);
                return 0;
            case ':':
                be_fatal("missing argument for %s option", argv[optind - 1]);
            case '?':
                be_fatal("unknown option %s", argv[optind - 1]);
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
    if (!module) return 1;
    be_fold(module);
    be_simplify(module);
    be_dump(module);
    be_free_arena(&arena);
    return 0;
}
