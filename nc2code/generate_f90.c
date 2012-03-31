#include "sds.h"
#include "util.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

#define MAX_WIDTH (80)

static const char CHECK_FUNCTION[] =
"subroutine checknc(status)\n"
"    integer, intent(in) :: status\n"
"\n"
"    if (status /= NF90_NOERR) then\n"
"        print *, nf90_strerror(status)\n"
"        stop\n"
"    end if\n"
"end subroutine checknc\n"
;

static void print_att_decl(FILE *fout, const char *type,
                       SDSAttInfo *last, SDSAttInfo *ai)
{
    if (last && last->count == ai->count) {
        fprintf(fout, ", %s", ai->name);
    } else {
        if (last) fputs("\n", fout);
        fputs(type, fout);
        if (ai->count < 2) {
            fputs(" :: ", fout);
        } else {
            fprintf(fout, ", dimension(%u) :: ", (unsigned)ai->count);
        }
    }
    fprintf(fout, "%s", ai->name);
}

static void generate_f90_var_att(FILE *fout, const char *varname,
                                 SDSAttInfo *atts)
{
    SDSAttInfo *ai, *last;
    char *var_id;

    if (!varname) {
        varname = "global";
        var_id = NEWA(char, 12);
        strcpy(var_id, "NF90_GLOBAL");
    } else {
        var_id = NEWA(char, strlen(varname) + 4);
        sprintf(var_id, "%s_id", varname);
    }

    fprintf(fout, "! %s attributes\n", varname);

    /* att variables */
    last = NULL;
    for (ai = atts; ai != NULL; ai = ai->next) {
        if (ai->type == SDS_STRING) {
            if (last && last->count == ai->count) {
                fprintf(fout, ", %s", ai->name);
            } else {
                if (last) fputs("\n", fout);
                fprintf(fout, "character(%u) :: %s",
                        (unsigned)ai->count, ai->name);
            }
            last = ai;
        }
    }
    if (last) fputs("\n", fout);

    last = NULL;
    for (ai = atts; ai != NULL; ai = ai->next) {
        if (ai->type == SDS_BYTE ||
            ai->type == SDS_SHORT ||
            ai->type == SDS_INT) {
            print_att_decl(fout, "integer", last, ai);
            last = ai;
        }
    }
    if (last) fputs("\n", fout);

    last = NULL;
    for (ai = atts; ai != NULL; ai = ai->next) {
        if (ai->type == SDS_FLOAT) {
            print_att_decl(fout, "real*4", last, ai);
            last = ai;
        }
    }
    if (last) fputs("\n", fout);

    last = NULL;
    for (ai = atts; ai != NULL; ai = ai->next) {
        if (ai->type == SDS_DOUBLE) {
            print_att_decl(fout, "real*8", last, ai);
            last = ai;
        }
    }
    if (last) fputs("\n", fout);

    fputs("\n", fout);

    /* read attributes */
    for (ai = atts; ai != NULL; ai = ai->next) {
        fprintf(fout, "call checknc( nf90_get_att(ncid, %s, \"%s\", %s) )\n",
                var_id, ai->name, ai->name);
    }
    fputs("\n", fout);

    free(var_id);
}

/* Selection sort of a given attribute list, in this order:
 * characters, then by length
 * integrals (byte, short, int) any order
 * double
 * float
 */
static int attinfo_cmp(SDSAttInfo *a, SDSAttInfo *b)
{
    assert(a->type != SDS_NO_TYPE);
    assert(b->type != SDS_NO_TYPE);

    switch (a->type) {
    case SDS_BYTE:
    case SDS_SHORT:
    case SDS_INT:
        switch (b->type) {
        case SDS_BYTE:
        case SDS_SHORT:
        case SDS_INT:
            return a->count - b->count;
        case SDS_FLOAT:
        case SDS_DOUBLE:
            return 1;
        case SDS_STRING:
            return -1;
        default:
            abort();
        }
    case SDS_FLOAT:
        switch (b->type) {
        case SDS_BYTE:
        case SDS_SHORT:
        case SDS_INT:
            return -1;
        case SDS_FLOAT:
            return a->count - b->count;
        case SDS_DOUBLE:
        case SDS_STRING:
            return -1;
        default:
            abort();
        }
    case SDS_DOUBLE:
        switch (b->type) {
        case SDS_BYTE:
        case SDS_SHORT:
        case SDS_INT:
            return -1;
        case SDS_FLOAT:
            return 1;
        case SDS_DOUBLE:
            return a->count - b->count;
        case SDS_STRING:
            return -1;
        default:
            abort();
        }
    case SDS_STRING:
        switch (b->type) {
        case SDS_BYTE:
        case SDS_SHORT:
        case SDS_INT:
        case SDS_FLOAT:
        case SDS_DOUBLE:
            return 1;
        case SDS_STRING:
            return a->count - b->count;
        default:
            abort();
        }
    default:
        abort();
    }
}

