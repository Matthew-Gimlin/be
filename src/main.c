#include <stdio.h>
#include <getopt.h>
#include "be.h"

static const char* const VERSION = "0.0.0";
static const char* const HELP =
    "usage:\n"
    "  be [options] FILE\n"
    "\n"
    "options:\n"
    "  -v, --version         show the version and exit\n"
    "  -h, --help            show the help message and exit\n"
    "  -O, --optimize LEVEL  optimize the output to LEVEL (default 1)\n"
    "  -o, --output FILE     write the output to FILE\n"
;

static struct option long_opts[] = {
    {"version", no_argument, NULL, 'v'},
    {"help", no_argument, NULL, 'h'},
    {"optimize", no_argument, NULL, 'O'},
    {"output", no_argument, NULL, 'o'},
    {NULL, 0, NULL, 0},
};

int main(int argc, char** argv) {
    opterr = 0;
    int opt;
    int level = 1;
    FILE* output = stdout;
    while ((opt = getopt_long(argc, argv, ":vhO:o:", long_opts, NULL)) != -1) {
        switch (opt) {
            case 'v':
                puts(VERSION);
                return 0;
            case 'h':
                fputs(HELP, stdout);
                return 0;
            case 'O':
                level = atoi(optarg);
                break;
            case 'o':
                output = fopen(optarg, "wb");
                if (!output) be_fatal("could not open %s", optarg);
                break;
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
    if (level > 0) {
        be_fold(module);
        be_build_cfg(module);
        be_simplify_branches(module);
        be_simplify(module);
        be_remove_unreachable(module);
    }
    be_dump(output, module);
    be_free_arena(&arena);
    if (output != stdout) fclose(output);
    return 0;
}
