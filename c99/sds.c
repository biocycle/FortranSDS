#include "sds.h"
#include "util.h"
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

/* Reads all of the given variable.
 * bufp: an opaque pointer to a buffer structure used to manage memory to be
 *       read into and other housekeeping.  In your code, create a void pointer
 *       set to NULL, then pass in the address of that variable.  Keep passing
 *       in that same void-pointer-pointer to re-use the buffer.  When you are
 *       done with the buffer, use sds_buffer_free to free it from memory.
 */
void *sds_read(SDSInfo *sds, SDSVarInfo *var, void **bufp)
{
    int *index = ALLOCA(int, var->ndims);
    for (int i = 0; i < var->ndims; i++) {
        index[i] = var->dims[i]->size;
    }
    return (sds->funcs->var_readv)(sds, var, bufp, index);
}

/* Read all of one timestep (i.e. the first dimension) from the given variable.
 * Returns a pointer to malloc()'d data managed by bufp.
 * bufp: an opaque pointer to a buffer structure used to manage memory to be
 *       read into and other housekeeping.  In your code, create a void pointer
 *       set to NULL, then pass in the address of that variable.  Keep passing
 *       in that same void-pointer-pointer to re-use the buffer.  When you are
 *       done with the buffer, use sds_buffer_free to free it from memory.
 */
void *sds_timestep(SDSInfo *sds, SDSVarInfo *var, void **bufp, int tstep)
{
    int *index = ALLOCA(int, var->ndims);
    index[0] = tstep;
    for (int i = 1; i < var->ndims; i++) {
        index[i] = var->dims[i]->size;
    }
    return (sds->funcs->var_readv)(sds, var, bufp, index);
}

/* Read from the given variable, subsetting based on the index array.
 * bufp: an opaque pointer to a buffer structure used to manage memory to be
 *       read into and other housekeeping.  In your code, create a void pointer
 *       set to NULL, then pass in the address of that variable.  Keep passing
 *       in that same void-pointer-pointer to re-use the buffer.  When you are
 *       done with the buffer, use sds_buffer_free to free it from memory.
 * index: an array of var->ndims elements used to index into the variable's
 *        data.  Each element >= 0 indicates that only that index of the
 *        corresponding dimension will be read.  Otherwise, the whole length
 *        of that dimension will be read.
 */
void *sds_readv(SDSInfo *sds, SDSVarInfo *var, void **bufp, int *index)
{
    return (sds->funcs->var_readv)(sds, var, bufp, index);
}

struct GenericBuffer {
    void (*free_func)(void *);
};

void sds_buffer_free(void *buf)
{
    struct GenericBuffer *gb = (struct GenericBuffer *)buf;
    (gb->free_func)(buf);
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
