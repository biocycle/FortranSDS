/* sds-dump.c - prints human- and script-readable parts of supported SDS files.
 */
#include "c99/sds.h"
#include "c99/util.h"

#include <stdio.h>
#include <string.h>

struct OutOpts {
    int color;
};

static struct OutOpts opts = {1};

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
    esc_color(2);
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
    esc_color(3);
    fputs(att->name, stdout);
    esc_stopcolor();

    // string length
    if (att->type == SDS_STRING) {
        printf("[%u]", (unsigned)(att->count - 1));
    }

    fputs(" = ", stdout);

    // string value
    if (att->type == SDS_STRING) {
        esc_color(5);
        putc('"', stdout);
        esc_stopcolor();
        esc_color(15);
        fputs(att->data.str, stdout);
        esc_stopcolor();
        esc_color(5);
        putc('"', stdout);
        esc_stopcolor();

        goto checknext;
    }

    // all other value types
    for (int i = 0;;) {
        esc_color(15);
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

int main(int argc, char **argv)
{
    if (argc < 2) {
        printf("Usage: %s INFILE.hdf\n", argv[0]);
        return -1;
    }

    SDSInfo *sds = open_any_sds(argv[1]);
    if (!sds) {
        printf("%s: Not a supported Scientific Data Set (SDS) file\n",
               argv[1]);
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
        esc_color(14);
        fputs(dim->name, stdout);
        esc_stopcolor();

        fputs(" = ", stdout);
        esc_color(15);
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
        esc_color(3);
        fputs(var->name, stdout);
        esc_stopcolor();
        for (int i = 0; i < var->ndims; i++) {
            putc('[', stdout);
            esc_color(14);
            fputs(var->dims[i]->name, stdout);
            esc_stopcolor();
            printf("=%u]", var->dims[i]->size);
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
