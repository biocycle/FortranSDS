/* sds_nc.c - NetCDF implementation of SDS interface.
 * Copyright (C) 2011 Matthew S. Bishop
 */
#include "sds.h"
#include "util.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netcdf.h>

static void netcdf_error(const char *filename, int status,
                         const char *sourcefile, int lineno)
{
    fprintf(stderr, "%s:%i: ", sourcefile, lineno);
    fprintf(stderr, "Error in %s: %s\n", filename, nc_strerror(status));
    abort();
}

#define CHECK_NC_ERROR(filename,status) \
    if ((status) != NC_NOERR) netcdf_error(filename,status,__FILE__,__LINE__)

typedef struct {
    void (*free)(void *);
    const char *path;
    void *data;
    size_t size;
} NCBuffer;

static void nc_buffer_free(NCBuffer *buf)
{
    assert(buf->free == (void (*)(void *))nc_buffer_free);

    if (buf->data) {
        free(buf->data);
    }
    free(buf);
}

static NCBuffer *nc_buffer_create(SDSInfo *sds)
{
    NCBuffer *buf = NEW(NCBuffer);
    buf->free = (void (*)(void *))nc_buffer_free;
    buf->path = sds->path;
    buf->data = NULL;
    buf->size = 0;
    return buf;
}

static void nc_buffer_ensure(NCBuffer *buf, size_t cap_needed)
{
    if (buf->size < cap_needed) {
        buf->data = xrealloc(buf->data, cap_needed);
        buf->size = cap_needed;
    }
}

static void *var_readv(SDSVarInfo *var, void **bufp, int *index)
{
    int status;

    size_t *start = ALLOCA(size_t, var->ndims);
    size_t *count = ALLOCA(size_t, var->ndims);
    size_t bufsize = sds_type_size(var->type);
    for (int i = 0; i < var->ndims; i++) {
        if (index[i] < 0) {
            start[i] = 0;
            count[i] = var->dims[i]->size;
        } else {
            start[i] = (size_t)index[i];
            count[i] = 1;
        }
        bufsize *= count[i];
    }

    NCBuffer *buf = (NCBuffer *)*bufp;
    if (buf) {
        assert(buf->free == (void (*)(void *))nc_buffer_free);
    } else {
        *((NCBuffer **)bufp) = buf = nc_buffer_create(var->sds);
    }
    nc_buffer_ensure(buf, bufsize);

#if HAVE_NETCDF4
    status = nc_get_vars(var->sds->id, var->id, start, count, NULL, buf->data);
    CHECK_NC_ERROR(var->sds->path, status);
#else
    switch (var->type) {
    case SDS_I8:
        status = nc_get_vars_uchar(var->sds->id, var->id, start, count, NULL,
                                   (unsigned char*)buf->data);
        CHECK_NC_ERROR(var->sds->path, status);
        break;
    case SDS_I16:
        status = nc_get_vars_short(var->sds->id, var->id, start, count, NULL,
                                   (short*)buf->data);
        CHECK_NC_ERROR(var->sds->path, status);
        break;
    case SDS_I32:
        status = nc_get_vars_int(var->sds->id, var->id, start, count, NULL,
                                 (int*)buf->data);
        CHECK_NC_ERROR(var->sds->path, status);
        break;
    case SDS_FLOAT:
        status = nc_get_vars_float(var->sds->id, var->id, start, count, NULL,
                                   (float*)buf->data);
        CHECK_NC_ERROR(var->sds->path, status);
        break;
    case SDS_DOUBLE:
        status = nc_get_vars_double(var->sds->id, var->id, start, count, NULL,
                                    (double*)buf->data);
        CHECK_NC_ERROR(var->sds->path, status);
        break;
    case SDS_STRING:
        status = nc_get_vars_text(var->sds->id, var->id, start, count, NULL,
                                  (char*)buf->data);
        CHECK_NC_ERROR(var->sds->path, status);
        break;
    case SDS_NO_TYPE:
    default:
        abort();
        break;
    }
#endif
    return buf->data;
}

