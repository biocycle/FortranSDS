#include "c99/sds.h"
#include "c99/util.h"

#include <stdio.h>

static char *sds_type_names[] = {
    "<no type>", "int8", "uint8", "int16", "uint16", "int32", "uint32",
    "float", "double", "string"
};

static void print_atts(SDSAttInfo *att)
{
    while (att) {
        printf("\n  %s %s = ", sds_type_names[att->type], att->name);

        switch (att->type) {
        case SDS_NO_TYPE:
            puts("?");
            break;
        case SDS_I8:
            for (int i = 0; i < att->count; i++) {
                printf("%i", att->data.b[i]);
                if (i + 1 < att->count)
                    printf(", ");
            }
            break;
        case SDS_U8:
            for (int i = 0; i < att->count; i++) {
                printf("%u", (unsigned)att->data.ub[i]);
                if (i + 1 < att->count)
                    printf(", ");
            }
            break;
        case SDS_I16:
            for (int i = 0; i < att->count; i++) {
                printf("%i", att->data.s[i]);
                if (i + 1 < att->count)
                    printf(", ");
            }
            break;
        case SDS_U16:
            for (int i = 0; i < att->count; i++) {
                printf("%u", (unsigned)att->data.us[i]);
                if (i + 1 < att->count)
                    printf(", ");
            }
            break;
        case SDS_I32:
            for (int i = 0; i < att->count; i++) {
                printf("%i", att->data.i[i]);
                if (i + 1 < att->count)
                    printf(", ");
            }
            break;
        case SDS_U32:
            for (int i = 0; i < att->count; i++) {
                printf("%u", (unsigned)att->data.ui[i]);
                if (i + 1 < att->count)
                    printf(", ");
            }
            break;
        case SDS_FLOAT:
            for (int i = 0; i < att->count; i++) {
                printf("%f", (double)att->data.f[i]);
                if (i + 1 < att->count)
                    printf(", ");
            }
            break;
        case SDS_DOUBLE:
            for (int i = 0; i < att->count; i++) {
                printf("%f", att->data.d[i]);
                if (i + 1 < att->count)
                    printf(", ");
            }
            break;
        case SDS_STRING:
            printf("\"%s\"", att->data.str);
            break;
        }

        att = att->next;
    }
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        printf("Usage: %s INFILE.hdf\n", argv[0]);
        return -1;
    }

    SDSInfo *sds = open_hdf_sds(argv[1]);

    printf("%s: %u atts, %u dims, %u vars\n", sds->path,
           (unsigned)list_count((List *)sds->gatts),
           (unsigned)list_count((List *)sds->dims),
           (unsigned)list_count((List *)sds->vars));

    if (sds->gatts) {
        puts("\nGlobal attributes:");
        print_atts(sds->gatts);
    } else {
        puts("\nNo global attributes");
    }

    puts("\nDimensions:");
    SDSDimInfo *dim = sds->dims;
    while (dim) {
        printf("  %s = %u%s\n", dim->name, (unsigned)dim->size,
               dim->isunlim ? " (unlimited)" : "");
        dim = dim->next;
    }


    fputs("\nVariables:", stdout);
    SDSVarInfo *var = sds->vars;
    while (var) {
        printf("\n\n%s %s", sds_type_names[var->type], var->name);
        for (int i = 0; i < var->ndims; i++) {
            printf("[%s]", var->dims[i]->name);
        }
        print_atts(var->atts);

        var = var->next;
    }
    puts("");

    sds_close(sds);

    return 0;
}
