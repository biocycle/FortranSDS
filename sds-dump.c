/* sds-dump.c - prints human- and script-readable parts of supported SDS files.
 */
#include "c99/sds.h"
#include "c99/util.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static const int TYPE_COLOR    = 16; // bright cyan
static const int ATTNAME_COLOR = 3; // yellow
static const int VARNAME_COLOR = 2; // green
static const int DIMNAME_COLOR = 5; // magenta
static const int VALUE_COLOR   = 14; // blue
static const int QUOTE_COLOR   = 4;

enum OutputType {
    FULL_SUMMARY,
    LIST_DIMS,
    LIST_VARS,
    LIST_ATTS
};

struct OutOpts {
    char *infile;
    int color;
    int single_column;
    char *separator;
    enum OutputType out_type;
};

static struct OutOpts opts = {NULL, 0, 0, " ", FULL_SUMMARY};

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

static void esc_bold(void)
{
    if (opts.color)
        fputs("\033[1m", stdout);
}

static void esc_stop(void)
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
    esc_stop();
}

static void print_atts(SDSAttInfo *att)
{
    fputs("  ", stdout);
    print_type(att->type, 7);
    esc_color(ATTNAME_COLOR);
    fputs(att->name, stdout);
    esc_stop();

    // string length
    if (att->type == SDS_STRING) {
        printf("[%u]", (unsigned)(att->count - 1));
    }

    fputs(" = ", stdout);

    // string value
    if (att->type == SDS_STRING) {
        esc_color(QUOTE_COLOR);
        putc('"', stdout);
        esc_stop();
        esc_color(VALUE_COLOR);
        fputs(att->data.str, stdout);
        esc_stop();
        esc_color(QUOTE_COLOR);
        putc('"', stdout);
        esc_stop();

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
        esc_stop();
        if (++i >= att->count)
            break;
        printf(", ");
    }

 checknext:
    puts("");
    if (att->next)
        print_atts(att->next);
}

static void print_full_summary(SDSInfo *sds)
{
    esc_bold();
    fputs(sds->path, stdout);
    esc_stop();
    printf(": %s format\n  ", sds_file_types[(int)sds->type]);

    esc_color(ATTNAME_COLOR);
    printf("%u global attributes", (unsigned)list_count((List *)sds->gatts));
    esc_stop();
    fputs(", ", stdout);
    esc_color(DIMNAME_COLOR);
    printf("%u dimensions", (unsigned)list_count((List *)sds->dims));
    esc_stop();
    fputs(", ", stdout);
    esc_color(VARNAME_COLOR);
    printf("%u variables\n", (unsigned)list_count((List *)sds->vars));
    esc_stop();

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
        esc_stop();

        fputs(" = ", stdout);
        esc_color(VALUE_COLOR);
        printf("%u", (unsigned)dim->size);
        esc_stop();

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
        esc_stop();
        for (int i = 0; i < var->ndims; i++) {
            putc('[', stdout);
            esc_color(DIMNAME_COLOR);
            fputs(var->dims[i]->name, stdout);
            esc_stop();
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
}

static void print_list_atts(SDSAttInfo *atts)
{
    for (SDSAttInfo *att = atts; att != NULL; att = att->next) {
        esc_color(ATTNAME_COLOR);
        fputs(att->name, stdout);
        esc_stop();
        fputs(opts.separator, stdout);
    }
    if (!opts.single_column)
        puts("");
}

static void print_list_dims(SDSDimInfo *dims)
{
    for (SDSDimInfo *dim = dims; dim != NULL; dim = dim->next) {
        esc_color(DIMNAME_COLOR);
        fputs(dim->name, stdout);
        esc_stop();
        fputs(opts.separator, stdout);
    }
    if (!opts.single_column)
        puts("");
}

static void print_list_vars(SDSVarInfo *vars)
{
    for (SDSVarInfo *var = vars; var != NULL; var = var->next) {
        esc_color(VARNAME_COLOR);
        fputs(var->name, stdout);
        esc_stop();
        fputs(opts.separator, stdout);
    }
    if (!opts.single_column)
        puts("");
}

static const char *USAGE =
    "Usage: %s [OPTION]... INFILE\n"
    "Dumps part or all of INFILE, producing a colorful summary of its contents "
    "by default.\n"
    "\n"
    "Options:\n"
    "  -1          Output values in a single column (use newline instead of space\n"
    "              as the separator).\n"
    "  -G          Always color the output.\n"
    "  -h          Print this help and exit.\n"
    "  -la [var]   List the attributes for the file or the variable if given\n"
    "  -ld [var]   List the dimensions for the file or the variable if given\n"
    "  -lv         List the variables in the file"
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
    if (!strcmp(argv[i]+1, "1")) { // single-column output mode
        opts.single_column = 1;
        opts.separator = "\n";
    } else if (!strcmp(argv[i]+1, "G")) { // force color on
        opts.color = 1;
    } else if (!strcmp(argv[i]+1, "h")) { // help
        printf(USAGE, argv[0]);
        exit(0);
    } else if (!strcmp(argv[i]+1, "la")) { // list atts [var]
        opts.out_type = LIST_ATTS;
        // XXX check for var
    } else if (!strcmp(argv[i]+1, "ld")) { // list dims [var]
        opts.out_type = LIST_DIMS;
        // XXX check for var
    } else if (!strcmp(argv[i]+1, "lv")) { // list vars
        opts.out_type = LIST_VARS;
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

    switch (opts.out_type) {
    case FULL_SUMMARY:
        print_full_summary(sds);
        break;
    case LIST_ATTS:
        print_list_atts(sds->gatts);
        break;
    case LIST_DIMS:
        print_list_dims(sds->dims);
        break;
    case LIST_VARS:
        print_list_vars(sds->vars);
        break;
    default:
        abort();
    }

    sds_close(sds);

    return 0;
}
