#include <stdio.h>
#include <getopt.h>
#include "error.h"

static const char* const VERSION = "0.0.0";
static const char* const HELP =
    "Usage:\n"
    "  be [-h] [-o output] input\n"
    "\n"
    "Options:\n"
    "  -v           Show the version and exit\n"
    "  -h           Show the help message and exit\n"
    "  -o output    Write the output to a file\n"
;

int main(int argc, char** argv) {
    opterr = 0;
    int opt;
    FILE* output = stdout;
    while ((opt = getopt(argc, argv, ":vho:")) != -1) {
        switch (opt) {
            case 'v':
                puts(VERSION);
                return 0;
            case 'h':
                fputs(HELP, stdout);
                return 0;
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
    const char* filename = argv[optind];
    if (output != stdout) fclose(output);
    return 0;
}