static void var_writev(SDSVarInfo *var, void *data, int *index)
{
    int status;

    size_t *start = ALLOCA(size_t, var->ndims);
    size_t *count = ALLOCA(size_t, var->ndims);
    for (int i = 0; i < var->ndims; i++) {
        if (index[i] < 0) {
            start[i] = 0;
            count[i] = var->dims[i]->size;
        } else {
            start[i] = (size_t)index[i];
            count[i] = 1;
        }
    }

#if HAVE_NETCDF4
    status = nc_put_vars(var->sds->id, var->id, start, count, NULL, data);
    CHECK_NC_ERROR(var->sds->path, status);
#else
    switch (var->type) {
    case SDS_I8:
        status = nc_put_vars_uchar(var->sds->id, var->id, start, count, NULL,
                                   (unsigned char*)data);
        CHECK_NC_ERROR(var->sds->path, status);
        break;
    case SDS_I16:
        status = nc_put_vars_short(var->sds->id, var->id, start, count, NULL,
                                   (short*)data);
        CHECK_NC_ERROR(var->sds->path, status);
        break;
    case SDS_I32:
        status = nc_put_vars_int(var->sds->id, var->id, start, count, NULL,
                                 (int*)data);
        CHECK_NC_ERROR(var->sds->path, status);
        break;
    case SDS_FLOAT:
        status = nc_put_vars_float(var->sds->id, var->id, start, count, NULL,
                                   (float*)data);
        CHECK_NC_ERROR(var->sds->path, status);
        break;
    case SDS_DOUBLE:
        status = nc_put_vars_double(var->sds->id, var->id, start, count, NULL,
                                    (double*)data);
        CHECK_NC_ERROR(var->sds->path, status);
        break;
    case SDS_STRING:
        status = nc_put_vars_text(var->sds->id, var->id, start, count, NULL,
                                  (char*)data);
        CHECK_NC_ERROR(var->sds->path, status);
        break;
    case SDS_NO_TYPE:
    default:
        fprintf(stderr, "Attempt to write variable %s type %i unsupported "
                "by NetCDF 3\n", var->name, (int)var->type);

        abort();
        break;
    }
#endif
}

static void close_nc(SDSInfo *sds)
{
    int status = nc_close(sds->id);
    CHECK_NC_ERROR(sds->path, status);
}

static struct SDS_Funcs nc_funcs = {
    var_readv,
    var_writev,
    close_nc
};

static SDSType nc_to_sds_type(nc_type type)
{
    switch (type) {
	case NC_NAT:    return SDS_NO_TYPE;
	case NC_BYTE:   return SDS_I8;
	case NC_CHAR:   return SDS_STRING;
	case NC_SHORT:  return SDS_I16;
	case NC_INT:    return SDS_I32;
	case NC_FLOAT:  return SDS_FLOAT;
	case NC_DOUBLE: return SDS_DOUBLE;
#ifdef HAVE_NETCDF4
    case NC_USHORT: return SDS_U16;
    case NC_UINT:   return SDS_U32;
    case NC_INT64:  return SDS_I64;
    case NC_UINT64: return SDS_U64;
    case NC_STRING: return SDS_STRING;
#endif
    default: break;
    }
    abort();
}

static nc_type sds_to_nc_type(SDSType type)
{
    switch (type) {
    case SDS_NO_TYPE: return NC_NAT;
    case SDS_I8:      return NC_BYTE;
    case SDS_I16:     return NC_SHORT;
    case SDS_I32:     return NC_INT;
#if HAVE_NETCDF4
    case SDS_U16:     return NC_USHORT;
    case SDS_U32:     return NC_UINT;
    case SDS_I64:     return NC_INT64;
    case SDS_U64:     return NC_UINT64;
#endif
    case SDS_FLOAT:   return NC_FLOAT;
    case SDS_DOUBLE:  return NC_DOUBLE;
    case SDS_STRING:  return NC_CHAR;
    default: break;
    }
    abort();
}

