#ifndef STR_SET_H
#define STR_SET_H

#define STR_SET_SIZE 1543

typedef struct str_set_node{
    char *key;
    struct str_set_node *next;
} StrSetNode;

typedef struct {
    StrSetNode *buckets[STR_SET_SIZE];
    long n_entries;
} StrSet;

void init_str_set(StrSet *map);
char insert_str_set(StrSet *map, char *key);
char lookup_str_set(StrSet *map, char *key);
void print_str_set(StrSet *map);
void free_str_set(StrSet *map, char free_content);
#endif
