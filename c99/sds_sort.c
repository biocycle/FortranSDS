#include "sds.h"

static int type_order(SDSType type)
{
    switch (type) {
    case SDS_I8:
    case SDS_U8:
    case SDS_I16:
    case SDS_U16:
    case SDS_I32:
    case SDS_U32:
        return 3;
    case SDS_FLOAT:
        return 1;
    case SDS_DOUBLE:
        return 2;
    case SDS_STRING:
        return 4;
    default:
        abort();
    }
    return -1;
}

/* Compare two attributes and return
 *   a < b  => -1
 *   a = b  =>  0
 *   a > b  =>  1
 * according to the ranking (larger is earlier):
 * - characters
 * - integrals (byte, short, int) any order
 * - double
 * - float
 * then by count
 */
static int attinfo_cmp(SDSAttInfo *a, SDSAttInfo *b)
{
    int ato = type_order(a->type), bto = type_order(b->type);

    if (ato != bto)
        return ato - bto;

    /* ato == bto, so go by count */
    return a->count - b->count;
}

SDSAttInfo *sds_sort_attributes(SDSAttInfo *atts)
{
    SDSAttInfo *prev, *smallest, *ai; /* unsorted, being selected from */
    SDSAttInfo *sorted = NULL; /* sorted list */

    while (atts) {
        /* find next smallest */
        prev = NULL;
        smallest = ai = atts;
        while (ai && ai->next) {
            if (attinfo_cmp(smallest, ai->next) > 0) {
                prev = ai;
                smallest = ai->next;
            }
            ai = ai->next;
        }

        /* pull out of unsorted list */
        if (prev)
            prev->next = smallest->next;
        else
            atts = smallest->next;

        /* append to sorted list */
        smallest->next = sorted;
        sorted = smallest;
    }
    return sorted;
}

static int varinfo_cmp(SDSVarInfo *a, SDSVarInfo *b)
{
    int ato = type_order(a->type), bto = type_order(b->type);
    int i;

    if (ato != bto)
        return ato - bto;

    /* ato = bto so try ndims */
    if (a->ndims != b->ndims)
        return a->ndims - b->ndims;

    /* same ndims so compare dim sizes */
    for (i = 0; i < a->ndims; i++) {
        if (a->dims[i]->size != b->dims[i]->size)
            return a->dims[i]->size - b->dims[i]->size;
    }

    return 0; /* all the same! */
}

/* Selection sort of a given variable list, in this order:
 * - characters
 * - integrals (byte, short, int) any order
 * - double
 * - float
 * then by number of dimensions
 * then by size of dimensions
 */
SDSVarInfo *sds_sort_vars(SDSVarInfo *vars)
{
    SDSVarInfo *prev, *smallest, *ai; /* unsorted, being selected from */
    SDSVarInfo *sorted = NULL; /* sorted list */

    while (vars) {
        /* find next smallest */
        prev = NULL;
        smallest = ai = vars;
        while (ai && ai->next) {
            if (varinfo_cmp(smallest, ai->next) > 0) {
                prev = ai;
                smallest = ai->next;
            }
            ai = ai->next;
        }

        /* pull out of unsorted list */
        if (prev)
            prev->next = smallest->next;
        else
            vars = smallest->next;

        /* append to sorted list */
        smallest->next = sorted;
        sorted = smallest;
    }
    return sorted;
}

