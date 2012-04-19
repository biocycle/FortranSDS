#include "sds.h"
#include "util.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

extern const char *f90_type_str(SDSType type);

void print_f90_dim_colons(FILE *fout, int ndims)
{
    int i;

    fputs("dimension(", fout);
    for (i = ndims - 1; i >= 0; i--) {
        fputs(":", fout);
        if (i > 0) fputs(",", fout);
    }
}

void generate_simple_f90_code(FILE *fout, SDSInfo *sds, int generate_att)
{
    SDSVarInfo *vi;

    fputs("! This enables source file:line number debugging info\n"
          "! Needs special compilation instructions\n", fout);
    fputs("#include \"simple_sds.inc\"\n\n", fout);

    /* Just read the variable(s) */
    fputs("! Use one of these if you just need one variable from the file\n",
          fout);
    fprintf(fout, "character(512), parameter :: filename = \"%s\"\n",
            sds->path);
    fputs("\n", fout);
    for (vi = sds->vars; vi != NULL; vi = vi->next) {
        fprintf(fout, "%s, pointer, dimension(", f90_type_str(vi->type));
        print_f90_dim_colons(fout, vi->ndims);
        fprintf(fout, ") :: %s\n", vi->name);
        fprintf(fout, "call snc_open_var(filename, \"%s\", %s)\n\n",
                vi->name, vi->name);
    }
    fputs("\n", fout);

    /* Open file and read variable(s) */
    fputs("! Use this code if you need to read multiple whole variables "
          "from the file\n", fout);
    fputs("\n", fout);
    fputs("type(SNCFile) :: file\n", fout);
    fputs("type(SNCVar) :: var\n", fout);
    fputs("integer :: i\n", fout);
    for (vi = sds->vars; vi != NULL; vi = vi->next) {
        fprintf(fout, "%s, pointer, ", f90_type_str(vi->type));
        print_f90_dim_colons(fout, vi->ndims);
        fprintf(fout, ") :: %s\n" , vi->name);
    }
    fputs("\n", fout);

    fputs("file = snc_open(filename)\n", fout);
    fputs("\n", fout);

    for (vi = sds->vars; vi != NULL; vi = vi->next) {
        fprintf(fout, "var = snc_inq_var(file, \"%s\", units)\n", vi->name);
        fprintf(fout, "call snc_read(file, var, %s)\n", vi->name);
        fputs("\n", fout);
    }

    fputs("call snc_close(file)\n", fout);


    /* Read everything by timestep */
}
