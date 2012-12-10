#include "sds.h"
#include <string.h>

size_t sds_type_size(SDSType t)
{
    switch (t) {
    case SDS_NO_TYPE: return 0;
    case SDS_I8:
    case SDS_U8:     return 1;
    case SDS_I16:
    case SDS_U16:    return 2;
    case SDS_I32:
    case SDS_U32:
    case SDS_FLOAT:  return 4;
    case SDS_DOUBLE: return 8;
    case SDS_STRING: return 0;
    default: break;
    }
    abort();
    return 0;
}

SDSDimInfo *sds_dim_by_name(SDSInfo *sds, const char *name)
{
    SDSDimInfo *dim = sds->dims;
    while (dim) {
        if (!strcmp(name, dim->name))
            break;
        dim = dim->next;
    }
    return dim;
}

SDSVarInfo *sds_var_by_name(SDSInfo *sds, const char *name)
{
    SDSVarInfo *var = sds->vars;
    while (var) {
        if (!strcmp(name, var->name))
            break;
        var = var->next;
    }
    return var;
}

size_t sds_var_size(SDSVarInfo *var)
{
    size_t size = sds_type_size(var->type);
    for (int i = 0; i < var->ndims; i++) {
        size *= var->dims[i]->size;
    }
    return size;
}

void *sds_read_var(SDSInfo *sds, SDSVarInfo *var)
{
    return (sds->funcs->var_read)(sds, var);
}


static void free_atts(SDSAttInfo *att)
{
    if (att) {
        SDSAttInfo *next = att->next;
        free(att->name);
        free(att->data.v);
        free(att);
        free_atts(next);
    }
}

static void free_dims(SDSDimInfo *dim)
{
    if (dim) {
        SDSDimInfo *next = dim->next;
        free(dim->name);
        free(dim);
        free_dims(next);
    }
}

static void free_vars(SDSVarInfo *var)
{
    if (var) {
        SDSVarInfo *next = var->next;
        free(var->name);
        free_atts(var->atts);
        free(var->dims);
        free(var);
        free_vars(next);
    }
}

void sds_close(SDSInfo *sds)
{
    sds->funcs->close(sds);

    free_atts(sds->gatts);
    free_dims(sds->dims);
    free_vars(sds->vars);

    free(sds->path);
    free(sds);
}
