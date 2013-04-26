/* sds-dump.c - prints human- and script-readable parts of supported SDS files.
 */
#include "c99/sds.h"
#include "c99/util.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
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
    LIST_ATTS,
    LIST_DIM_SIZES,
    PRINT_VAR
};

struct OutOpts {
    char *infile;
    int color;
    int single_column;
    char *separator;
    enum OutputType out_type;
    const char *name; // dim, var, etc. to narrow output to
};

static struct OutOpts opts = {NULL, 0, 0, " ", FULL_SUMMARY, NULL};

#ifndef S_ISLNK
#  define S_ISLNK(mode) (((mode) & S_IFLNK) != 0)
#endif

static int file_exists(const char *path)
{
    struct stat buf;
    if (stat(path, &buf)) {
        return 0; // error stat()ing so, not a file for our purposes
    }
    if ((S_ISREG(buf.st_mode) || S_ISLNK(buf.st_mode))) {
        return 1;
    }
    return 0;
}

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

static void print_value(SDSType type, void *ary, size_t u)
{
    esc_color(VALUE_COLOR);
    switch (type) {
    case SDS_NO_TYPE:
        fputs("?", stdout);
        break;
    case SDS_I8:
        printf("%i", ((int8_t *)ary)[u]);
        break;
    case SDS_U8:
        printf("%u", ((uint8_t *)ary)[u]);
        break;
    case SDS_I16:
        printf("%i", ((int16_t *)ary)[u]);
        break;
    case SDS_U16:
        printf("%u", ((uint16_t *)ary)[u]);
        break;
    case SDS_I32:
        printf("%i", ((int32_t *)ary)[u]);
        break;
    case SDS_U32:
        printf("%u", ((uint32_t *)ary)[u]);
        break;
    case SDS_I64:
        printf("%li", ((int64_t *)ary)[u]);
        break;
    case SDS_U64:
        printf("%lu", ((uint64_t *)ary)[u]);
        break;
    case SDS_FLOAT:
        printf("%g", ((float *)ary)[u]);
        break;
    case SDS_DOUBLE:
        printf("%g", ((double *)ary)[u]);
        break;
    case SDS_STRING:
        fprintf(stderr, "asked to print_value(SDS_STRING, ...)!");
        abort();
        break;
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
        print_value(att->type, att->data.v, i);
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
        puts("\n - no global attributes -\n");
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

static SDSVarInfo *var_or_die(SDSInfo *sds, const char *varname)
{
    SDSVarInfo *var = sds_var_by_name(sds->vars, varname);
    if (!var) {
        fprintf(stderr, "%s: no variable '%s' found\n", opts.infile, varname);
        exit(-3);
    }
    return var;
}

static void print_list_atts(SDSInfo *sds)
{
    SDSAttInfo *att = sds->gatts;
    if (opts.name) {
        att = var_or_die(sds, opts.name)->atts;
    }

    for (; att != NULL; att = att->next) {
        esc_color(ATTNAME_COLOR);
        fputs(att->name, stdout);
        esc_stop();
        fputs(opts.separator, stdout);
    }
    if (!opts.single_column)
        puts("");
}

static void print_dim(SDSDimInfo *dim)
{
    esc_color(DIMNAME_COLOR);
    fputs(dim->name, stdout);
    esc_stop();
    fputs(opts.separator, stdout);
}

static void print_list_dims(SDSInfo *sds)
{
    if (opts.name) {
        SDSVarInfo *var = var_or_die(sds, opts.name);
        for (int i = 0; i < var->ndims; i++)
            print_dim(var->dims[i]);
    } else {
        for (SDSDimInfo *dim = sds->dims; dim != NULL; dim = dim->next)
            print_dim(dim);
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

static void print_dim_sizes(SDSInfo *sds)
{
    for (SDSDimInfo *dim = sds->dims; dim != NULL; dim = dim->next) {
        esc_color(VALUE_COLOR);
        printf("%u", (unsigned)dim->size);
        esc_stop();
        fputs(opts.separator, stdout);
    }
    if (!opts.single_column)
        puts("");
}

static void print_var_dim_sizes(SDSInfo *sds)
{
    SDSVarInfo *var = var_or_die(sds, opts.name);

    for (int i = 0; i < var->ndims; i++) {
        esc_color(VALUE_COLOR);
        printf("%u", (unsigned)var->dims[i]->size);
        esc_stop();
        fputs(opts.separator, stdout);
    }
    if (!opts.single_column)
        puts("");
}

static void print_some_values(SDSType type, void *values, size_t count)
{
    for (size_t u = 0;;) {
        print_value(type, values, u);
        if (++u >= count)
            break;
        fputs(opts.separator, stdout);
    }
}

static void print_var_values(SDSInfo *sds)
{
    SDSVarInfo *var = var_or_die(sds, opts.name);

    size_t count_per_tstep = 1;
    for (int i = 1; i < var->ndims; i++)
        count_per_tstep *= var->dims[i]->size;

    void *buf = NULL;
    if (var->ndims > 1) {
        for (int tstep = 0; tstep < var->dims[0]->size; tstep++) {
            void *values = sds_timestep(var, &buf, tstep);
            print_some_values(var->type, values, count_per_tstep);
            fputs(opts.separator, stdout);
        }
    } else {
        void *values = sds_read(var, &buf);
        if (var->ndims == 0) {
            print_value(var->type, values, 0);
        } else {
            print_some_values(var->type, values, var->dims[0]->size);
        }
    }
    sds_buffer_free(buf);

    if (!opts.single_column)
        puts("");
}

static const char *USAGE =
    "Usage: %s [OPTION]... INFILE\n"
    "Dumps part or all of INFILE, producing a colorful summary of its contents "
    "by default.\n"
    "\n"
    "Options:\n"
    "  -1          output values in a single column\n"
    "  -d [VAR]    print dimension sizes for the whole file or the specified\n"
    "              variable, if given\n"
    "  -g          never color the output\n"
    "  -G          always color the output\n"
    "  -h          print this help and exit\n"
    "  -la [VAR]   list the attributes in the file or for the specified\n"
    "              variable if given\n"
    "  -ld [VAR]   list the dimensions in the file or for the specified\n"
    "              variable if given\n"
    "  -lv         list the variables in the file\n"
    "  -v VAR      print the specified variable's values\n"
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

/* For command line options that take optional non-file-name arguments
 * (typically variable names), return the name if given and NULL otherwise.
 * Increments ip if the optional argument is found.
 */
static const char *get_optional_arg(int argc, char **argv, int *ip)
{
    if (*ip + 1 >= argc) {
        return NULL; // past end of argv
    }
    char *arg = argv[*ip + 1];
    if (arg[0] != '-' && !file_exists(arg)) {
        (*ip)++;
        return arg; // looks like a non-file argument!
    }
    return NULL;
}

static void parse_arg(int argc, char **argv, int *ip)
{
    char *opt = argv[*ip]+1;

    if (!strcmp(opt, "1")) { // single-column output mode
        opts.single_column = 1;
        opts.separator = "\n";
    } else if (!strcmp(opt, "d")) { // list dim sizes
        opts.out_type = LIST_DIM_SIZES;
        opts.name = get_optional_arg(argc, argv, ip);
    } else if (!strcmp(opt, "g")) { // force color off
        opts.color = 0;
    } else if (!strcmp(opt, "G")) { // force color on
        opts.color = 1;
    } else if (!strcmp(opt, "h")) { // help
        printf(USAGE, argv[0]);
        exit(0);
    } else if (!strcmp(opt, "la")) { // list atts
        opts.out_type = LIST_ATTS;
        opts.name = get_optional_arg(argc, argv, ip);
    } else if (!strcmp(opt, "ld")) { // list dims
        opts.out_type = LIST_DIMS;
        opts.name = get_optional_arg(argc, argv, ip);
    } else if (!strcmp(opt, "lv")) { // list vars
        opts.out_type = LIST_VARS;
    } else if (!strcmp(opt, "v")) { // print var's values
        opts.out_type = PRINT_VAR;
        opts.name = get_optional_arg(argc, argv, ip);
        if (!opts.name)
            usage(argv[0], "missing variable name argument to -v");
    } else {
        usage(argv[0], "unrecognized command line option '%s'", argv[*ip]);
    }
}

static void parse_args(int argc, char **argv)
{
    if (isatty(STDOUT_FILENO))
        opts.color = 1; // default to color when printing to the terminal

    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-') { // parse the option
            parse_arg(argc, argv, &i);
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
        print_list_atts(sds);
        break;
    case LIST_DIMS:
        print_list_dims(sds);
        break;
    case LIST_VARS:
        print_list_vars(sds->vars);
        break;
    case LIST_DIM_SIZES:
        if (opts.name)
            print_var_dim_sizes(sds);
        else
            print_dim_sizes(sds);
        break;
    case PRINT_VAR:
        print_var_values(sds);
        break;
    default:
        abort();
    }

    sds_close(sds);

    return 0;
}