static SDSAttInfo *read_attributes(const char *path, int ncid, int id,
                                   int natts)
{
    SDSAttInfo *att, *att_list = NULL;
    char buf[NC_MAX_NAME + 1];
    int status, i;
    nc_type type;
    size_t count, bytes;
    void *data;

    for (i = 0; i < natts; i++) {
        status = nc_inq_attname(ncid, id, i, buf);
        CHECK_NC_ERROR(path, status);
        status = nc_inq_att(ncid, id, buf, &type, &count);
        CHECK_NC_ERROR(path, status);

#ifdef HAVE_NETCDF4
        status = nc_inq_type(ncid, type, NULL, &bytes);
        CHECK_NC_ERROR(path, status);

        switch (type) {
        case NC_STRING:
            // override to equivalent type for easier processing below
            type = NC_CHAR;
            // fall through
        case NC_CHAR:
            count++;
            break;
        default:
            break;
        }
#else
        switch (type) {
        case NC_NAT:    bytes = 0; break;
        case NC_BYTE:   bytes = 1; break;
        case NC_CHAR:   bytes = 1; count++; break;
        case NC_SHORT:  bytes = 2; break;
        case NC_INT:    bytes = 4; break;
        case NC_FLOAT:  bytes = 4; break;
        case NC_DOUBLE: bytes = 8; break;
        default: abort(); break;
        }
#endif
        data = xmalloc(count * bytes);
        status = nc_get_att(ncid, id, buf, data);
        CHECK_NC_ERROR(path, status);
        if (type == NC_CHAR) {
            ((char *)data)[count - 1] = '\0';
        }

        att = NEW(SDSAttInfo);
        att->name = xstrdup(buf);
        att->type = nc_to_sds_type(type);
        att->count = count;
        att->bytes = bytes;
        att->data.v = data;

        att->next = att_list;
        att_list = att;
    }
    return (SDSAttInfo *)list_reverse((List *)att_list);
}

/* Checks the list of dimensions for a match with the given variable name.
 * That is the definition of a coordinate variable in NetCDF.
 */
static int is_coord_var(SDSDimInfo *dim, const char *varname)
{
    while (dim) {
        if (!strcmp(dim->name, varname))
            return 1;
        dim = dim->next;
    }
    return 0;
}

/* Map array of the variable's dimension ids to an array of SDSDimInfo pointers.
 */
static void map_dimids(SDSVarInfo *vi, int *dimids, SDSDimInfo *dims)
{
    SDSDimInfo *di;
    int i;

    for (i = 0; i < vi->ndims; i++) {
        for (di = dims; di != NULL; di = di->next) {
            if (di->id == dimids[i]) {
                vi->dims[i] = di;
                break;
            }
        }
        assert(vi->dims[i] != NULL);
    }
}

/* Opens a NetCDF file and reads all its metadata, returning an SDSInfo
 * structure containing this metadata.  Returns NULL on error.
 */
SDSInfo *open_nc_sds(const char *path)
{
    int ids[MAX(MAX(NC_MAX_DIMS, NC_MAX_ATTRS), NC_MAX_VARS)];
    int dimids[NC_MAX_VAR_DIMS], unlimdimid;
    char buf[NC_MAX_NAME + 1];
    int ncid, status, i, ndims, nvars, ngatts;
    SDSInfo *sds;

    status = nc_open(path, NC_NOWRITE, &ncid);
    CHECK_NC_ERROR(path, status);

    sds = NEW0(SDSInfo);
    sds->path = xstrdup(path);
    sds->id = ncid;

#if HAVE_NETCDF4
    int fmt;
    status = nc_inq_format(ncid, &fmt);
    CHECK_NC_ERROR(path, status);
    switch (fmt) {
    case NC_FORMAT_CLASSIC:
    case NC_FORMAT_64BIT:
        sds->type = SDS_NC3_FILE;
        break;
    case NC_FORMAT_NETCDF4:
    case NC_FORMAT_NETCDF4_CLASSIC:
        sds->type = SDS_NC4_FILE;
        break;
    default:
        sds->type = SDS_UNKNOWN_FILE;
        break;
    }
#else
    sds->type = SDS_NC3_FILE;
#endif

    /* get counts for everything */
    unlimdimid = -1;
    status = nc_inq(ncid, &ndims, &nvars, &ngatts, &unlimdimid);
    CHECK_NC_ERROR(path, status);

    /* read global attributes */
    sds->gatts = read_attributes(path, ncid, NC_GLOBAL, ngatts);

    /* read dimension info */
#if HAVE_NETCDF4
    status = nc_inq_dimids(ncid, &ndims, ids, 1);
    CHECK_NC_ERROR(path, status);
#else
    for (i = 0; i < ndims; i++) {
        ids[i] = i;
    }
#endif
    for (i = 0; i < ndims; i++) {
        SDSDimInfo *dim;
        size_t size;
        status = nc_inq_dim(ncid, ids[i], buf, &size);
        CHECK_NC_ERROR(path, status);

        dim = NEW(SDSDimInfo);
        dim->name = xstrdup(buf);
        dim->size = size;
        dim->isunlim = (ids[i] == unlimdimid);
        dim->id = ids[i];

        dim->next = sds->dims;
        sds->dims = dim;

        if (ids[i] == unlimdimid)
            sds->unlimdim = dim;
    }
    sds->dims = (SDSDimInfo *)list_reverse((List *)sds->dims);

    /* read variable info */
#if HAVE_NETCDF4
    status = nc_inq_varids(ncid, &nvars, ids);
    CHECK_NC_ERROR(path, status);
#else
    for (i = 0; i < nvars; i++) {
        ids[i] = i;
    }
#endif
    for (i = 0; i < nvars; i++) {
        SDSVarInfo *vi;
        nc_type type;
        int nvdims, natts;
        status = nc_inq_var(ncid, ids[i], buf, &type, &nvdims, dimids, &natts);
        CHECK_NC_ERROR(path, status);

        vi = NEW(SDSVarInfo);
        vi->name = xstrdup(buf);
        vi->type = nc_to_sds_type(type);
        vi->iscoord = is_coord_var(sds->dims, buf);
        vi->ndims = nvdims;
        vi->id = ids[i];
        vi->dims = NEWA(SDSDimInfo *, ndims);
        map_dimids(vi, dimids, sds->dims);

        vi->atts = read_attributes(path, ncid, ids[i], natts);

        vi->sds = sds;

        vi->next = sds->vars;
        sds->vars = vi;
    }
    sds->vars = (SDSVarInfo *)list_reverse((List *)sds->vars);

    sds->funcs = &nc_funcs;
    return sds;
}

