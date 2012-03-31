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

    /* variable id vars */
    if (sds->vars) {
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
