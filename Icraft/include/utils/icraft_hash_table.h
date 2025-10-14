/**
 * @file network.h
 * @author Junkun Wang (wangjunkun@fmsh.com.cn)
 * @brief Icraft3.x 中哈希表的定义，用于记录、查找Tensors
 * @date 2024-04-15
 * 
 * @copyright Copyright (c) 2024 Shanghai Fudan Microelectronics Group Co., Ltd. All rights reserved.
 * 
 */

#ifndef _ICRAFT_HASHTABLE_H_
#define _ICRAFT_HASHTABLE_H_

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "icraft_errno.h"

#include "icraft_common.h"
#include "icraft_cache.h"
#include "icraft_type.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 函数指针，指向哈希表使用的哈希函数
 * 
 */
typedef uint32_t(*icraft_hash_func_t)(void*, uint32_t);


/**
* @brief 哈希表节点结构
*/
typedef struct icraft_hashnode_t{
    void *key;                   ///< 键
    void *value;                 ///< 值
    struct icraft_hashnode_t *next;
}icraft_hashnode_t;


/**
* @brief 哈希表结构（链地址法）
*/
typedef struct icraft_hashtable_t{
    icraft_hashnode_t **buckets;          ///< 指向哈希桶数组的指针，每个bucket维护一个单项链表
    uint32_t capacity;                    ///< 哈希表键值对个数
    uint32_t key_size;
    uint32_t value_size;
    uint32_t len;
    icraft_hash_func_t  hash_func;        ///< 哈希函数
}icraft_hashtable_t;


/**
* @brief  创建新的哈希表
* 
* @param  [out]  hash_table       待创建的哈希表
* @param  [in]   capacity         哈希表元素个数
* @param  [in]   key_size         key类型占的字节数
* @param  [in]   value_size       value类型占的字节数
* @param  [in]   hash_func        哈希函数
* 
* @return 错误码
* @retval 成功时返回ICRAFT_SUCCESS，失败时返回其它错误状态，详见icraft_errno.h
*/
icraft_return
icraft_hashtable_create(icraft_hashtable_t **hash_table, 
                        const uint32_t key_size,
                        const uint32_t value_size,
                        const uint32_t capacity,
                        icraft_hash_func_t hash_func);


/**
* @brief  将新的键值对插入到已有哈希表中
* 
* @param  [in]  table          需要插入键值对的哈希表
* @param  [in]  key            插入节点的键
* @param  [in]  value          插入节点的值
* 
* @return 错误码  
* @retval 成功时返回ICRAFT_SUCCESS，失败时返回其它错误状态，详见icraft_errno.h
*/
icraft_return 
icraft_hashtable_insert(icraft_hashtable_t *table, void *key, void *value);


/**
* @brief  给定键，查找哈希表中的对应的值
* 
* @param  [in]   table          需要插入键值对的哈希表
* @param  [in]   key            被查找节点的键
* @param  [out]  value          被查找节点的值
* 
* @return 错误码
* @retval 成功时返回ICRAFT_SUCCESS，失败时返回其它错误状态，详见icraft_errno.h
* 
* @note   key必须是变量！
*/
icraft_return 
icraft_hashtable_search(icraft_hashtable_t *table, void *key, void **value);


/**
* @brief  释放哈希表资源
* 
* @param  [in]   table       需要被释放的哈希表
* 
* @return 无
*/
void icraft_hashtable_destroy(icraft_hashtable_t *table);



/**
 * @brief  给哈希表添加新节点
 * 
 * @param  [in]   table      需要创建新节点的哈希表
 * @param  [in]   key        新节点的key 
 * @param  [in]   value      新节点的value
 * 
 * @return 错误码
 * @retval 成功时返回ICRAFT_SUCCESS，失败时返回其它错误状态，详见icraft_errno.h
 * 
 * @note   内部接口
 */
icraft_return 
_icraft_hashtable_create_node(icraft_hashtable_t *table, void *key, void *value);


/**
 * @brief 哈希表迭代器
 */
typedef struct {
    icraft_hashtable_t *table;           //> 遍历的哈希表
    icraft_hashnode_t *current_node;     //> 当前节点
    uint32_t current_bucket_idx;         //> 当前桶的索引          
}icraft_hashtable_iterator_t;


/**
 * @brief  创建哈希表的迭代器
 * 
 * @param  [in]    iter       带创建的迭代器  
 * @param  [in]    table      需要被遍历的哈希表
 * 
 * @return 错误码
 * @retval 成功时返回ICRAFT_SUCCESS，失败时返回其它错误状态，详见icraft_errno.h
 */
icraft_return
icraft_hashtable_create_iterator(icraft_hashtable_iterator_t **iter, icraft_hashtable_t *table);


/**
 * @brief   判断当前迭代器所处的节点是否还具有后继节点
 * 
 * @param   [in]    iter       迭代器 
 * @param   [out]   has_next   是否有后继节点
 * 
 * @return  错误码
 * @retval  成功时返回ICRAFT_SUCCESS，失败时返回其它错误状态，详见icraft_errno.h
 * 
 * @note    后继节点也可以为NULL，因此该接口也可以理解为当前迭代器是否有效。
 */
icraft_return 
icraft_hashtable_has_next(icraft_hashtable_iterator_t *iter, icraft_bool_t *has_next);


/**
 * @brief  迭代器向后步进一个节点，并返回当前节点
 * 
 * @param   [in]    iter        迭代器 
 * @param   [out]   next_node   当前节点
 * 
 * @return 错误码
 * @retval 成功时返回ICRAFT_SUCCESS，失败时返回其它错误状态，详见icraft_errno.h
 */
icraft_return 
icraft_hashtable_next(icraft_hashtable_iterator_t *iter, icraft_hashnode_t **current_node);


/**
 * @brief  销毁迭代器
 * 
 * @param   [in]   iter      需要被销毁的迭代器 
 * 
 * @return 错误码
 * @retval 成功时返回ICRAFT_SUCCESS，失败时返回其它错误状态，详见icraft_errno.h
 */
icraft_return 
icraft_hashtable_destroy_iterator(icraft_hashtable_iterator_t *iter);


#ifdef __cplusplus
}
#endif  // __cplusplus

#endif // _ICRAFT_HASHTABLE_H_