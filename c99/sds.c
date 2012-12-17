#include "sds.h"
#include "util.h"
#include <stdio.h>
#include <string.h>

#ifdef HAVE_HDF4
#  include <hdf.h>    
#endif

static int sds_magic(const char *path)
{
    int ret = 0;

    FILE *fin = fopen(path, "r");
    if (!fin)
        return 0;
    char buf[4];
    if (fread(buf, 1, sizeof(buf), fin) < sizeof(buf))
        goto done;

    if (buf[0] == 'C' && buf[1] == 'D' && buf[2] == 'F' &&
        (buf[3] == 0x1 || buf[3] == 0x2)) {
        ret = 3; // NetCDF classic or 64-bit offset
    } else if (buf[0] == 137 && buf[1] == 'H' && buf[2] == 'D' && buf[3] == 'F') {
        if (fread(buf, 1, sizeof(buf), fin) < sizeof(buf))
            goto done;
        if (buf[0] != '\r' || buf[1] != '\n' || buf[2] != ' ' || buf[3] != '\n')
            goto done;
        size_t len = strlen(path);
        if (!strcmp(path+len-4, ".hdf") ||
            !strcmp(path+len-3, ".h5") ||
            !strcmp(path+len-5, ".hdf5") ||
            !strcmp(path+len-4, ".he5"))
            ret = 5; // just an HDF5 file 'cuz extension sez so
        else
            ret = 4; // assume NetCDF v.4 using HDF5 file format
    }
 done:
    fclose(fin);
    return ret;
}

SDSFileType sds_file_type(const char *path)
{
#ifdef HAVE_HDF4
    if (Hishdf(path))
        return SDS_HDF4_FILE;
#endif
    switch (sds_magic(path)) {
    case 3: return SDS_NC3_FILE;
    case 4: return SDS_NC4_FILE;
    case 5: return SDS_HDF5_FILE;
    default: break;
    }
    return SDS_UNKNOWN_FILE;
}

SDSInfo *open_any_sds(const char *path)
{
    switch (sds_file_type(path)) {

    case SDS_NC3_FILE:
#ifdef HAVE_NETCDF4
    case SDS_NC4_FILE:
#endif
        return open_nc_sds(path);

#ifdef HAVE_HDF4
    case SDS_HDF4_FILE:
        return open_h4_sds(path);
#endif

    default:
        break;
    }
    return NULL;
}

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

void *sds_read_var_by_name(SDSInfo *sds, const char *name, void **bufp)
{
    SDSVarInfo *var = sds_var_by_name(sds, name);
    if (!var)
        return NULL;
    return sds_read(sds, var, bufp);
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
        index[i] = -1; // read all of this dimension
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
        index[i] = -1; // read all of this dimension
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
