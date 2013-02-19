/* sds_hdf4.c - HDF4 implementation of SDS interface.
 * Copyright (C) 2011 Matthew S. Bishop
 */
#include "sds.h"
#include "util.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>

#include <mfhdf.h>
#include <hproto.h>

// from http://www.hdfgroup.org/release4/doc/UsrGuide_html/UG_SD.html#wp17650
#define _H4_MAX_SDS_NAME 64

static void hdf_error(const char *filename, int status,
                      const char *sourcefile, int lineno)
{
    fprintf(stderr, "%s:%i: ", sourcefile, lineno);
    fprintf(stderr, "Error reading %s: %s\n", filename, HEstring(HEvalue(1)));
    exit(2);
}

#define CHECK_HDF_ERROR(filename, status) \
    if ((status) == FAIL) hdf_error(filename,status,__FILE__,__LINE__)

typedef struct {
    void (*free)(void *);
    const char *path; // don't free; a copy from SDSInfo
    void *data;
    size_t size;
    int sds_index;
    int sds_id;
} H4Buffer;

static void h4buffer_free(H4Buffer *buf)
{
    assert(buf->free == (void (*)(void *))h4buffer_free);

    if (buf->data) {
        free(buf->data);
    }
    if (buf->sds_id != -1) {
        int status = SDendaccess(buf->sds_id);
        CHECK_HDF_ERROR(buf->path, status);
    }
    free(buf);
}

static H4Buffer *h4buffer_create(SDSInfo *sds)
{
    H4Buffer *buf = NEW(H4Buffer);
    buf->free = (void (*)(void *))h4buffer_free;
    buf->path = sds->path;
    buf->data = NULL;
    buf->size = 0;
    buf->sds_index = -1;
    buf->sds_id = -1;
    return buf;
}

static void h4buffer_ensure(H4Buffer *buf, size_t cap_needed)
{
    if (buf->size < cap_needed) {
        buf->data = xrealloc(buf->data, cap_needed);
        buf->size = cap_needed;
    }
}

static H4Buffer *prep_read_buffer(SDSVarInfo *var, void **bufp)
{
    H4Buffer *buf = (H4Buffer *)*bufp;
    if (buf) {
        assert(buf->free == (void (*)(void *))h4buffer_free);
    } else {
        *((H4Buffer **)bufp) = buf = h4buffer_create(var->sds);
    }

    if (buf->sds_index != var->id) {
        if (buf->sds_id != -1) { // close other var
            int status = SDendaccess(buf->sds_id);
            CHECK_HDF_ERROR(buf->path, status);
        }

        // open this var
        buf->sds_id = SDselect(var->sds->id, var->id);
        CHECK_HDF_ERROR(var->sds->path, buf->sds_id);
        buf->sds_index = var->id;
    }

    return buf;
}

static void *var_readv(SDSVarInfo *var, void **bufp, int *index)
{
    int32 start[H4_MAX_VAR_DIMS], count[H4_MAX_VAR_DIMS];
    size_t bufsize = sds_type_size(var->type);
    for (int i = 0; i < var->ndims; i++) {
        if (index[i] < 0) {
            start[i] = 0;
            count[i]   = (int32)var->dims[i]->size;
        } else {
            start[i] = (int32)index[i];
            count[i]   = 1;
        }
        bufsize *= (size_t)count[i];
    }

    H4Buffer *buf = prep_read_buffer(var, bufp);
    h4buffer_ensure(buf, bufsize);

    int status = SDreaddata(buf->sds_id, start, NULL, count, buf->data);
    CHECK_HDF_ERROR(var->sds->path, status);

    return buf->data;
}

static void var_writev(SDSVarInfo *var, void *data, int *index)
{
	fprintf(stderr, "hdf4 variable writing not implemented yet!\n");
	abort();
}

static void close_hdf(SDSInfo *sds)
{
    int status = SDend(sds->id);
    CHECK_HDF_ERROR(sds->path, status);
}