/* Selection sort of a given attribute list, in this order:
 * characters, then by length
 * integrals (byte, short, int) any order
 * double
 * float
 */
static SDSAttInfo *sort_attributes(SDSAttInfo *atts)
{
    SDSAttInfo *prev, *smallest, *ai; /* unsorted, being selected from */
    SDSAttInfo *sorted = NULL; /* sorted list */

    while (atts) {
        /* find next smallest */
        prev = NULL;
        smallest = ai = atts;
        while (ai && ai->next) {
            if (attinfo_cmp(smallest, ai->next) > 0) {
                prev = ai;
                smallest = ai->next;
            }
            ai = ai->next;
        }

        /* pull out of unsorted list */
        if (prev)
            prev->next = smallest->next;
        else
            atts = smallest->next;

        /* append to sorted list */
        smallest->next = sorted;
        sorted = smallest;
    }
    return sorted;
}

void generate_f90_code(FILE *fout, SDSInfo *sds, int generate_att)
{
    SDSDimInfo *di;
    SDSVarInfo *vi;

    fputs("use netcdf\n\n", fout);

    fputs("integer :: ncid\n", fout);

    /* dimension id vars */
    fputs("integer :: ", fout);
    for (di = sds->dims; di != NULL; di = di->next) {
        fprintf(fout, "%s_dimid", di->name);
        if (di->next) fputs(", ", fout);
    }
    fputs("\n", fout);

    /* variable id vars */
    if (sds->vars) {
        fputs("integer :: ", fout);
        for (vi = sds->vars; vi != NULL; vi = vi->next) {
            fprintf(fout, "%s_id", vi->name);
            if (vi->next) fputs(", ", fout);
        }
        fputs("\n", fout);
    }

    fputs("\n", fout);

    fputs("! open file\n", fout);
    fprintf(fout, "call checknc( nf90_open(\"%s\", NF90_NOWRITE, ncid) )\n", sds->path);
    fputs("\n", fout);

    fputs("! read dimensions\n", fout);
    for (di = sds->dims; di != NULL; di = di->next) {
        fprintf(fout, "call checknc( nf90_inq_dimid(ncid, \"%s\", %s_dimid) )\n",
                di->name, di->name);
    }
    fputs("\n", fout);

    fputs("! get variable IDs\n", fout);
    for (vi = sds->vars; vi != NULL; vi = vi->next) {
        fprintf(fout, "call checknc( nf90_inq_varid(ncid, \"%s\", %s_id) )\n",
                vi->name, vi->name);
    }
    fputs("\n", fout);

    fputs("! read var data\n", fout);
    for (vi = sds->vars; vi != NULL; vi = vi->next) {
        fprintf(fout, "call checknc( nf90_get_var(ncid, %s_id, %s) )\n",
                vi->name, vi->name);
    }
    fputs("\n", fout);

    /* XXX read last dim of var data in a loop */

    if (generate_att) {
        fputs("\n", fout);

        if (sds->gatts) {
            fputs("\n", fout);
            sds->gatts = sort_attributes(sds->gatts);
            generate_f90_var_att(fout, NULL, sds->gatts);
        }

        for (vi = sds->vars; vi != NULL; vi = vi->next) {
            if (vi->atts)
                generate_f90_var_att(fout, vi->name, vi->atts);
        }
    }

    fputs("! close the file\n", fout);
    fputs("call checknc( nf90_close(ncid) )\n", fout);
    fputs("\n", fout);

    fputs(CHECK_FUNCTION, fout);
}
