#ifndef SDS_H
#define SDS_H

#include <stdint.h>
#include <stdlib.h>

typedef enum {
    SDS_NO_TYPE,
    SDS_BYTE,
    SDS_SHORT,
    SDS_INT,
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
        int8_t  *b;
        int16_t *s;
        int32_t *i;
        float   *f;
        double  *d;
        char    *str;
        void    *v;
    } data;
} SDSAttInfo;

typedef struct SDSDimInfo {
    struct SDSDimInfo *next;
    char *name;
    size_t size;
    int id; /* private */
} SDSDimInfo;

typedef struct SDSVarInfo {
    struct SDSVarInfo *next;
    char *name;
    SDSType type;
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
    SDSDimInfo *unlimdim;
    // private
    int ncid;
    struct SDS_Funcs *funcs;
} SDSInfo;

struct SDS_Funcs {
    void *(*var_read)(SDSInfo *, SDSVarInfo *);
};

SDSAttInfo *sort_attributes(SDSAttInfo *atts);
SDSVarInfo *sort_vars(SDSVarInfo *vars);

size_t sds_type_size(SDSType t);

SDSDimInfo *sds_dim_by_name(SDSInfo *sds, const char *name);
SDSVarInfo *sds_var_by_name(SDSInfo *sds, const char *name);

void *sds_read_var(SDSInfo *sds, SDSVarInfo *var);

SDSInfo *open_nc_sds(const char *path);
int close_nc_sds(SDSInfo *sds);

#endif /* SDS_H */
