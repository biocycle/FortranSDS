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

static int same_var_dims(SDSVarInfo *a, SDSVarInfo *b)
{
    int i;

    if (a->ndims != b->ndims)
        return 0;

    for (i = 0; i < a->ndims; i++) {
        if (a->dims[i]->size != b->dims[i]->size)
            return 0;
    }
    return 1;
}

static const char *type_str(SDSType type)
{
    switch (type) {
    case SDS_BYTE:
    case SDS_SHORT:
    case SDS_INT:
        return "integer";
    case SDS_FLOAT:
        return "real*4";
    case SDS_DOUBLE:
        return "real*8";
    case SDS_STRING:
        return "character(%u)";
    default:
        abort();
    }
    return NULL;
}

static void print_var_decl(FILE *fout, SDSVarInfo *last, SDSVarInfo *vi)
{
    int i;

    if (last && last->type == vi->type && same_var_dims(last, vi)) {
        fprintf(fout, ", %s", vi->name);
    } else {
        if (last) fputs("\n", fout);
        fprintf(fout, "%s, dimension(", type_str(vi->type));
        for (i = vi->ndims - 1; i >= 0; i--) {
            fprintf(fout, "%u", (unsigned)vi->dims[i]->size);
            if (i > 0) fputs(",", fout);
        }
        fprintf(fout, ") :: %s", vi->name);
    }
}

static void print_att_decl(FILE *fout, const char *varname,
                           SDSAttInfo *last, SDSAttInfo *ai)
{
    if (last && last->count == ai->count) {
        fputs(", ", fout);
    } else {
        if (last) fputs("\n", fout);
        fputs(type_str(ai->type), fout);
        if (ai->count < 2) {
            fputs(" :: ", fout);
        } else {
            fprintf(fout, ", dimension(%u) :: ", (unsigned)ai->count);
        }
    }
    fprintf(fout, "%s_%s", varname, ai->name);
}

static void print_att_type_decl(FILE *fout, const char *varname,
                                SDSAttInfo *atts, SDSType type)
{
    SDSAttInfo *last = NULL, *ai;
    for (ai = atts; ai != NULL; ai = ai->next) {
        if (type == ai->type || (type == SDS_NO_TYPE &&
                                 ai->type == SDS_BYTE ||
                                 ai->type == SDS_SHORT ||
                                 ai->type == SDS_INT)) {
            print_att_decl(fout, varname, last, ai);
            last = ai;
        }
    }
    if (last) fputs("\n", fout);
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
                fprintf(fout, ", %s_%s", varname, ai->name);
            } else {
                if (last) fputs("\n", fout);
                fprintf(fout, "character(%u) :: %s_%s",
                        (unsigned)ai->count, varname, ai->name);
            }
            last = ai;
        }
    }
    if (last) fputs("\n", fout);

    print_att_type_decl(fout, varname, atts, SDS_NO_TYPE); /* integrals */
    print_att_type_decl(fout, varname, atts, SDS_FLOAT);
    print_att_type_decl(fout, varname, atts, SDS_DOUBLE);

    fputs("\n", fout);

    /* read attributes */
    for (ai = atts; ai != NULL; ai = ai->next) {
        fprintf(fout, "call checknc( nf90_get_att(ncid, %s, \"%s\", %s_%s) )\n",
                var_id, ai->name, varname, ai->name);
    }
    fputs("\n", fout);

    free(var_id);
}

void generate_f90_code(FILE *fout, SDSInfo *sds, int generate_att)
{
    SDSDimInfo *di;
    SDSVarInfo *vi;
    int w;

    fputs("use netcdf\n\n", fout);

    fputs("integer :: ncid", fout);

    /* dimension id vars */
    w = MAX_WIDTH;
    for (di = sds->dims; di != NULL; di = di->next) {
        w += strlen(di->name) + 8;
        if (w >= MAX_WIDTH) {
            fputs("\ninteger :: ", fout);
            w = strlen(di->name) + 11 + 8;
        }
        fprintf(fout, "%s_dimid", di->name);
        if (di->next && w + strlen(di->next->name) + 8 < MAX_WIDTH)
            fputs(", ", fout);
    }

    if (sds->vars) {
        SDSVarInfo *last;

        sds->vars = sort_vars(sds->vars);

        /* variable id vars */
        w = MAX_WIDTH;
        for (vi = sds->vars; vi != NULL; vi = vi->next) {
            w += strlen(vi->name) + 5;
            if (w >= MAX_WIDTH) {
                fputs("\ninteger :: ", fout);
                w = strlen(vi->name) + 11 + 5;
            }
            fprintf(fout, "%s_id", vi->name);
            if (vi->next && w + strlen(vi->next->name) + 5 < MAX_WIDTH)
                fputs(", ", fout);
        }
        fputs("\n", fout);

        /* variable data vars */
        last = NULL;
        for (vi = sds->vars; vi != NULL; vi = vi->next) {
            print_var_decl(fout, last, vi);
            last = vi;
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
            if (vi->atts) {
                vi->atts = sort_attributes(vi->atts);
                generate_f90_var_att(fout, vi->name, vi->atts);
            }
        }
    }

    fputs("! close the file\n", fout);
    fputs("call checknc( nf90_close(ncid) )\n", fout);
    fputs("\n", fout);

    fputs(CHECK_FUNCTION, fout);
}
