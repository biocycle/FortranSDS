#ifndef SKIPLIST_H
#define SKIPLIST_H

typedef struct SkipListNode SkipListNode;
struct SkipListNode {
	char *key;
	void *value;
    SkipListNode *forwards[];
};

typedef struct {
	SkipListNode *header, **update;
    int n_nodes;   // number of nodes (entries) in the skip list
	int max_level; // internal; initial level of the forward pointer array
} SkipList;

SkipList *skiplist_new(void);
void skiplist_free(SkipList *sl, void (*free_value)(void *));

void *skiplist_find(SkipList *sl, char *key);
int skiplist_add(SkipList *sl, char *key, void *value);
void *skiplist_remove(SkipList *sl, char *key);

void skiplist_each(SkipList *sl, void (*visit)(char *key, void *value));
void *skiplist_search(SkipList *sl,
                      int (*searchcb)(char *key, void *value, void *data),
                      void *data);

#endif /* SKIPLIST_H */
