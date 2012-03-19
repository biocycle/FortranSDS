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
    int ncid; /* private */
} SDSInfo;

SDSInfo *open_nc_sds(const char *path);
int close_nc_sds(SDSInfo *sds);

#endif /* SDS_H */
