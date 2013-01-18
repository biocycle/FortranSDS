#ifndef SDS_H
#define SDS_H

#include <stdint.h>
#include <stdlib.h>

// multi-dimensional array indexing mapped onto 1-d array
#define SDS_IDX2D(lat,lon, nlon) (lon + nlon * lat)
#define SDS_IDX3D(lev,lat,lon, nlat,nlon) (lon + nlon * (lat + lev * nlat))
#define SDS_IDX4D(time,lev,lat,lon, nlev,nlat,nlon) \
    (lon + nlon * (lat + nlat * (lev + time * nlev)))

typedef enum {
    SDS_UNKNOWN_FILE,
    SDS_NC3_FILE,
    SDS_NC4_FILE,
    SDS_HDF4_FILE,
    SDS_HDF5_FILE
} SDSFileType;

typedef enum {
    SDS_NO_TYPE,
    SDS_I8,
    SDS_U8,
    SDS_I16,
    SDS_U16,
    SDS_I32,
    SDS_U32,
    SDS_I64,
    SDS_U64,
    SDS_FLOAT,
    SDS_DOUBLE,
    SDS_STRING
} SDSType;

#define SDS_COORD 1
#define SDS_DATA 0

#define SDS_UNLIM 1
#define SDS_LIM 0

typedef struct SDSAttInfo {
    struct SDSAttInfo *next;
    char *name;
    SDSType type;
    size_t count; /* number of elements */
    size_t bytes; /* # bytes per element */
    union {
        int8_t   *b;
        uint8_t  *ub;
        int16_t  *s;
        uint16_t *us;
        int32_t  *i;
        uint32_t *ui;
        int64_t  *i64;
        uint64_t *u64;
        float    *f;
        double   *d;
        char     *str;
        void     *v;
    } data;
} SDSAttInfo;

typedef struct SDSDimInfo {
    struct SDSDimInfo *next;
    char *name;
    size_t size;
    int isunlim; // unlimited dimension?
    int id; /* private */
} SDSDimInfo;

typedef struct SDSVarInfo {
    struct SDSVarInfo *next;
    char *name;
    SDSType type;
    int iscoord; // coordinate variable?
    int ndims;
    SDSDimInfo **dims;
    SDSAttInfo *atts;
    int id; /* private */
} SDSVarInfo;

typedef struct {
    char *path;
    SDSFileType type;
    SDSAttInfo *gatts;
    SDSDimInfo *dims;
    SDSVarInfo *vars;
    // note: for HDF files, there can be multiple unlimited dimensions
    SDSDimInfo *unlimdim;

    // private
    int id;
    struct SDS_Funcs *funcs;
} SDSInfo;

struct SDS_Funcs {
    void *(*var_readv)(SDSInfo *, SDSVarInfo *, void **, int *);
    void (*var_writev)(SDSInfo *, SDSVarInfo *, void *, int *);
    void (*close)(SDSInfo *);
};

SDSFileType sds_file_type(const char *path);

// open existing SDS file
SDSInfo *open_nc_sds(const char *path);
SDSInfo *open_h4_sds(const char *path);
SDSInfo *open_any_sds(const char *path);

// write new SDS file
void write_as_nc_sds(const char *path, SDSInfo *sds);

// read variable data
void *sds_read_var_by_name(SDSInfo *sds, const char *name, void **bufp);

void *sds_read(SDSInfo *sds, SDSVarInfo *var, void **bufp);
void *sds_timestep(SDSInfo *sds, SDSVarInfo *var, void **buf, int tstep);
void *sds_readv(SDSInfo *sds, SDSVarInfo *var, void **bufp, int *idx);

void sds_buffer_free(void *buf);

// write variable data
void sds_write(SDSInfo *sds, SDSVarInfo *var, void *buf);

// close any open SDS file
void sds_close(SDSInfo *sds);

// free lists of various things
void sds_free_atts(SDSAttInfo *atts);
void sds_free_dims(SDSDimInfo *dims);
void sds_free_vars(SDSVarInfo *vars);

// Creating new SDS structures from scratch
SDSInfo *create_sds(SDSAttInfo *gatts, SDSDimInfo *dims, SDSVarInfo *vars);

SDSAttInfo *sds_create_att(SDSAttInfo *next, const char *name, SDSType type,
                           size_t count, const void *data);
SDSAttInfo *sds_create_stratt(SDSAttInfo *next, const char *name,
                              const char *str);

SDSDimInfo *sds_create_dim(SDSDimInfo *next, const char *name, size_t size,
                           int isunlim);

SDSVarInfo *sds_create_varv(SDSVarInfo *next, const char *name, SDSType type,
                            int iscoord, SDSAttInfo *atts, int ndims, ...);
SDSVarInfo *sds_create_var(SDSVarInfo *next, const char *name, SDSType type,
                           int iscoord, SDSAttInfo *atts,
                           int ndims, SDSDimInfo **dims);

// Copy an existing SDS structure to write a modified version thereof
SDSInfo *sds_generic_copy(SDSInfo *sds);
SDSAttInfo *sds_atts_generic_copy(SDSAttInfo *att);
SDSDimInfo *sds_dims_generic_copy(SDSDimInfo *dim);
SDSVarInfo *sds_vars_generic_copy(SDSVarInfo *var, SDSDimInfo *newdims);

size_t sds_type_size(SDSType t);
size_t sds_var_size(SDSVarInfo *var);
size_t sds_var_count(SDSVarInfo *var);

SDSAttInfo *sds_att_by_name(SDSAttInfo *atts, const char *name);
SDSDimInfo *sds_dim_by_name(SDSDimInfo *dims, const char *name);
SDSVarInfo *sds_var_by_name(SDSVarInfo *vars, const char *name);

SDSAttInfo *sds_sort_attributes(SDSAttInfo *atts);
SDSVarInfo *sds_sort_vars(SDSVarInfo *vars);

#endif /* SDS_H */
