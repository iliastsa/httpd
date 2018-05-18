#ifndef STR_MAP_H
#define STR_MAP_H

#define STR_HASH_SIZE 1543

typedef struct str_hash_node{
    char *key;
    char *val;
    struct str_hash_node *next;
} StrHashNode;

typedef struct {
    StrHashNode *buckets[STR_HASH_SIZE];
    long n_entries;
} StrHashMap;

void init_str_map(StrHashMap *map);
char insert_str_map(StrHashMap *map, char *key, char *val);
char *lookup_str_map(StrHashMap *map, char *str);
void print_str_map(StrHashMap *map);
void free_str_map(StrHashMap *map, char free_content);
#endif
