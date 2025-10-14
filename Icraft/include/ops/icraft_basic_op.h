/**
 * @file icraft_basic_op.h
 * @author Junkun Wang (wangjunkun@fmsh.com.cn)
 * @brief Icraft3.x 所有算子都会具备的公共信息
 * @date 2024-04-10
 * 
 * @copyright Copyright (c) 2024 Shanghai Fudan Microelectronics Group Co., Ltd. All rights reserved.
 * 
 */

#ifndef _ICRAFT_BASIC_OP_H_
#define _ICRAFT_BASIC_OP_H_

#include <stddef.h>

#include "icraft_type.h"
#include "icraft_errno.h"
#include "icraft_ftmp.h"
#include "icraft_cfg.h"

extern char const *g_icraft_op_type_names[];

#define icraft_timer_convert_to_ms(a, b) (float)((a) - (b)) / 1000.0 / GTC_CLK_FREQ

typedef struct{
    float  interface_time;  //< 前向总时间
    float  memcpy_time;     //< 数据拷贝时间
    float  hw_time;         //< 硬件时间
}icraft_time_log_t;


/**
* @brief HardOp执行前向推理所需的信息
*/
typedef struct icraft_basic_op_info_t{
    uint8_t    op_type;            ///< op类型，目前包含：
    uint8_t    compile_target;     ///< op的计算资源类型, 参考icraft_compile_target_t
    uint32_t   op_id;              ///< op id
    uint32_t   ifms_num;           ///< ifm的个数
    uint32_t   ofms_num;           ///< ofm的个数
    uint32_t   *ifms;              ///< 所有ifm的vid
    uint32_t   *ofms;              ///< 所有ofm的vid
    icraft_ftmp_info_t **ifms_ptr; ///< 存放所有ifm对象地址的指针数组
    icraft_ftmp_info_t **ofms_ptr; ///< 存放所有ofm对象地址的指针数组
    struct icraft_network_info_t* network_p;

    icraft_time_log_t *time_log;  

    icraft_return (*free)(struct icraft_basic_op_info_t* self);
} icraft_basic_op_info_t;


typedef struct icraft_basic_op_t{
    icraft_basic_op_info_t *basic_info;
    icraft_return (*free) (struct icraft_basic_op_t* self);
    icraft_return (*forward) (struct icraft_basic_op_t* self);
    icraft_return (*init) (struct icraft_basic_op_t* self);
    
}icraft_basic_op_t;

icraft_return freeBasicOperation(struct icraft_basic_op_info_t* op);

icraft_return icraft_get_op_type(icraft_basic_op_t const *op, char *name);

#endif // ! _ICRAFT_BASIC_OP_H_