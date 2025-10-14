/**
 * @file network.h
 * @author Junkun Wang (wangjunkun@fmsh.com.cn)
 * @brief Icraft3.x 中哈希表的定义，用于记录、查找Tensors
 * @date 2024-04-15
 * 
 * @copyright Copyright (c) 2024 Shanghai Fudan Microelectronics Group Co., Ltd. All rights reserved.
 * 
 */

#include "utils/icraft_hash_table.h"
#include "utils/icraft_print.h"

icraft_return
icraft_hashtable_create(icraft_hashtable_t **hash_table,
                        const uint32_t key_size,
                        const uint32_t value_size,
                        const uint32_t capacity,
                        icraft_hash_func_t hash_func) {
    // if((*hash_table) != NULL){
    //     return ICRAFT_HASHTABLE_DOUBLE_CREATE;
    // }
    *hash_table = (icraft_hashtable_t*)aligned_alloc(
        CACHE_ALIGN_SIZE, align64_sizeof(icraft_hashtable_t));
    if(*hash_table == NULL){
        return ICRAFT_HASHTABLE_MALLOC_ERROR;
    }
    (*hash_table)->capacity = capacity;
    (*hash_table)->buckets = (icraft_hashnode_t**)aligned_alloc(
        CACHE_ALIGN_SIZE, align64_sizeof(icraft_hashnode_t*) * capacity);
    if((*hash_table)->buckets == NULL){
        return ICRAFT_HASHTABLE_MALLOC_ERROR;
    }
    for (uint32_t i = 0; i < capacity; i++) {
        (*hash_table)->buckets[i] = NULL;
    }
    (*hash_table)->key_size = key_size;
    (*hash_table)->value_size = value_size;
    (*hash_table)->hash_func = *hash_func;
    (*hash_table)->len = 0;
    return ICRAFT_SUCCESS;
}

// uint32_t  
// icraft_hashtable_default_hashfunc(const char *str, const uint32_t table_size) {
//     unsigned long hash = 5381;
//     int c;
//     while ((c = *str++))
//         hash = ((hash << 5) + hash) + c;    // hash * 33 + c
//     return hash % table_size;
// }



icraft_return 
icraft_hashtable_insert(icraft_hashtable_t *table, void *key, void *value) {
    if(table == NULL){
        return ICRAFT_HASHTABLE_NOT_CREATE;
    }
    if(key == NULL){
        return ICRAFT_HASHTABLE_INVALID_KEY;
    }
    if(value == NULL){
        return ICRAFT_HASHTABLE_INVALID_VALUE;
    }
    uint32_t index = table->hash_func(key, table->key_size) % table->capacity;
    icraft_hashnode_t *node = table->buckets[index];
    BOOL exist = FALSE;
    while (node) {
        if (memcmp(node->key, key, table->key_size) == 0) {
            exist = TRUE;
            break;
        }
        node = node->next;
    }
    if (exist == TRUE) {
        node->value = value;
        return ICRAFT_SUCCESS;
    }
    node = (icraft_hashnode_t*)aligned_alloc(
        CACHE_ALIGN_SIZE, align64_sizeof(icraft_hashnode_t));
    if(node == NULL){
        return ICRAFT_HASHTABLE_MALLOC_ERROR;
    }
    //node->key = aligned_alloc(CACHE_ALIGN_SIZE, align64(table->key_size));
    //node->value = aligned_alloc(CACHE_ALIGN_SIZE, align64(table->value_size));
    //memcpy(node->key, key, table->key_size);
    //memcpy(node->value, value, table->value_size);
    node->key = key;
    node->value = value;
    node->next = table->buckets[index];
    table->buckets[index] = node;
    table->len++;
    return ICRAFT_SUCCESS;
}


icraft_return 
icraft_hashtable_search(icraft_hashtable_t *table, void *key, void **value) {
    if(key == NULL){
        return ICRAFT_HASHTABLE_INVALID_KEY;
    }
    if(table->hash_func == NULL){
        return ICRAFT_HASHTABLE_HASH_FUNC_NULL;
    }
    uint32_t index = table->hash_func(key, table->key_size) % table->capacity;
    icraft_hashnode_t *node = table->buckets[index];
    while (node) {
        if (memcmp(node->key, key, table->key_size) == 0) {
            (*value) = node->value;
            return ICRAFT_SUCCESS;
        }
        node = node->next;
    }
    return ICRAFT_HASHTABLE_KEY_NOT_EXIST;
}


void icraft_hashtable_destroy(icraft_hashtable_t* table) {
    for (uint32_t i = 0; i < table->capacity; ++i) {
        icraft_hashnode_t *node = table->buckets[i];
        while (node) {
            icraft_hashnode_t *temp = node;
            node = node->next;
            free(temp->key);
            free(temp->value);
            free(temp);
        }
    }
    free(table->buckets);
    free(table);
}


icraft_return
icraft_hashtable_create_iterator(icraft_hashtable_iterator_t **iter, icraft_hashtable_t *table)
{
    (*iter) = (icraft_hashtable_iterator_t*)aligned_alloc(CACHE_ALIGN_SIZE, align64_sizeof(icraft_hashtable_iterator_t));
    if((*iter) == NULL){
        icraft_print("malloc for hashtable iterator failed, malloc size: %u\r\n", align64_sizeof(icraft_hashtable_iterator_t));
        return ICRAFT_HASHTABLE_MALLOC_ERROR;
    }
    (*iter)->table = table;
    (*iter)->current_bucket_idx = 0;
    (*iter)->current_node = table->buckets[0];
    return ICRAFT_SUCCESS;
}


icraft_return 
icraft_hashtable_has_next(icraft_hashtable_iterator_t *iter, icraft_bool_t* has_next)
{
    while(iter->current_bucket_idx < iter->table->capacity){
        if(iter->current_node != NULL){
            (*has_next) = ICRAFT_TRUE;
            return ICRAFT_SUCCESS;
        }
        // 如果当前node为空，则找下一个桶
        iter->current_bucket_idx ++;
        if(iter->current_bucket_idx < iter->table->capacity){
            iter->current_node = iter->table->buckets[iter->current_bucket_idx];  // 指向新的桶的首节点
        }
    }
    (*has_next) = ICRAFT_FALSE;
    return ICRAFT_SUCCESS;
}


icraft_return 
icraft_hashtable_next(icraft_hashtable_iterator_t *iter, icraft_hashnode_t **current_node)
{
    icraft_hashnode_t *cur_node = iter->current_node;
    if(cur_node != NULL){
        iter->current_node = cur_node->next;
    }
    (*current_node) = cur_node;
    return ICRAFT_SUCCESS;
}