static void def_atts(const char *path, int ncid, int varid, SDSAttInfo *att)
{
    int status;

    while (att) {
#ifdef HAVE_NETCDF4
        status = nc_put_att(ncid, varid, att->name, sds_to_nc_type(att->type),
                            att->count, att->data.v);
#else
        switch (att->type) {
        case SDS_I8:
            status = nc_put_att_schar(ncid, varid, att->name, NC_BYTE,
                                      att->count, att->data.b);
            break;
        case SDS_I16:
            status = nc_put_att_short(ncid, varid, att->name, NC_SHORT,
                                      att->count, att->data.s);
            break;
        case SDS_I32:
            status = nc_put_att_int(ncid, varid, att->name, NC_INT,
                                    att->count, att->data.i);
            break;
        case SDS_FLOAT:
            status = nc_put_att_float(ncid, varid, att->name, NC_FLOAT,
                                      att->count, att->data.f);
            break;
        case SDS_DOUBLE:
            status = nc_put_att_double(ncid, varid, att->name, NC_DOUBLE,
                                       att->count, att->data.d);
            break;
        case SDS_STRING:
            status = nc_put_att_text(ncid, varid, att->name,
                                     (size_t)att->count, att->data.str);
            break;
        default:
            fprintf(stderr, "Attempt to create attribute %s type %i "
                    "unsupported by NetCDF 3\n", att->name, (int)att->type);
            abort();
            break;
        }
#endif
        CHECK_NC_ERROR(path, status);

        att = att->next;
    }
}

void write_as_nc_sds(const char *path, SDSInfo *sds)
{
    // make sure we're not starting from an open file
    if (sds->type != SDS_UNKNOWN_FILE || sds->funcs != NULL) {
        fprintf(stderr, "Attempt to create nc file %s from uncopied SDSInfo\n",
                path);
        abort();
    }

    int status, ncid, flags = 0;
#if HAVE_NETCDF4
    flags = NC_NETCDF4;
#endif
    status = nc_create(path, flags, &ncid);
    CHECK_NC_ERROR(path, status);

    SDSDimInfo *dim = sds->dims;
    while (dim) {
        status = nc_def_dim(ncid, dim->name, dim->isunlim ? NC_UNLIMITED : dim->size, &dim->id);
        CHECK_NC_ERROR(path, status);

        dim = dim->next;
    }

    SDSVarInfo *var = sds->vars;
    while (var) {
        int i, dimids[NC_MAX_VAR_DIMS];
        for (i = 0; i < var->ndims; i++) {
            dimids[i] = var->dims[i]->id;
        }
        status = nc_def_var(ncid, var->name, sds_to_nc_type(var->type), var->ndims, dimids, &var->id);
        CHECK_NC_ERROR(path, status);

        def_atts(path, ncid, var->id, var->atts);

        var->sds = sds;

        var = var->next;
    }

    def_atts(path, ncid, NC_GLOBAL, sds->gatts);

    nc_enddef(ncid);

    sds->path = xstrdup(path);
#if HAVE_NETCDF4
    sds->type = SDS_NC4_FILE;
#else
    sds->type = SDS_NC3_FILE;
#endif
    sds->id = ncid;
    sds->funcs = &nc_funcs;
}
