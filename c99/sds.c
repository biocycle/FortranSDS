#include "sds.h"
#include "util.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

/* Copies an SDSInfo struct, its attributes, dimensions and variables. The copy
 * is deep and typeless (i.e. not tied to  NetCDF, HDF, etc.), so a new SDS
 * file of any type can be created from it.
 *
 * The returned data structure still needs to be freed with sds_close() even
 * if you don't write it to a file.
 */
SDSInfo *sds_generic_copy(SDSInfo *sds)
{
    SDSAttInfo *gatts = (sds->gatts == NULL) ? NULL :
        sds_atts_generic_copy(sds->gatts);
    SDSDimInfo *dims = (sds->dims == NULL) ? NULL :
        sds_dims_generic_copy(sds->dims);
    SDSVarInfo *vars = (sds->vars == NULL) ? NULL :
        sds_vars_generic_copy(sds->vars, dims);
    return create_sds(gatts, dims, vars);
}

/* Copies a list of attributes, usually useful for converting from one
 * SDS format to another.
 */
SDSAttInfo *sds_atts_generic_copy(SDSAttInfo *att)
{
    SDSAttInfo *next = (att->next == NULL) ? NULL : sds_atts_generic_copy(att->next);
    return sds_create_att(next, att->name, att->type, att->count, att->data.v);
}

/* Copies a list of dimensions, usually useful for converting from one
 * SDS format to another.
 */
SDSDimInfo *sds_dims_generic_copy(SDSDimInfo *dim)
{
    SDSDimInfo *next = (dim->next == NULL) ? NULL : sds_dims_generic_copy(dim->next);
    return sds_create_dim(next, dim->name, dim->size, dim->isunlim);
}

/* Copies a list of variables, usually useful for converting from one SDS
 * format to another.
 *
 * newdims - the dimension list for the same SDSInfo that this variable
 *           list will belong to.  The original dimensions are matched by
 *           name to the corresponding dimensions in the new list.  If
 *           the dimension name is not found, an error will be printed
 *           to the screen and your program will crash.
 */
SDSVarInfo *sds_vars_generic_copy(SDSVarInfo *var, SDSDimInfo *newdims)
{
    SDSVarInfo *next = (var->next == NULL) ? NULL : sds_vars_generic_copy(var->next, newdims);
    // match up var->dims to those in newdims
    SDSDimInfo **dims = alloca(sizeof(SDSDimInfo *) * var->ndims);
    for (int i = 0; i < var->ndims; i++) {
        dims[i] = sds_dim_by_name(newdims, var->dims[i]->name);
        if (!dims[i]) {
            fprintf(stderr, "could not find new dimension named '%s' when copying var '%s'\n",
                    var->dims[i]->name, var->name);
            abort();
        }
    }
    SDSAttInfo *atts = (var->atts == NULL) ? NULL : sds_atts_generic_copy(var->atts);
    return sds_create_var(next, var->name, var->type, var->iscoord,
                          atts, var->ndims, dims);
}

/* Create a new SDSInfo with the given global attributes, dimensions and
 * variables.  These members should be created for this SDSInfo and not
 * belong to another one since they will be freed in the call to sds_close()
 * (which you should use whether you write to a file or not).
 */
SDSInfo *create_sds(SDSAttInfo *gatts, SDSDimInfo *dims, SDSVarInfo *vars)
{
    SDSInfo *sds = NEW(SDSInfo);
    sds->path = NULL;
    sds->type = SDS_UNKNOWN_FILE;
    sds->gatts = gatts;
    sds->dims = dims;
    sds->vars = vars;

    SDSDimInfo *unlim = NULL;
    while (dims) {
        if (dims->isunlim) {
            if (unlim) { // more than one unlimited dimension!
                unlim = NULL;
                break;
            }
            unlim = dims;
        }
        dims = dims->next;
    }
    sds->unlimdim = unlim;

    while (vars) {
        vars->sds = sds;
        vars = vars->next;
    }

    sds->id = -1;
    sds->funcs = NULL;
    return sds;
}

