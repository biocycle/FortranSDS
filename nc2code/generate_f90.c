#include "sds.h"
#include "util.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

#define MAX_WIDTH (80 - 4)

static const char CHECK_FUNCTION[] =
"    subroutine checknc(status)\n"
"        integer, intent(in) :: status\n"
"\n"
"        if (status /= NF90_NOERR) then\n"
"            print *, nf90_strerror(status)\n"
"            stop\n"
"        end if\n"
"    end subroutine checknc\n"
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

static void print_var_decl(FILE *fout, SDSVarInfo *prev, SDSVarInfo *vi)
{
    int i;

    if (prev && prev->type == vi->type && same_var_dims(prev, vi)) {
        fprintf(fout, ", %s", vi->name);
    } else {
        if (prev) fputs("\n", fout);
        fprintf(fout, "    %s, dimension(", type_str(vi->type));
        for (i = vi->ndims - 1; i >= 0; i--) {
            fprintf(fout, "%u", (unsigned)vi->dims[i]->size);
            if (i > 0) fputs(",", fout);
        }
        fprintf(fout, ") :: %s", vi->name);
    }
}

static void print_att_decl(FILE *fout, const char *varname,
                           SDSAttInfo *prev, SDSAttInfo *ai)
{
    if (prev && prev->count == ai->count) {
        fputs(", ", fout);
    } else {
        if (prev) fputs("\n", fout);
        fprintf(fout, "    %s", type_str(ai->type));
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
    SDSAttInfo *prev = NULL, *ai;
    for (ai = atts; ai != NULL; ai = ai->next) {
        if (type == ai->type || (type == SDS_NO_TYPE &&
                                 ai->type == SDS_BYTE ||
                                 ai->type == SDS_SHORT ||
                                 ai->type == SDS_INT)) {
            print_att_decl(fout, varname, prev, ai);
            prev = ai;
        }
    }
    if (prev) fputs("\n", fout);
}

static void generate_f90_var_att(FILE *fout, const char *varname,
                                 SDSAttInfo *atts)
{
    SDSAttInfo *ai, *prev;
    char *var_id;

    if (!varname) {
        varname = "global";
        var_id = NEWA(char, 12);
        strcpy(var_id, "NF90_GLOBAL");
    } else {
        var_id = NEWA(char, strlen(varname) + 4);
        sprintf(var_id, "%s_id", varname);
    }

    fprintf(fout, "    ! %s attributes\n", varname);

    /* att variables */
    prev = NULL;
    for (ai = atts; ai != NULL; ai = ai->next) {
        if (ai->type == SDS_STRING) {
            if (prev && prev->count == ai->count) {
                fprintf(fout, ", %s_%s", varname, ai->name);
            } else {
                if (prev) fputs("\n", fout);
                fprintf(fout, "    character(%u) :: %s_%s",
                        (unsigned)ai->count, varname, ai->name);
            }
            prev = ai;
        }
    }
    if (prev) fputs("\n", fout);

    print_att_type_decl(fout, varname, atts, SDS_NO_TYPE); /* integrals */
    print_att_type_decl(fout, varname, atts, SDS_FLOAT);
    print_att_type_decl(fout, varname, atts, SDS_DOUBLE);

    fputs("\n", fout);

    /* read attributes */
    for (ai = atts; ai != NULL; ai = ai->next) {
        fprintf(fout, "    call checknc( nf90_get_att(ncid, %s, \"%s\", %s_%s) )\n",
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

    fputs("program read_netcdf\n", fout);
    fputs("    use netcdf\n", fout);
    fputs("    implicit none\n\n", fout);   

    fputs("    character(512) :: filename\n", fout);
    fputs("    integer :: ncid, i", fout);
    fprintf("    integer, dimension(%u) :: start, count\n", list_count(sds->dims));

    /* dimension id vars */
    w = MAX_WIDTH;
    for (di = sds->dims; di != NULL; di = di->next) {
        w += strlen(di->name) + 8;
        if (w >= MAX_WIDTH) {
            fputs("\n    integer :: ", fout);
            w = strlen(di->name) + 11 + 8;
        }
        fprintf(fout, "%s_dimid", di->name);
        if (di->next && w + strlen(di->next->name) + 8 < MAX_WIDTH)
            fputs(", ", fout);
    }

    /* dimension size vars */
    w = MAX_WIDTH;
    for (di = sds->dims; di != NULL; di = di->next) {
        w += strlen(di->name) + 7;
        if (w >= MAX_WIDTH) {
            fputs("\n    integer :: ", fout);
            w = strlen(di->name) + 11 + 7;
        }
        fprintf(fout, "%s_size", di->name);
        if (di->next && w + strlen(di->next->name) + 8 < MAX_WIDTH)
            fputs(", ", fout);
    }

    if (sds->vars) {
        SDSVarInfo *prev;

        sds->vars = sort_vars(sds->vars);

        /* variable id vars */
        w = MAX_WIDTH;
        for (vi = sds->vars; vi != NULL; vi = vi->next) {
            w += strlen(vi->name) + 5;
            if (w >= MAX_WIDTH) {
                fputs("\n    integer :: ", fout);
                w = strlen(vi->name) + 11 + 5;
            }
            fprintf(fout, "%s_id", vi->name);
            if (vi->next && w + strlen(vi->next->name) + 5 < MAX_WIDTH)
                fputs(", ", fout);
        }
        fputs("\n", fout);

        /* variable data vars */
        prev = NULL;
        for (vi = sds->vars; vi != NULL; vi = vi->next) {
            print_var_decl(fout, prev, vi);
            prev = vi;
        }
        fputs("\n", fout);
    }
    fputs("\n", fout);

    fputs("    ! open file\n", fout);
    fprintf(fout, "    filename = \"%s\"\n", sds->path);
    fprintf(fout, "    call checknc( nf90_open(filename, NF90_NOWRITE, ncid) )\n");
    fputs("\n", fout);

    fputs("    ! read dimensions\n", fout);
    for (di = sds->dims; di != NULL; di = di->next) {
        fprintf(fout, "    call checknc( nf90_inq_dimid(ncid, \"%s\", %s_dimid) )\n",
                di->name, di->name);
    }
    fputs("\n", fout);

    fputs("    ! read dimension sizes\n", fout);
    for (di = sds->dims; di != NULL; di = di->next) {
        fprintf(fout, "    call checknc( nf90_inquire_dimension(ncid, %s_dimid, len = %s_size) )\n",
                di->name, di->name);
    }
    fputs("\n", fout);

    fputs("    ! get variable IDs\n", fout);
    for (vi = sds->vars; vi != NULL; vi = vi->next) {
        fprintf(fout, "    call checknc( nf90_inq_varid(ncid, \"%s\", %s_id) )\n",
                vi->name, vi->name);
    }
    fputs("\n", fout);

    fputs("    ! read var data\n", fout);
    for (vi = sds->vars; vi != NULL; vi = vi->next) {
        fprintf(fout, "    call checknc( nf90_get_var(ncid, %s_id, %s) )\n",
                vi->name, vi->name);
    }
    fputs("\n", fout);

    fputs("    ! read var data in a loop\n", fout);
    for (vi = sds->vars; vi != NULL; vi = vi->next) {
        int i;

        fprintf(fout, "    start(%s%u) = 1\n", (vi->ndims > 1) ? "1:" : "",
                vi->ndims);
        fprintf(fout, "    count(%s%u) = (/", (vi->ndims > 1) ? "1:" : "",
                vi->ndims);
        for (i = vi->ndims - 1; i > 0; i--) {
            fprintf(fout, "%s_size, ", vi->dims[i]->name);
        }
        fputs("1/)\n", fout);

        fprintf(fout, "    do i = 1, %s_size\n", (unsigned)vi->dims[0]->name);
        fprintf(fout, "        start(%u) = i\n", (unsigned)vi->ndims);
        fprintf(fout, "        call checknc( nf90_get_var(ncid, %s_id, %s(",
                vi->name, vi->name);
        for (i = vi->ndims - 1; i > 0; i--)
            fputs(":,", fout);
        fputs("i), start, count) )\n", fout);
        fputs("    end do\n\n", fout);
    }

    if (generate_att) {
        if (sds->gatts) {
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

    fputs("    ! close the file\n", fout);
    fputs("    call checknc( nf90_close(ncid) )\n", fout);

    fputs("\ncontains\n\n", fout);
    fputs(CHECK_FUNCTION, fout);
    fputs("\nend program read_netcdf\n", fout);
}
