#include "sds.h"
#include <stdio.h>

struct Context {
    FILE *fout;
    const char *name;
    int i, num, w, size;
};

#define MAX_WIDTH (72)

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

static void dim_varname(char *name, void *value, void *data)
{
    struct Context *ctx = data;
    if (ctx->i < ctx->num) {
        fprintf(ctx->fout, "%s_dimid, ", name);
        ctx->i++;
    } else {
        fprintf(ctx->fout, "%s_dimid", name);
    }
}

static void var_varname(char *name, void *value, void *data)
{
    struct Context *ctx = data;
    if (ctx->i < ctx->num) {
        fprintf(ctx->fout, "%s_id, ", name);
        ctx->i++;
    } else {
        fprintf(ctx->fout, "%s_id", name);
    }
}

static void global_att_max_size(char *name, void *value, void *data)
{
    struct Context *ctx = data;
    SDSAttInfo *att = value;

    if (att->bytes > ctx->size)
        ctx->size = att->bytes;
}

static void global_att_var(char *name, void *value, void *data)
{
    struct Context *ctx = data;
    if (ctx->w >= MAX_WIDTH) {
        ctx->w = 0;
        fprintf(ctx->fout, "character(%i) :: ", ctx->size);
    }

    if (ctx->i < ctx->num) {
        fprintf(ctx->fout, "%s, ", name);
        ctx->i++;
    } else {
        fprintf(ctx->fout, "%s", name);
    }

    ctx->w++;
    if (ctx->w >= MAX_WIDTH)
        fputs("\n", ctx->fout);
}

static void varatt_varname(char *name, void *value, void *data)
{
    struct Context *ctx = data;

    if (ctx->i < ctx->num) {
        fprintf(ctx->fout, "%s_%s, ", ctx->name, name);
        ctx->i++;
    } else {
        fprintf(ctx->fout, "%s_%s", ctx->name, name);
    }
}

static void var_att_vars(char *name, void *value, void *data)
{
    SDSVarInfo *var = value;
    FILE *fout = data;
    struct Context ctx;
    ctx.fout = fout;
    ctx.name = name;
    ctx.i = 1;
    ctx.num = var->atts->n_nodes;
    fputs("character() :: ", fout);
    skiplist_each(var->atts, varatt_varname, &ctx);
    fputs("\n", fout);
}

static void read_global_att(char *name, void *value, void *data)
{
    FILE *fout = data;
    fprintf(fout, "call checknc( nf90_get_att(ncid, NF90_GLOBAL, \"%s\", %s) )\n",
            name, name);
}

static void read_var_att(char *name, void *value, void *data)
{
    struct Context *ctx = data;
    fprintf(ctx->fout, "call checknc( nf90_get_att(ncid, %s_id, \"%s\", %s_%s) )\n",
            ctx->name, name, ctx->name, name);
}

static void read_var_atts(char *name, void *value, void *data)
{
    SDSVarInfo *var = value;
    FILE *fout = data;
    struct Context ctx;
    ctx.fout = fout;
    ctx.name = name;
    skiplist_each(var->atts, read_var_att, &ctx);
    fputs("\n", fout);
}

static void read_dimension(char *name, void *value, void *data)
{
    FILE *fout = data;
    fprintf(fout, "call checknc( nf90_inq_dimid(ncid, \"%s\", %s_dimid) )\n", name, name);
}

static void read_varid(char *name, void *value, void *data)
{
    FILE *fout = data;
    fprintf(fout, "call checknc( nf90_inq_varid(ncid, \"%s\", %s_id) )\n", name, name);
}

static void read_var(char *name, void *value, void *data)
{
    FILE *fout = data;
    fprintf(fout, "call checknc( nf90_get_var(ncid, %s_id, %s) )\n", name, name);
}

void generate_f90_code(FILE *fout, SDSInfo *sds, int generate_att)
{
    struct Context ctx;
    ctx.fout = fout;

    fputs("use netcdf\n\n", fout);

    fputs("integer :: ncid\n", fout);

    /* dimension id vars */
    fputs("integer :: ", fout);
    ctx.num = sds->dims->n_nodes;
    ctx.i = 1;
    skiplist_each(sds->dims, dim_varname, &ctx);
    fputs("\n", fout);

    /* variable id vars */
    if (sds->vars->n_nodes > 0) {
        fputs("integer :: ", fout);
        ctx.num = sds->vars->n_nodes;
        ctx.i = 1;
        skiplist_each(sds->vars, var_varname, &ctx);
        fputs("\n", fout);
    }

    fputs("\n", fout);

    fputs("! open file\n", fout);
    fprintf(fout, "call checknc( nf90_open(\"%s\", NF90_NOWRITE, ncid) )\n", sds->path);
    fputs("\n", fout);

    fputs("! read dimensions\n", fout);
    skiplist_each(sds->dims, read_dimension, fout);
    fputs("\n", fout);

    fputs("! get variable IDs\n", fout);
    skiplist_each(sds->vars, read_varid, fout);
    fputs("\n", fout);

    fputs("! read var data\n", fout);
    skiplist_each(sds->vars, read_var, fout);
    fputs("\n", fout);

    /* read last dim of var data in a loop */

    fputs("! close the file\n", fout);
    fputs("call checknc( nf90_close(ncid) )\n", fout);
    fputs("\n", fout);

    fputs(CHECK_FUNCTION, fout);

    if (generate_att) {
        fputs("\n\n! attribute variables\n", fout);
        ctx.i = 1;
        ctx.num = sds->gatts->n_nodes;
        ctx.w = MAX_WIDTH;
        ctx.size = 0;
        skiplist_each(sds->gatts, global_att_max_size, &ctx);
        skiplist_each(sds->gatts, global_att_var, &ctx);
        fputs("\n", fout);

        skiplist_each(sds->vars, var_att_vars, fout);
        fputs("\n\n", fout);

        if (sds->gatts->n_nodes > 0) {
            fputs("! read global attributes\n", fout);
            skiplist_each(sds->gatts, read_global_att, fout);
            fputs("\n", fout);
        }

        fputs("! read var attributes\n", fout);
        skiplist_each(sds->vars, read_var_atts, fout);
        fputs("\n", fout);
    }
}
