#include "sds.h"
#include <string.h>

size_t sds_type_size(SDSType t)
{
    switch (t) {
    case SDS_NO_TYPE: return 0;
    case SDS_BYTE:    return 1;
    case SDS_SHORT:   return 2;
    case SDS_INT:     return 4;
    case SDS_FLOAT:   return 4;
    case SDS_DOUBLE:  return 8;
    case SDS_STRING:  return 0;
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

void *sds_read_var(SDSInfo *sds, SDSVarInfo *var)
{
    return (sds->funcs->var_read)(sds, var);
}