static struct SDS_Funcs h4_funcs = {
    var_readv,
	var_writev,
    close_hdf
};

static SDSType h4_to_sdstype(int32 h4type)
{
    switch (h4type) {
    case DFNT_CHAR8:
    case DFNT_UCHAR8:  return SDS_STRING;
    case DFNT_INT8:    return SDS_I8;
    case DFNT_UINT8:   return SDS_U8;
    case DFNT_INT16:   return SDS_I16;
    case DFNT_UINT16:  return SDS_U16;
    case DFNT_INT32:   return SDS_I32;
    case DFNT_UINT32:  return SDS_U32;
    case DFNT_FLOAT32: return SDS_FLOAT;
    case DFNT_FLOAT64: return SDS_DOUBLE;
    default:           return SDS_NO_TYPE;
    }
    abort();
}

static size_t h4_typesize(int32 h4type)
{
    switch (h4type) {
    case DFNT_CHAR8:
    case DFNT_UCHAR8:
    case DFNT_INT8:
    case DFNT_UINT8:   return 1;
    case DFNT_INT16:
    case DFNT_UINT16:  return 2;
    case DFNT_INT32:
    case DFNT_UINT32:
    case DFNT_FLOAT32: return 4;
    case DFNT_FLOAT64: return 8;
    default: abort(); break;
    }
    return 0;
}

static SDSAttInfo *read_attributes(const char *path, int obj_id, int natts)
{
    SDSAttInfo *att, *att_list = NULL;
    char buf[H4_MAX_NC_NAME + 1];
    int32 type, nvalues;
    int i, status;

    for (i = 0; i < natts; i++) {
        memset(buf, 0, sizeof(buf));
        status = SDattrinfo(obj_id, i, buf, &type, &nvalues);
        CHECK_HDF_ERROR(path, status);

        if (!strncasecmp("coremetadata",     buf, 12) ||
            !strncasecmp("structmetadata",   buf, 14) ||
            !strncasecmp("archivemetadata",  buf, 15) ||
            !strncasecmp("archivedmetadata", buf, 16))
            continue; // skip these useless attributes

        // read attribute data
        size_t typesize = h4_typesize(type);
        if (type == DFNT_CHAR8 || type == DFNT_UCHAR8)
            nvalues++;
        void *data = xmalloc(typesize * nvalues);
        status = SDreadattr(obj_id, i, data);
        CHECK_HDF_ERROR(path, status);
        if (type == DFNT_CHAR8 || type == DFNT_UCHAR8)
            ((char *)data)[nvalues - 1] = '\0';

        // stick attribute in struct in list
        att = NEW(SDSAttInfo);
        att->name = xstrdup(buf);
        att->type = h4_to_sdstype(type);
        att->count = (size_t)nvalues;
        att->bytes = typesize;
        att->data.v = data;

        att->next = att_list;
        att_list = att;
    }
    return (SDSAttInfo *)list_reverse((List *)att_list);
}

// match /fakeDim\d+/
static int is_fake_dim(const char *name)
{
    if (strncmp("fakeDim", name, 7))
        return 0; // not a fake dim

    const char *s = name + 7;
    do {
        if (*s < '0' || '9' < *s)
            return 0; // not a fakeDim
        s++;
    } while (*s != '\0');

    return 1;
}

static SDSDimInfo *dim_by_id_or_same_fake(SDSDimInfo *dim, int dim_id,
                                          size_t size, const char *name)
{
    SDSDimInfo *d = dim;

    // try to find by ID
    while (dim) {
        if (dim->id == dim_id)
            return dim;
        dim = dim->next;
    }

    // see if it's a duplicate /fakeDim\d+/
    if (is_fake_dim(name)) {
        dim = d;
        while (dim) {
            if (is_fake_dim(dim->name) && dim->size == size && !dim->isunlim)
                return dim;
            dim = dim->next;
        }
    }

    return NULL;
}