/* Creates a new SDSAttInfo with the given values.  These values are copied
 * (where applicable), so you are responsible for freeing any dynamically-
 * allocated memory passed in.  In turn, the returned struct should be freed
 * by attaching it to an SDSInfo which is freed by a call to sds_close().
 *
 * next - The next SDSAttInfo in the list, or NULL if this is the first
 *        attribute.
 */
SDSAttInfo *sds_create_att(SDSAttInfo *next, const char *name, SDSType type,
                           size_t count, const void *data)
{
    SDSAttInfo *att = NEW(SDSAttInfo);
    att->next = next;
    att->name = xstrdup(name);
    att->type = type;
    att->count = count;
    att->bytes = sds_type_size(type);
    att->data.v = malloc(att->bytes * count);
    memcpy(att->data.v, data, att->bytes * count);
    return att;
}

/* Creates a new string attribute.  This helper function makes it easier to
 * create string-type attributes, the most common kind.  See sds_create_att()
 * for more.
 */
SDSAttInfo *sds_create_stratt(SDSAttInfo *next, const char *name,
                              const char *str)
{
    return sds_create_att(next, name, SDS_STRING, strlen(str) + 1, str);
}

/* Creates a new SDSDimInfo.  The name argument is copied, so you are
 * responsible for freeing its memory (if applicable).
 *
 * next - the next SDSDimInfo in the list, or NULL if this is the first
 *        dimension.
 */
SDSDimInfo *sds_create_dim(SDSDimInfo *next, const char *name, size_t size,
                           int isunlim)
{
    SDSDimInfo *dim = NEW(SDSDimInfo);
    dim->next = next;
    dim->name = xstrdup(name);
    dim->size = size;
    dim->isunlim = isunlim;
    dim->id = -1;
    return dim;
}

/* Creates a new SDSVarInfo like sds_create_var(), but takes the dimensions
 * as a series of SDSDimInfo* arguments instead of an array you have to
 * populate.  See sds_create_var() for details.
 */
SDSVarInfo *sds_create_varv(SDSVarInfo *next, const char *name, SDSType type,
                            int iscoord, SDSAttInfo *atts, int ndims, ...)
{
    SDSDimInfo **dims = alloca(sizeof(SDSDimInfo *) * ndims);
    va_list ap;

    va_start(ap, ndims);
    for (int i = 0; i < ndims; i++) {
        dims[i] = va_arg(ap, SDSDimInfo *);
    }
    va_end(ap);

    return sds_create_var(next, name, type, iscoord, atts,
                          ndims, dims);
}

/* Creates a new SDSVarInfo with the given arguments.  The name is copied,
 * so you do not give ownership of that argument.  The atts ownership is
 * taken by the new SDSVarInfo though, so they need to be separately
 * allocated from other variables' attributes.
 *
 * next - the next SDSVarInfo in the list, or NULL if this is the first
 *        variable.
 * atts - a chain of attributes for this variable.  Not copied; must be
 *        created (malloc()d) separately for this variable.
 * dims - array of dimension pointers from left to right.  The array will
 *        be copied
 */
SDSVarInfo *sds_create_var(SDSVarInfo *next, const char *name, SDSType type,
                           int iscoord, SDSAttInfo *atts,
                           int ndims, SDSDimInfo **dims)
{
    SDSVarInfo *var = NEW(SDSVarInfo);
    var->next = next;
    var->name = xstrdup(name);
    var->type = type;
    var->iscoord = iscoord;
    var->ndims = ndims;
    var->dims = malloc(sizeof(SDSDimInfo *) * ndims);
    memcpy(var->dims, dims, sizeof(SDSDimInfo *) * ndims);
    var->atts = atts;
    var->id = -1;
    return var;
}

