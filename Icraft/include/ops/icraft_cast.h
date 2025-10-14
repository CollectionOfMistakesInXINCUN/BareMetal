/**
 * @file hardop.h
 * @author Junkun Wang (wangjunkun@fmsh.com.cn)
 * @brief Icraft3.x HardOp前行所需的信息、前向推理接口
 * @date 2024-04-10
 * 
 * @copyright Copyright (c) 2024 Shanghai Fudan Microelectronics Group Co., Ltd. All rights reserved.
 * 
 */

#ifndef _ICRAFT_CAST_H_
#define _ICRAFT_CAST_H_

#include "icraft_type.h"
#include "icraft_errno.h"
#include "ops/icraft_basic_op.h"


/**
* @brief Cast执行前向推理所需的信息
*/
typedef struct icraft_cast_t{
    icraft_basic_op_info_t *basic_info;
    icraft_return (*free) (struct icraft_cast_t* self);
    icraft_return (*forward) (struct icraft_cast_t* self);
    icraft_return (*init) (struct icraft_cast_t* self);
} icraft_cast_t;


icraft_return
icraft_cast_forward(struct icraft_cast_t* cast);

icraft_return
icraft_cast_init(struct icraft_cast_t* cast);

icraft_return freeCast(struct icraft_cast_t* self);

#endif // ! _ICRAFT_CAST_H_