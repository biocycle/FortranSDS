#ifndef SDS_H
#define SDS_H

#include <stdint.h>
#include <stdlib.h>

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
    void (*close)(SDSInfo *);
};

SDSInfo *open_nc_sds(const char *path);
SDSInfo *open_h4_sds(const char *path);

void sds_close(SDSInfo *sds);

size_t sds_type_size(SDSType t);
size_t sds_var_size(SDSVarInfo *var);

SDSDimInfo *sds_dim_by_name(SDSInfo *sds, const char *name);
SDSVarInfo *sds_var_by_name(SDSInfo *sds, const char *name);

void *sds_read(SDSInfo *sds, SDSVarInfo *var, void **bufp);
void *sds_timestep(SDSInfo *sds, SDSVarInfo *var, void **buf, int tstep);
void *sds_readv(SDSInfo *sds, SDSVarInfo *var, void **bufp, int *idx);

void sds_buffer_free(void *buf);

SDSAttInfo *sds_sort_attributes(SDSAttInfo *atts);
SDSVarInfo *sds_sort_vars(SDSVarInfo *vars);

#endif /* SDS_H */
