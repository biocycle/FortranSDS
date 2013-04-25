/* sds-dump.c - prints human- and script-readable parts of supported SDS files.
 */
#include "c99/sds.h"
#include "c99/util.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static const int TYPE_COLOR = 2;
static const int VARNAME_COLOR = 3;
static const int DIMNAME_COLOR = 14;
static const int VALUE_COLOR = 15;
static const int QUOTE_COLOR = 5;

struct OutOpts {
    char *infile;
    int color;
};

static struct OutOpts opts = {NULL, 0};

/* Print ANSI terminal color escape sequence.
 * Color: 0 - black, 1 - red, 2 - green, 3 - yellow, 4 - blue, 5 - dark magenta,
 *        6 - cyan, 7 - white; 10 + any previous color turns on bold.
 */
static void esc_color(int color)
{
    if (opts.color) {
        if (color < 10) {
            printf("\033[%im", 30 + color);
        } else {
            printf("\033[%i;1m", 20 + color);
        }
    }
}

static void esc_stopcolor(void)
{
    if (opts.color)
        fputs("\033[0m", stdout);
}

static void print_type(SDSType type, int min_width)
{
    esc_color(TYPE_COLOR);
    const char *s = sds_type_names[type];
    int w = strlen(s);
    fputs(s, stdout);
    for (; w < min_width; w++) {
        putc(' ', stdout);
    }
    esc_stopcolor();
}

static void print_atts(SDSAttInfo *att)
{
    fputs("  ", stdout);
    print_type(att->type, 7);
    esc_color(VARNAME_COLOR);
    fputs(att->name, stdout);
    esc_stopcolor();

    // string length
    if (att->type == SDS_STRING) {
        printf("[%u]", (unsigned)(att->count - 1));
    }

    fputs(" = ", stdout);

    // string value
    if (att->type == SDS_STRING) {
        esc_color(QUOTE_COLOR);
        putc('"', stdout);
        esc_stopcolor();
        esc_color(VALUE_COLOR);
        fputs(att->data.str, stdout);
        esc_stopcolor();
        esc_color(QUOTE_COLOR);
        putc('"', stdout);
        esc_stopcolor();

        goto checknext;
    }

    // all other value types
    for (int i = 0;;) {
        esc_color(VALUE_COLOR);
        switch (att->type) {
        case SDS_NO_TYPE:
            fputs("?", stdout);
            break;
        case SDS_I8:
            printf("%i", att->data.b[i]);
            break;
        case SDS_U8:
            printf("%u", (unsigned)att->data.ub[i]);
            break;
        case SDS_I16:
            printf("%i", att->data.s[i]);
            break;
        case SDS_U16:
            printf("%u", (unsigned)att->data.us[i]);
            break;
        case SDS_I32:
            printf("%i", att->data.i[i]);
            break;
        case SDS_U32:
            printf("%u", (unsigned)att->data.ui[i]);
            break;
        case SDS_I64:
            printf("%li", (long int)att->data.i64[i]);
            break;
        case SDS_U64:
            printf("%lu", (long unsigned int)att->data.u64[i]);
            break;
        case SDS_FLOAT:
            printf("%g", (double)att->data.f[i]);
            break;
        case SDS_DOUBLE:
            printf("%g", att->data.d[i]);
            break;
        case SDS_STRING:
            abort();
            break;
        }
        esc_stopcolor();
        if (++i >= att->count)
            break;
        printf(", ");
    }

 checknext:
    puts("");
    if (att->next)
        print_atts(att->next);
}

static const char *USAGE =
    "Usage: %s [OPTION]... INFILE\n"
    "Dumps part or all of INFILE, producing a colorful summary of its contents "
    "by default.\n"
    "\n"
    "Options:\n"
    "  -G   Always color the output.\n"
    "  -h   Print this help and exit.\n"
;

static void usage(const char *progname, const char *message, ...)
{
    char *pname = strrchr(progname, '/');
    if (pname) {
        pname++;
    } else {
        pname = (char *)progname;
    }
    fprintf(stderr, "%s: ", pname);

    va_list ap;
    va_start(ap, message);
    vfprintf(stderr, message, ap);
    va_end(ap);
    puts("\n");

    fprintf(stderr, USAGE, pname);
    exit(-1);
}

static void parse_arg(int argc, char **argv, int i)
{
    if (!strcmp(argv[i]+1, "h")) { // help
        printf(USAGE, argv[0]);
        exit(0);
    } else if (!strcmp(argv[i]+1, "G")) { // force color on
        opts.color = 1;
    } else {
        usage(argv[0], "unrecognized command line option '%s'", argv[i]);
    }
}

static void parse_args(int argc, char **argv)
{
    if (isatty(STDOUT_FILENO))
        opts.color = 1; // default to color when printing to the terminal

    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-') { // parse the option
            parse_arg(argc, argv, i);
        } else { // this is the infile
            if (opts.infile) {
                usage(argv[0], "only one input file is allowed");
            }
            opts.infile = argv[i];
        }
    }

    if (!opts.infile) {
        usage(argv[0], "you need to specify an input file");
    }
}

int main(int argc, char **argv)
{
    parse_args(argc, argv);

    SDSInfo *sds = open_any_sds(opts.infile);
    if (!sds) {
        printf("%s: error opening file\n", opts.infile);
        return -2;
    }

    printf("%s:\n  %s format; %u global attributes, %u dimensions, %u variables\n", sds->path,
           sds_file_types[(int)sds->type],
           (unsigned)list_count((List *)sds->gatts),
           (unsigned)list_count((List *)sds->dims),
           (unsigned)list_count((List *)sds->vars));

    if (sds->gatts) {
        puts("\nGlobal attributes:");
        print_atts(sds->gatts);
        puts("");
    } else {
        puts("\nNo global attributes");
    }

    puts("Dimensions:");
    SDSDimInfo *dim = sds->dims;
    while (dim) {
        fputs("  ", stdout);
        esc_color(DIMNAME_COLOR);
        fputs(dim->name, stdout);
        esc_stopcolor();

        fputs(" = ", stdout);
        esc_color(VALUE_COLOR);
        printf("%u", (unsigned)dim->size);
        esc_stopcolor();

        puts(dim->isunlim ? " (unlimited)" : "");

        dim = dim->next;
    }

    fputs("\nVariables:\n", stdout);
    SDSVarInfo *var = sds->vars;
    while (var) {
        puts("");
        print_type(var->type, -1);
        putc(' ', stdout);
        esc_color(VARNAME_COLOR);
        fputs(var->name, stdout);
        esc_stopcolor();
        for (int i = 0; i < var->ndims; i++) {
            putc('[', stdout);
            esc_color(DIMNAME_COLOR);
            fputs(var->dims[i]->name, stdout);
            esc_stopcolor();
            printf("=%u]", (unsigned)var->dims[i]->size);
        }
        if (var->iscoord)
            fputs(" (coordinate)\n", stdout);
        else
            puts("");
        if (var->atts)
            print_atts(var->atts);

        var = var->next;
    }
    puts("");

    sds_close(sds);

    return 0;
}
