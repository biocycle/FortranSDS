/* nc2code.c - given a netcdf file, generate code to read or write it.
 * Copyright (C) 2012 Matthew Bishop
 */
#include <assert.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum Language {
    LANG_NONE,
    LANG_C,
    LANG_F90,
    LANG_F77
};

static const char USAGE[] = "Usage: nc2code [OPTION]... FILE.nc\n";
static const char OPTIONS[] =
"Inspect a NetCDF file's metadata and emit code in various languages\n"
"to read or write that file.\n\n"
"Options:\n"
"  -c       generate C code\n"
"  -f       generate Fortran 90 code; default\n"
"  -F       generate Fortran 77 code\n"
"  -h       print this help message\n"
"  -o FILE  write the code to FILE instead of stdout\n"
"  -r       generate code to read this file; default\n"
"  -w       generate code to write this file format\n"
"  -q       goes about its work more quietly\n"
;

/* Options from command line. */
static enum Language gen_lang = LANG_NONE;
static char *input_file = NULL, *output_file = NULL;
static FILE *fout = NULL;
static int gen_read = 0, gen_write = 0;
static int be_verbose = 1;

static void arg_parse_error(const char *argv0, const char *fmt, ...)
{
    va_list ap;

    fputs(USAGE, stderr);
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fprintf(stderr, "\nTry `%s -h` for more information\n", argv0);
    exit(-1);
}

static void parse_args(int argc, char **argv)
{
    int i, j, nexti;

    /* parse 'em */
    for (i = 1; i < argc; i = nexti) {
        nexti = i + 1;
        if (argv[i][0] == '-') {
            for (j = 1; argv[i][j] != '\0'; j++) {
                switch (argv[i][j]) {
                case 'c':
                    gen_lang = LANG_C;
                    break;
                case 'f':
                    gen_lang = LANG_F90;
                    break;
                case 'F':
                    gen_lang = LANG_F77;
                    break;
                case 'h':
                    puts(USAGE);
                    puts(OPTIONS);
                    exit(0);
                    break;
                case 'o':
                    if (i + 1 < argc) {
                        output_file = argv[i + 1];
                        nexti = i + 2;
                    } else {
                        arg_parse_error(argv[0], "Missing output file after '%s'", argv[i]);
                    }
                    break;
                case 'r':
                    gen_read = 1;
                    break;
                case 'q':
                    be_verbose = 0;
                    break;
                case 'w':
                    gen_write = 1;
                    break;
                default:
                    arg_parse_error(argv[0], "Invalid option '%c'", argv[i][j]);
                    break;
                }
            }
        } else if (input_file) {
            arg_parse_error(argv[0], "Input file already given");
        } else {
            input_file = argv[i];
        }
    }

    /* verify 'em */
    if (!input_file) {
        arg_parse_error(argv[0], "No input file given");
    }

    if (gen_lang == LANG_NONE) {
        if (be_verbose)
            puts("Defaulting to Fortran 90 output");
        gen_lang = LANG_F90;
    }

    if (output_file && be_verbose) {
        printf("Writing output to %s instead of stdout\n", output_file);
    }

    if (!gen_read && !gen_write) {
        if (be_verbose)
            puts("Defaulting to generating read code");
        gen_read = 1;
    }
}

int main(int argc, char **argv)
{
    parse_args(argc, argv);

    /* read netcdf metadata */

    if (output_file) {
        fout = fopen(output_file, "w");
        if (!fout) {
            fprintf(stderr, "Opening output file '%s': %s\n", output_file,
                    strerror(errno));
            exit(-1);
        }
    } else {
        fout = stdout;
    }

    if (fout != stdout && fclose(fout) != 0) {
        fprintf(stderr, "Closing output file '%s': %s\n", output_file,
                strerror(errno));
        exit(-1);
    }
    return 0;
}
