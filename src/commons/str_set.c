#include <string.h>
#include <stdlib.h>

#include "utils.h"
#include "str_set.h"

// Hash string by summing the characters.
static
size_t hash(char *key){
    size_t sum = 0;

    for (size_t i = 0; i < strlen(key); ++i)
        sum += key[i];

    return sum%STR_SET_SIZE;
}

// Simple initialization.
void init_str_set(StrSet *map){
    memset(map, 0, sizeof(StrSet));
}

// Lookup string in StrSet
char lookup_str_set(StrSet *map, char *key){
    long idx = hash(key);

    StrSetNode *node = map->buckets[idx];
    while (node != NULL)
        if (!strcmp(node->key, key))
            return 1;
        else
            node = node->next;

    return 0;
}

// Uniquely insert string in StrSet
char insert_str_set(StrSet *map, char *key){
    // It already exists, do not insert.
    if (lookup_str_set(map, key) != 0)
        return 0;

    long idx = hash(key);
    StrSetNode *bucket = map->buckets[idx];

    StrSetNode *new_node = (StrSetNode*)malloc(sizeof(StrSetNode));
    if (new_node == NULL)
        return -1;

    // Setup fields in new node
    new_node->key  = key;
    new_node->next = bucket;

    map->buckets[idx] = new_node;

    map->n_entries++;

    return 1;
}

void print_str_set(StrSet *map) {
    // If map is NULL, return
    if (map == NULL)
        return;

    // For every bucket in our hashmap
    for (int i = 0; i < STR_SET_SIZE; ++i) {
        StrSetNode *node = map->buckets[i];

        // For each node in the bucket
        while (node != NULL) {
            P_DEBUG("Key : (%s)\n", node->key);
            node = node->next;
        }
    }
}

// Free StrSet
void free_str_set(StrSet *map, char free_content){
    // If map is NULL, return
    if (map == NULL)
        return;

    // For every bucket in our hashmap
    for (int i = 0; i < STR_SET_SIZE; ++i) {
        StrSetNode *node = map->buckets[i];

        // For each node in the bucket
        while (node != NULL) {
            // If the free content flag is set, free key and value
            if (free_content) {
                free(node->key);
            }

            // Free current node
            StrSetNode *next = node->next;
            free(node);

            // Move ahead in node list
            node = next;
        }
    }

    free(map);
}
