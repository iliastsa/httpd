#include <string.h>
#include <stdlib.h>

#include "str_map.h"

// Hash string by summing the characters.
static
size_t hash(char *key){
    size_t sum = 0;

    for (size_t i = 0; i < strlen(key); ++i)
        sum += key[i];

    return sum%STR_HASH_SIZE;
}

// Simple initialization.
void init_str_map(StrHashMap *map){
    memset(map, 0, sizeof(StrHashMap));
}

// Lookup string in StrHashMap
char *lookup_str_map(StrHashMap *map, char *key){
    long idx = hash(key);

    StrHashNode *node = map->buckets[idx];
    while (node != NULL)
        if (!strcmp(node->key, key))
            return node->val;
        else
            node = node->next;

    return NULL;
}

// Uniquely insert string in StrHashMap.
char insert_str_map(StrHashMap *map, char *key, char *val){
    // It already exists, do not insert.
    if (lookup_str_map(map, key) != NULL)
        return 0;

    long idx = hash(key);
    StrHashNode *bucket = map->buckets[idx];

    StrHashNode *new_node = (StrHashNode*)malloc(sizeof(StrHashNode));
    if (new_node == NULL)
        return -1;

    // Setup fields in new node
    new_node->key  = key;
    new_node->val  = val;
    new_node->next = bucket;

    map->buckets[idx] = new_node;

    map->n_entries++;

    return 1;
}

// Free hashmap.
void free_str_map(StrHashMap *map, char free_content){
    // If map is NULL, return
    if (map == NULL)
        return;

    // For every bucket in our hashmap
    for (int i = 0; i < STR_HASH_SIZE; ++i) {
        StrHashNode *node = map->buckets[i];

        // For each node in the bucket
        while (node != NULL) {
            // If the free content flag is set, free key and value
            if (free_content) {
                free(node->key);
                free(node->val);
            }

            // Free current node
            StrHashNode *next = node->next;
            free(node);

            // Move ahead in node list
            node = next;
        }
    }

    free(map);
}