static int sds_magic(const char *path)
{
    int ret = 0;

    FILE *fin = fopen(path, "r");
    if (!fin)
        return 0;
    unsigned char buf[4];
    if (fread(buf, 1, sizeof(buf), fin) < sizeof(buf))
        goto done;

    if (buf[0] == 'C' && buf[1] == 'D' && buf[2] == 'F' &&
        (buf[3] == 0x1 || buf[3] == 0x2)) {
        ret = 3; // NetCDF classic or 64-bit offset
    } else if (buf[0] == 14 && buf[1] == 3 && buf[2] == 19 && buf[3] == 1) {
        ret = 1; // HDF 4
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
    switch (sds_magic(path)) {
    case 1: return SDS_HDF4_FILE;
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
    case SDS_STRING: return 1;
    default: break;
    }
    abort();
    return 0;
}

SDSAttInfo *sds_att_by_name(SDSAttInfo *att, const char *name)
{
    while (att) {
        if (!strcmp(name, att->name))
            break;
        att = att->next;
    }
    return att;
}

SDSDimInfo *sds_dim_by_name(SDSDimInfo *dim, const char *name)
{
    while (dim) {
        if (!strcmp(name, dim->name))
            break;
        dim = dim->next;
    }
    return dim;
}

SDSVarInfo *sds_var_by_name(SDSVarInfo *var, const char *name)
{
    while (var) {
        if (!strcmp(name, var->name))
            break;
        var = var->next;
    }
    return var;
}

static int name_in_list(const char *name, const char **names, int n_names)
{
    for (int i = 0; i < n_names; i++) {
        if (!strcmp(names[i], name)) {
            return 1; // found
        }
    }
    return 0; // not found
}


SDSAttInfo *sds_keep_atts(SDSAttInfo *att, const char **names, int n_names)
{
    SDSAttInfo *keep = NULL, *del = NULL;
    while (att) {
        SDSAttInfo *next = att->next;
        if (name_in_list(att->name, names, n_names)) {
            att->next = keep;
            keep = att;
        } else {
            att->next = del;
            del = att;
        }
        att = next;
    }
    sds_free_atts(del);
    return keep;
}

SDSDimInfo *sds_keep_dims(SDSDimInfo *dim, const char **names, int n_names)
{
    SDSDimInfo *keep = NULL, *del = NULL;
    while (dim) {
        SDSDimInfo *next = dim->next;
        if (name_in_list(dim->name, names, n_names)) {
            dim->next = keep;
            keep = dim;
        } else {
            dim->next = del;
            del = dim;
        }
        dim = next;
    }
    sds_free_dims(del);
    return keep;
}

SDSVarInfo *sds_keep_vars(SDSVarInfo *var, const char **names, int n_names)
{
    SDSVarInfo *keep = NULL, *del = NULL;
    while (var) {
        SDSVarInfo *next = var->next;
        if (name_in_list(var->name, names, n_names)) {
            var->next = keep;
            keep = var;
        } else {
            var->next = del;
            del = var;
        }
        var = next;
    }
    sds_free_vars(del);
    return keep;
}

SDSAttInfo *sds_delete_atts(SDSAttInfo *att, const char **names, int n_names)
{
    SDSAttInfo *keep = NULL, *del = NULL;
    while (att) {
        SDSAttInfo *next = att->next;
        if (name_in_list(att->name, names, n_names)) {
            att->next = del;
            del = att;
        } else {
            att->next = keep;
            keep = att;
        }
        att = next;
    }
    sds_free_atts(del);
    return keep;
}

SDSDimInfo *sds_delete_dims(SDSDimInfo *dim, const char **names, int n_names)
{
    SDSDimInfo *keep = NULL, *del = NULL;
    while (dim) {
        SDSDimInfo *next = dim->next;
        if (name_in_list(dim->name, names, n_names)) {
            dim->next = del;
            del = dim;
        } else {
            dim->next = keep;
            keep = dim;
        }
        dim = next;
    }
    sds_free_dims(del);
    return keep;
}

SDSVarInfo *sds_delete_vars(SDSVarInfo *var, const char **names, int n_names)
{
    SDSVarInfo *keep = NULL, *del = NULL;
    while (var) {
        SDSVarInfo *next = var->next;
        if (name_in_list(var->name, names, n_names)) {
            var->next = del;
            del = var;
        } else {
            var->next = keep;
            keep = var;
        }
        var = next;
    }
    sds_free_vars(del);
    return keep;
}

/* Calculates the number of bytes required for the variable's entire data
 * by multiplying the dimensions together.
 */
size_t sds_var_size(SDSVarInfo *var)
{
    size_t size = sds_type_size(var->type);
    for (int i = 0; i < var->ndims; i++) {
        size *= var->dims[i]->size;
    }
    return size;
}

/* Calculates the total number of elements for this variable by
 * multiplying the dimensions together.
 */
size_t sds_var_count(SDSVarInfo *var)
{
    size_t size = 1;
    for (int i = 0; i < var->ndims; i++) {
        size *= var->dims[i]->size;
    }
    return size;
}

/* Helper function to read the entire contents of the named variable.
 *
 * bufp - a pointer to an opaque data buffer holding the variable's
 *        malloc()d data and other bookkeeping information.  When you
 *        are done with the data but before you close the file,
 *        call sds_buffer_free() to free this opaque pointer.
 */
void *sds_read_var_by_name(SDSInfo *sds, const char *name, void **bufp)
{
    SDSVarInfo *var = sds_var_by_name(sds->vars, name);
    if (!var)
        return NULL;
    return sds_read(var, bufp);
}

/* Reads all of the given variable.
 * bufp: an opaque pointer to a buffer structure used to manage memory to be
 *       read into and other housekeeping.  In your code, create a void pointer
 *       set to NULL, then pass in the address of that variable.  Keep passing
 *       in that same void-pointer-pointer to re-use the buffer.  When you are
 *       done with the buffer, use sds_buffer_free to free it from memory.
 */
void *sds_read(SDSVarInfo *var, void **bufp)
{
    int *index = ALLOCA(int, var->ndims);
    for (int i = 0; i < var->ndims; i++) {
        index[i] = -1; // read all of this dimension
    }
    return (var->sds->funcs->var_readv)(var, bufp, index);
}

/* Writes all of a given variable.
 * buf: a pointer to the raw data.
 */
void sds_write(SDSVarInfo *var, void *buf)
{
    int *index = ALLOCA(int, var->ndims);
    for (int i = 0; i < var->ndims; i++) {
        index[i] = -1; // read all of this dimension
    }
    (var->sds->funcs->var_writev)(var, buf, index);
}

void sds_writev(SDSVarInfo *var, void *buf, int *idx)
{
    (var->sds->funcs->var_writev)(var, buf, idx);
}

/* Read all of one timestep (i.e. the first dimension) from the given variable.
 * Returns a pointer to malloc()'d data managed by bufp.
 * bufp: an opaque pointer to a buffer structure used to manage memory to be
 *       read into and other housekeeping.  In your code, create a void pointer
 *       set to NULL, then pass in the address of that variable.  Keep passing
 *       in that same void-pointer-pointer to re-use the buffer.  When you are
 *       done with the buffer, use sds_buffer_free to free it from memory.
 */
void *sds_timestep(SDSVarInfo *var, void **bufp, int tstep)
{
    int *index = ALLOCA(int, var->ndims);
    index[0] = tstep;
    for (int i = 1; i < var->ndims; i++) {
        index[i] = -1; // read all of this dimension
    }
    return (var->sds->funcs->var_readv)(var, bufp, index);
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
void *sds_readv(SDSVarInfo *var, void **bufp, int *index)
{
    return (var->sds->funcs->var_readv)(var, bufp, index);
}

struct GenericBuffer {
    void (*free_func)(void *);
};

void sds_buffer_free(void *buf)
{
    struct GenericBuffer *gb = (struct GenericBuffer *)buf;
    (gb->free_func)(buf);
}

void sds_free_atts(SDSAttInfo *att)
{
    if (att) {
        SDSAttInfo *next = att->next;
        free(att->name);
        free(att->data.v);
        free(att);
        sds_free_atts(next);
    }
}

void sds_free_dims(SDSDimInfo *dim)
{
    if (dim) {
        SDSDimInfo *next = dim->next;
        free(dim->name);
        free(dim);
        sds_free_dims(next);
    }
}

void sds_free_vars(SDSVarInfo *var)
{
    if (var) {
        SDSVarInfo *next = var->next;
        free(var->name);
        sds_free_atts(var->atts);
        free(var->dims);
        free(var);
        sds_free_vars(next);
    }
}

void sds_close(SDSInfo *sds)
{
    if (sds->funcs)
        sds->funcs->close(sds);

    sds_free_atts(sds->gatts);
    sds_free_dims(sds->dims);
    sds_free_vars(sds->vars);

    free(sds->path);
    free(sds);
}