static SDSDimInfo **read_dimensions(SDSInfo *sds, int sds_id, int rank,
                                    const int *dim_sizes)
{
    char buf[H4_MAX_NC_NAME + 1];
    int i, status;
    int32 size, type, natts;

    SDSDimInfo **dims = NEWA(SDSDimInfo *, rank);

    for (i = 0; i < rank; i++) {
        int dim_id = SDgetdimid(sds_id, i);
        CHECK_HDF_ERROR(sds->path, dim_id);

        memset(buf, 0, sizeof(buf));
        status = SDdiminfo(dim_id, buf, &size, &type, &natts);
        CHECK_HDF_ERROR(sds->path, status);

        SDSDimInfo *dim = dim_by_id_or_same_fake(sds->dims, dim_id, size, buf);
        if (!dim) {
            dim = NEW0(SDSDimInfo);
            dim->name = xstrdup(buf);
            dim->size = dim_sizes[i];
            dim->isunlim = (size == 0);
            dim->id = dim_id;

            dim->next = sds->dims;
            sds->dims = dim;

            if (dim->isunlim)
                sds->unlimdim = dim;
        }
        dims[i] = dim;
    }

    return dims;
}

/* Opens an HDF file and reads all its SDS metadata, returning an SDSInfo
 * structure containing this metadata.  Returns NULL on error.
 */
SDSInfo *open_h4_sds(const char *path)
{
    int i, status;

    int sd_id = SDstart(path, DFACC_READ);
    CHECK_HDF_ERROR(path, sd_id);

    // get dataset and global att counts
    int32 n_datasets, n_global_atts;
    status = SDfileinfo(sd_id, &n_datasets, &n_global_atts);
    CHECK_HDF_ERROR(path, status);

    SDSInfo *sds = NEW0(SDSInfo);
    sds->path = xstrdup(path);
    sds->type = SDS_HDF4_FILE;
    sds->id = sd_id;

    // read global attributes
    sds->gatts = read_attributes(path, sd_id, n_global_atts);

    // read variables ('datasets')
    for (i = 0; i < n_datasets; i++) {
        int sds_id = SDselect(sd_id, i);
        CHECK_HDF_ERROR(path, sds_id);

        char buf[_H4_MAX_SDS_NAME + 1];
        memset(buf, 0, sizeof(buf));
        int32 rank, dim_sizes[H4_MAX_VAR_DIMS], type, natts;
        status = SDgetinfo(sds_id, buf, &rank, dim_sizes, &type, &natts);
        CHECK_HDF_ERROR(path, status);

        SDSVarInfo *var = NEW0(SDSVarInfo);
        var->name = xstrdup(buf);
        var->type = h4_to_sdstype(type);
        var->iscoord = SDiscoordvar(sds_id);
        var->ndims = rank;
        var->dims = read_dimensions(sds, sds_id, rank, dim_sizes);
        var->atts = read_attributes(path, sds_id, natts);
        var->id = i; // actually the sds_index

        comp_coder_t comp_type;
        comp_info c_info;
        status = SDgetcompinfo(sds_id, &comp_type, &c_info);
        CHECK_HDF_ERROR(path, status);
        switch (comp_type) {
        case COMP_CODE_NONE:
            var->compress = 0;
            break;
        case COMP_CODE_DEFLATE:
            var->compress = c_info.deflate.level;
            break;
        default:
            // any other compression method is 'worth' 1 imo *trollface*
            // better than claiming 0 to the user
            var->compress = 1;
            break;
        }

        var->sds = sds;

        var->next = sds->vars;
        sds->vars = var;

        status = SDendaccess(sds_id);
        CHECK_HDF_ERROR(path, status);
    }
    sds->vars = (SDSVarInfo *)list_reverse((List *)sds->vars);
    sds->dims = (SDSDimInfo *)list_reverse((List *)sds->dims);

    sds->funcs = &h4_funcs;
    return sds;
}
