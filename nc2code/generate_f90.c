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

void generate_f90_code(FILE *fout, SDSInfo *sds, int generate_att)
{
    SDSAttInfo *ai;
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
        int w = MAX_WIDTH;
        int size = 0;

        fputs("\n\n! attribute variables\n", fout);
        for (ai = sds->gatts; ai != NULL; ai = ai->next) {
            int asize = ai->bytes * ai->count;
            if (asize > size) size = asize;
        }
        for (ai = sds->gatts; ai != NULL; ai = ai->next) {
            if (w >= MAX_WIDTH) {
                w = 0;
                fprintf(fout, "character(%i) :: ", size);
            }

            fprintf(fout, "%s", ai->name);
            if (ai->next) fputs(", ", fout);

            w++;
            if (w >= MAX_WIDTH)
                fputs("\n", fout);
        }
        fputs("\n", fout);

        for (vi = sds->vars; vi != NULL; vi = vi->next) {
            size = 0;
            for (ai = vi->atts; ai != NULL; ai = ai->next) {
                int asize = ai->bytes * ai->count;
                if (asize > size) size = asize;
            }
            if (vi->atts)
                fprintf(fout, "character(%i) :: ", size);
            for (ai = vi->atts; ai != NULL; ai = ai->next) {
                fprintf(fout, "%s_%s", vi->name, ai->name);
                if (ai->next) fputs(", ", fout);
            }
            if (vi->atts)
                fputs("\n", fout);
        }
        fputs("\n\n", fout);

        if (sds->gatts) {
            fputs("! read global attributes\n", fout);
            for (ai = sds->gatts; ai != NULL; ai = ai->next) {
                fprintf(fout, "call checknc( nf90_get_att(ncid, NF90_GLOBAL, \"%s\", %s) )\n",
                        ai->name, ai->name);
            }
            fputs("\n", fout);
        }

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
}
