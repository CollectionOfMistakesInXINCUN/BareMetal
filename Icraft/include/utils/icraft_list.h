/**
 * @file list.h
 * @author Junkun Wang (wangjunkun@fmsh.com.cn)
 * @brief Icraft3.x 中链表节点的创建、添加、删除
 * @date 2024-04-11
 * 
 * @copyright Copyright (c) 2024 Shanghai Fudan Microelectronics Group Co., Ltd. All rights reserved.
 * 
 */


#ifndef _ICRAFT_LIST_H_
#define _ICRAFT_LIST_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "icraft_errno.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 单向链表节点的前向声明
 * 
 */
typedef struct icraft_list_node_t icraft_list_node_t;


/**
 * @brief 单向链表结构体
 * 
 */
struct icraft_list_node_t{
    void* data;
    struct icraft_list_node_t* next;
};

/**
 * @brief 创建链表节点
 * 
 * @param [in] node  需要被创建的链表节点的二级指针
 * @param [in] data  被创建列表所存放的数据
 * 
 * @return 链表相关的错误码
 * @retval 成功时返回ICRAFT_SUCCESS，失败时返回其它错误状态，详见icraft_errno.h
 */
icraft_return
icraft_list_node_create(icraft_list_node_t **node, void *data);

/**
 * @brief 在链表末尾添加新节点
 * 
 * @param [in] node  需要在末尾添加新节点的起始节点
 * @param [in] data  新节点的数据
 * 
 * @return 链表相关的错误码
 * @retval 成功时返回ICRAFT_SUCCESS，失败时返回其它错误状态，详见icraft_errno.h
 */
icraft_return 
icraft_list_append(icraft_list_node_t **node, void *data);


/**
 * @brief 获取链表的长度
 * 
 * @param [in]  node  链表起始节点
 * @param [out] len   链表长度 
 * 
 * @return 链表相关的错误码
 * @retval 成功时返回ICRAFT_SUCCESS，失败时返回其它错误状态，详见icraft_errno.h
 */
icraft_return
icraft_list_len(icraft_list_node_t *node, uint32_t *len);


/**
 * @brief 删除链表并释放内存
 * 
 * @param [in] node  需要被删除链表的起始节点
 * 
 * @return 无
 */
void icraft_list_destroy(icraft_list_node_t* node);

#ifdef __cplusplus
}
#endif

#endif // _ICRAFT_LIST_H_