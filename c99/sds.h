#ifndef SDS_H
#define SDS_H

#include <stdint.h>
#include <stdlib.h>

// multi-dimensional array indexing mapped onto 1-d array
// use like:
//     float *var = ...;
//     ... = var[SDS_IDX2D(lat, lon, nlon)];
#define SDS_IDX2D(i,j, nj) (i * nj + j)
#define SDS_IDX3D(i,j,k, nj,nk) ((i * nj + j) * nk + k)
#define SDS_IDX4D(i,j,k,l, nj,nk,nl) (((i * nj + j) * nk + k) * nl + l)

typedef enum {
    SDS_UNKNOWN_FILE,
    SDS_NC3_FILE,
    SDS_NC4_FILE,
    SDS_HDF4_FILE,
    SDS_HDF5_FILE
} SDSFileType;

/* Index into this array with an SDSFileType to get a human-readable name
 * for the file type.
 */
extern const char *sds_file_types[];

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

/* Index into this array with an SDSType to get a human-readable name
 * for that type.
 */
extern const char *sds_type_names[];

// Constants for the iscoord value of SDSVarInfo
#define SDS_COORD 1
#define SDS_DATA 0

// Constants for the isunlim value of SDSDimInfo
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

typedef struct SDSInfo SDSInfo;

typedef struct SDSVarInfo {
    struct SDSVarInfo *next;
    char *name;
    SDSType type;
    int compress; // 0 - no compression; 1 - lowest, 9 - best compression
    int iscoord; // coordinate variable?
    int ndims;
    SDSDimInfo **dims;
    SDSAttInfo *atts;
    int id; /* private */
    SDSInfo *sds; /* private */
} SDSVarInfo;

struct SDSInfo {
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
};

struct SDS_Funcs {
    void *(*var_readv)(SDSVarInfo *, void **, int *);
    void (*var_writev)(SDSVarInfo *, void *, int *);
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

void *sds_read(SDSVarInfo *var, void **bufp);
void *sds_timestep(SDSVarInfo *var, void **buf, int tstep);
void *sds_readv(SDSVarInfo *var, void **bufp, int *idx);

void sds_buffer_free(void *buf);

// write variable data
void sds_write(SDSVarInfo *var, void *buf);
void sds_writev(SDSVarInfo *var, void *buf, int *idx);

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

SDSAttInfo *sds_keep_atts(SDSAttInfo *atts, const char **names, int n_names);
SDSDimInfo *sds_keep_dims(SDSDimInfo *dims, const char **names, int n_names);
SDSVarInfo *sds_keep_vars(SDSVarInfo *vars, const char **names, int n_names);

SDSAttInfo *sds_delete_atts(SDSAttInfo *atts, const char **names, int n_names);
SDSDimInfo *sds_delete_dims(SDSDimInfo *dims, const char **names, int n_names);
SDSVarInfo *sds_delete_vars(SDSVarInfo *vars, const char **names, int n_names);

SDSAttInfo *sds_sort_attributes(SDSAttInfo *atts);
SDSVarInfo *sds_sort_vars(SDSVarInfo *vars);

#endif /* SDS_H */
