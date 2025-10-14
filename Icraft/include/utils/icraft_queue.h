/**
 * @file icraft_print.h
 * @author Junkun Wang (wangjunkun@fmsh.com.cn)
 * @brief Icraft3.x Icraft工程中使用的队列及其方法
 * @date 2024-06-30
 * 
 * @copyright Copyright (c) 2023 Shanghai Fudan Microelectronics Group Co., Ltd. All rights reserved.
 * 
 */

#ifndef _ICRAFT_QUEUE_H_
#define _ICRAFT_QUEUE_H_

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "icraft_errno.h"
#include "icraft_type.h"
#include "icraft_cache.h"
#include "icraft_common.h"
#include "icraft_print.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 队列节点的前向声明
 * 
 */
typedef struct icraft_queue_node_t icraft_queue_node_t;


/**
 * @brief 队列节点结构体
 * 
 */
struct icraft_queue_node_t{
    void *data;                             //> 指向节点数据的指针
    struct icraft_queue_node_t *next;       //> 指向下一个节点
};

/**
 * @brief 队列结构体
 * 
 */
typedef struct{
    icraft_queue_node_t *front;      //> 队列的首节点
    icraft_queue_node_t *rear;       //> 队列的尾节点
}icraft_queue_t;


/**
 * @brief   初始化队列 
 * 
 * @param   [in]      queue        待初始化的队列（二级指针） 
 * @return  错误码
 * @retval  成功时返回ICRAFT_SUCCESS，失败时返回其它错误状态，详见icraft_errno.h      
 */
icraft_return
icraft_queue_init(icraft_queue_t **queue);


/**
 * @brief   释放队列 
 * 
 * @param   [in]      queue        待初始化的队列 
 * 
 * @return  错误码
 * @retval  成功时返回ICRAFT_SUCCESS，失败时返回其它错误状态，详见icraft_errno.h      
 */
icraft_return
icraft_queue_destroy(icraft_queue_t *queue);

/**
 * @brief   向队列添加节点
 * 
 * @param   [in]     queue       目标队列
 * @param   [in]     data        添加节点的数据指针
 * 
 * @return  错误码
 * @retval  成功时返回ICRAFT_SUCCESS，失败时返回其它错误状态，详见icraft_errno.h  
 */
icraft_return
icraft_queue_enqueue(icraft_queue_t *queue, void *data);

/**
 * @brief   从队列弹出节点
 * 
 * @param   [in]     queue       目标队列
 * @param   [out]    data        弹出的数据
 * 
 * @return  错误码
 * @retval  成功时返回ICRAFT_SUCCESS，失败时返回其它错误状态，详见icraft_errno.h  
 */
icraft_return
icraft_queue_dequeue(icraft_queue_t *queue, void **data);

/**
 * @brief   判断队列是否为空
 * 
 * @param   [in]     queue       目标队列
 * @param   [out]    result      若队列为空，则为ICRAFT_TRUE；如队列非空，则为ICRAFT_FALSE
 * 
 * @return  错误码
 * @retval  成功时返回ICRAFT_SUCCESS，失败时返回其它错误状态，详见icraft_errno.h  
 */
icraft_return
icraft_queue_is_empty(icraft_queue_t *queue, icraft_bool_t *result);


/**
 * @brief   获取队列的长度（节点个数）
 * 
 * @param   [in]     queue       目标队列
 * @param   [out]    size        队列的长度
 * 
 * @return  错误码
 * @retval  成功时返回ICRAFT_SUCCESS，失败时返回其它错误状态，详见icraft_errno.h  
 */
icraft_return
icraft_queue_size(icraft_queue_t *queue, uint32_t *size);


#ifdef __cplusplus
}
#endif // __cplusplus
#endif // _ICRAFT_QUEUE_H_