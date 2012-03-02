/* sds_nc.c - NetCDF implementation of SDS interface.
 * Copyright (C) 2011 Matthew S. Bishop
 */
#include "sds.h"
#include "util.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <netcdf.h>

static void netcdf_error(const char *filename, int status,
                         const char *sourcefile, int lineno)
{
    fprintf(stderr, "%s:%i: ", sourcefile, lineno);
    fprintf(stderr, "Error reading %s: %s\n", filename, nc_strerror(status));
    exit(2);
}

#define CHECK_NC_ERROR(filename,status) \
    if ((status) != NC_NOERR) netcdf_error(filename,status,__FILE__,__LINE__)

static SDSType nc_to_sds_type(nc_type type)
{
    switch (type) {
	case NC_NAT:    return SDS_NO_TYPE;
	case NC_BYTE:   return SDS_BYTE;
	case NC_CHAR:   return SDS_STRING;
	case NC_SHORT:  return SDS_SHORT;
	case NC_INT:    return SDS_INT;
	case NC_FLOAT:  return SDS_FLOAT;
	case NC_DOUBLE: return SDS_DOUBLE;
    default: break;
    }
    abort();
}

static void read_attributes(const char *path, int ncid, int id, int natts,
                            SkipList *sl)
{
    char buf[NC_MAX_NAME + 1];
    int status, i;
    nc_type type;
    size_t count, bytes;
    SDSAttInfo *att;
    void *data;

    for (i = 0; i < natts; i++) {
        status = nc_inq_attname(ncid, id, i, buf);
        CHECK_NC_ERROR(path, status);
        status = nc_inq_att(ncid, id, buf, &type, &count);
        CHECK_NC_ERROR(path, status);

        status = nc_inq_type(ncid, type, NULL, &bytes);
        CHECK_NC_ERROR(path, status);
        data = xmalloc(count * bytes);
        status = nc_get_att(ncid, id, buf, data);
        CHECK_NC_ERROR(path, status);

        att = NEW(SDSAttInfo);
        att->name = xstrdup(buf);
        att->type = nc_to_sds_type(type);
        att->count = count;
        att->bytes = bytes;
        att->data.v = data;
        skiplist_add(sl, att->name, att);
    }
}

static int search_dimid(char *key, SDSDimInfo *dim, int *id)
{
    return (dim->id == *id) ? 1 : 0;
}

/* Map array of the variable's dimension ids to an array of SDSDimInfo pointers.
 */
static void map_dimids(SDSVarInfo *vi, int *dimids, SkipList *dims)
{
    int i;

    for (i = 0; i < vi->ndims; i++) {
        vi->dims[i] = skiplist_search(dims,
            (int (*)(char *, void *, void *))search_dimid, &dimids[i]);
        assert(vi->dims[i] != NULL);
    }
}

/* Opens a NetCDF file and reads all its metadata, return an SDSInfo
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
    sds->ncid = ncid;

    /* get counts for everything */
    unlimdimid = -1;
    status = nc_inq(ncid, &ndims, &nvars, &ngatts, &unlimdimid);
    CHECK_NC_ERROR(path, status);

    /* read global attributes */
    sds->gatts = skiplist_new();
    read_attributes(path, ncid, NC_GLOBAL, ngatts, sds->gatts);

    /* read dimension info */
    status = nc_inq_dimids(ncid, &ndims, ids, 1);
    CHECK_NC_ERROR(path, status);
    sds->dims = skiplist_new();
    for (i = 0; i < ndims; i++) {
        SDSDimInfo *dim;
        size_t size;
        status = nc_inq_dim(ncid, ids[i], buf, &size);
        CHECK_NC_ERROR(path, status);

        dim = NEW(SDSDimInfo);
        dim->name = xstrdup(buf);
        dim->size = size;
        dim->id = ids[i];
        skiplist_add(sds->dims, dim->name, dim);

        if (ids[i] == unlimdimid)
            sds->unlimdim = dim;
    }

    /* read variable info */
    status = nc_inq_varids(ncid, &nvars, ids);
    CHECK_NC_ERROR(path, status);
    sds->vars = skiplist_new();
    for (i = 0; i < nvars; i++) {
        SDSVarInfo *vi;
        nc_type type;
        int nvdims, natts;
        status = nc_inq_var(ncid, ids[i], buf, &type, &nvdims, dimids, &natts);
        CHECK_NC_ERROR(path, status);

        vi = NEW(SDSVarInfo);
        vi->name = xstrdup(buf);
        vi->type = nc_to_sds_type(type);
        vi->ndims = nvdims;
        vi->id = ids[i];
        vi->dims = NEWA(SDSDimInfo *, ndims);
        map_dimids(vi, dimids, sds->dims);

        vi->atts = skiplist_new();
        read_attributes(path, ncid, ids[i], natts, vi->atts);

        skiplist_add(sds->vars, vi->name, vi);
    }

    return sds;
}

static void free_atts(SDSAttInfo *att)
{
    free(att->name);
    free(att->data.v);
    free(att);
}

static void free_dims(SDSDimInfo *dim)
{
    free(dim->name);
    free(dim);
}

static void free_vars(SDSVarInfo *var)
{
    free(var->name);
    if (var->atts)
        skiplist_free(var->atts, (void (*)(void *))free_atts);
    free(var->dims);
    free(var);
}

/* Close the NetCDF file and free all memory used for metadata.
 * Returns 0 on success and -1 on failure.
 */
int close_nc_sds(SDSInfo *sds)
{
    int status;

    if (sds->gatts)
        skiplist_free(sds->gatts, (void (*)(void *))free_atts);
    if (sds->dims)
        skiplist_free(sds->dims, (void (*)(void *))free_dims);
    if (sds->vars)
        skiplist_free(sds->vars, (void (*)(void *))free_vars);

    status = nc_close(sds->ncid);
    CHECK_NC_ERROR(sds->path, status);

    free(sds->path);
    free(sds);

    return 0;
}
