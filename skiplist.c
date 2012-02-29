#include "skiplist.h"
#include "util.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define P (0.25f)
#define INITIAL_LEVEL 4
#define MAX_LEVEL (sizeof(void *) * 2)

#define NODE_SIZE(level) \
	(sizeof(SkipListNode) + sizeof(SkipListNode *) * (level))

#define ALLOC_NODE(level) (SkipListNode *)xmalloc(NODE_SIZE(level))

#define FORWARDS(node) ((SkipListNode **)((SkipListNode *)(node) + 1))

/* Subtractive method PRNG due to Knuth, described in chapter 7 of "Numerical
 * Recipes in C: The Art of Scientific Computing" (ISBN 0-521-43108-5).
 */

static int prng_inited = 0;

#define MBIG 1000000000
#define MSEED 161803398
#define MZ 0
#define FAC (1.0f / MBIG)

static int ma[56];
static int inext, inextp;

static void init_ran(void) {
	int i, ii, k;
	int mj, mk;
	
	mj = labs(MSEED - time(0));
	mj %= MBIG;
	ma[55] = mj;
	mk = 1;
                
	for (i = 1; i <= 54; i++) {
		ii = (21 * i) % 55;
		ma[ii] = mk;
		mk = mj - mk;
		if (mk < MZ) mk += MBIG;
		mj = ma[ii];
	}
                
	for (k = 1; k <= 4; k++) {
		for (i = 1; i <= 55; i++) {
			ma[i] -= ma[1 + (i + 30) % 55];
			if (ma[i] < MZ) ma[i] += MBIG;
		}
	}
                
	inext = 0;
	inextp = 31;
}

static double frnd(void) {
	int mj;
        
	if (++inext == 56) inext = 1;
	if (++inextp == 56) inextp = 1;
	mj = ma[inext] - ma[inextp];
	if (mj < MZ) mj += MBIG;
	ma[inext] = mj;
        
	return mj * FAC;
}

static int random_level()
{
	int level = 1;

	while (frnd() < P && level <= MAX_LEVEL) level++;

	return level;
}

static SkipListNode *create_node(char *key, void *value, int *level)
{
    SkipListNode *node;

	*level = random_level();
	node = ALLOC_NODE(*level);
	node->key = key;
	node->value = value;

	return node;
}

/* Create a new empty skip list.  This skip list uses strings for the keys
 * and a void * for the value.
 */
SkipList *skiplist_new(void)
{
    SkipList *sl;

    if (!prng_inited) {
        init_ran();
        prng_inited = 1;
    }

	sl = NEW(SkipList);
	sl->max_level = INITIAL_LEVEL;
	sl->header = ALLOC_NODE(sl->max_level);
    sl->header->key = sl->header->value = NULL;
	memset(FORWARDS(sl->header), 0, sizeof(SkipListNode *) * sl->max_level);
	sl->update = NEWA(SkipListNode *, sl->max_level);

	return sl;
}

/* Frees the skip list itself.  If you want to free the keys or values, pass
 * in an appropriate callback function otherwise use NULL for these arguments.
 */
void skiplist_free(SkipList *sl, void (*free_value)(void *))
{
	SkipListNode *n = sl->header, *f;

    do {
        f = FORWARDS(n)[0];
        if (free_value && n->value)
            (free_value)(n->value);
        free(n);
        n = f;
    } while (n);

	free(sl->update);
	free(sl);
}

/* Looks for the given key in the skip list and returns the value if found
 * or NULL if it was not.
 */
void *skiplist_find(SkipList *sl, char *key)
{
	SkipListNode *n = sl->header, *f;
	int l = sl->max_level - 1;

	while (l >= 0) {
		for (;;) {
			f = FORWARDS(n)[l];
			if (!f || strcmp(f->key, key) >= 0) break;
			n = f;
		}
		l--;
	}
	n = FORWARDS(n)[0];

	return (n && !strcmp(n->key, key)) ? n->value : NULL;
}

/* Add the given key/value pair to the skip list.  If the key already
 * exists, the old value is kept and -1 is returned.  Otherwise
 * inserts the new node in the list and returns 0.
 */
int skiplist_add(SkipList *sl, char *key, void *value)
{
	SkipListNode *n = sl->header, *f;
	int l = sl->max_level - 1;

	while (l >= 0) {
		for (;;) {
			f = FORWARDS(n)[l];
			if (!f || strcmp(f->key, key) >= 0) break;
			n = f;
		}
		sl->update[l] = n;
		l--;
	}
	n = FORWARDS(n)[0];

	if (n && !strcmp(n->key, key)) { /* key already exists */
		return -1;
	} else { /* insert new node */
		int level = -1;
		n = create_node(key, value, &level);

		if (level > sl->max_level) { /* grow header and update */
            sl->header = xrealloc(sl->header, NODE_SIZE(level));
            sl->update = xrealloc(sl->update, sizeof(SkipListNode *) * level);
            l = level - 1;
            while (l >= sl->max_level)
				sl->update[l--] = n;
			sl->max_level = level;
		}

        l = level - 1;
		while (l >= 0) {
			FORWARDS(n)[l] = FORWARDS(sl->update[l])[l];
			FORWARDS(sl->update[l])[l] = n;
			l--;
		}
		sl->n_nodes++;
	}

	return 0;
}

/* Remove the entry with the given key.  Returns the removed value if found,
 * otherwise returns NULL.
 */
void *skiplist_remove(SkipList *sl, char *key)
{
	SkipListNode *n = sl->header, *f;
	int l = sl->max_level - 1;
	void *ret = NULL;

	while (l >= 0) {
		for (;;) {
			f = FORWARDS(n)[l];
			if (!f || strcmp(f->key, key) >= 0) break;
			n = f;
		}
		sl->update[l] = n;
		l--;
	}
	n = FORWARDS(n)[0];

	if (n && !strcmp(n->key, key)) { /* key found; remove */
		ret = n->value;

		for (l = 0; l < sl->max_level; l++) {
			if (FORWARDS(sl->update[l])[l] != n) break;
			FORWARDS(sl->update[l])[l] = FORWARDS(n)[l];
		}
		free(n);
		sl->n_nodes--;
	}
	return ret;
}

/* Iterate through each skip list entry in order, with the given
 * callback function called for each node.
 */
void skiplist_each(SkipList *sl, void (*visit)(char *key, void *value, void *data),
                   void *data)
{
    SkipListNode *n = FORWARDS(sl->header)[0];
    while (n) {
        (visit)(n->key, n->value, data);
        n = FORWARDS(n)[0];
    }
}

/* Does a linear search through the skip list looking for a matching element
 * using a callback function.  The callback function gets key, value and 
 * data as arguments and returns non-zero if it has found the node it's looking
 * for or 0 otherwise.  The skiplist_search function then returns the value
 * that was identified by searchcb or NULL if no such value was identified.
 */
void *skiplist_search(SkipList *sl, int (*searchcb)(char *, void *, void *),
                     void *data)
{
    SkipListNode *n = FORWARDS(sl->header)[0];
    while (n) {
        if ((searchcb)(n->key, n->value, data))
            return n->value;
        n = FORWARDS(n)[0];
    }
    return NULL;
}
