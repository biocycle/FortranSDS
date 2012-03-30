#include "sds.h"
#include <stdio.h>

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

static void print_dimension(FILE *fout, int count)
{
    if (count <= 0) {
        fputs(" :: ", fout);
    } else {
        fputs(", dimension(:", fout);
        for (; count > 1; count--)
            fputs(",:", fout);
        fputs(") :: ", fout);
    }
}

static void print_att_type(FILE *fout, SDSAttInfo *ai)
{
    switch (ai->type) {
    case SDS_NO_TYPE:
        abort();
    case SDS_BYTE:
    case SDS_SHORT:
    case SDS_INT:
        fputs("integer", fout);
        print_dimension(fout, ai->count);
        break;
    case SDS_FLOAT:
        fputs("real*4", fout);
        print_dimension(fout, ai->count);
        break;
    case SDS_DOUBLE:
        fputs("real*8", fout);
        print_dimension(fout, ai->count);
        break;
    case SDS_STRING:
        fprintf(fout, "character(%i) :: ", ai->count);
        break;
    }
}

static void generate_f90_att(FILE *fout, SDSInfo *sds)
{
    SDSAttInfo *ai;
    SDSVarInfo *vi;
    int w = MAX_WIDTH;
    int size = 0;

    fputs("\n\n! attribute variables\n", fout);
    /* find max attribute size */
    for (ai = sds->gatts; ai != NULL; ai = ai->next) {
        int asize = ai->bytes * ai->count;
        if (asize > size) size = asize;
    }
    /* print global attribute variables */
    for (ai = sds->gatts; ai != NULL; ai = ai->next) {
        /*if (w >= MAX_WIDTH) {*/
            w = 0;
            print_att_type(fout, ai);
            /*}*/

        fprintf(fout, "%s\n", ai->name);
        /*if (ai->next) fputs(", ", fout);*/

        w++;
        if (w >= MAX_WIDTH)
            fputs("\n", fout);
    }
    fputs("\n", fout);

    /* print var attribute variables */
    for (vi = sds->vars; vi != NULL; vi = vi->next) {
        size = 0;
        for (ai = vi->atts; ai != NULL; ai = ai->next) {
            int asize = ai->bytes * ai->count;
            if (asize > size) size = asize;
        }
        /*if (vi->atts)
            fprintf(fout, "character(%i) :: ", size);*/
        for (ai = vi->atts; ai != NULL; ai = ai->next) {
            print_att_type(fout, ai);
            fprintf(fout, "%s_%s\n", vi->name, ai->name);
            /*if (ai->next) fputs(", ", fout);*/
        }
        if (vi->atts)
            fputs("\n", fout);
    }
    fputs("\n\n", fout);

    /* code to read global attributes */
    if (sds->gatts) {
        fputs("! read global attributes\n", fout);
        for (ai = sds->gatts; ai != NULL; ai = ai->next) {
            fprintf(fout, "call checknc( nf90_get_att(ncid, NF90_GLOBAL, \"%s\", %s) )\n",
                    ai->name, ai->name);
        }
        fputs("\n", fout);
    }

    /* code to read var attributes */
    fputs("! read var attributes\n", fout);
    for (vi = sds->vars; vi != NULL; vi = vi->next) {
        for (ai = vi->atts; ai != NULL; ai = ai->next) {
            fprintf(fout, "call checknc( nf90_get_att(ncid, %s_id, \"%s\", %s_%s) \
)\n",
                    vi->name, ai->name, vi->name, ai->name);
        }
        fputs("\n", fout);
    }

    fputs("\n", fout);
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

    /* read last dim of var data in a loop */

    fputs("! close the file\n", fout);
    fputs("call checknc( nf90_close(ncid) )\n", fout);
    fputs("\n", fout);

    fputs(CHECK_FUNCTION, fout);

    if (generate_att) {
        generate_f90_att(fout, sds);
    }
}
