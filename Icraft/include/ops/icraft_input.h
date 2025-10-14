/**
 * @file input.h
 * @author Junkun Wang (wangjunkun@fmsh.com.cn)
 * @brief Icraft3.x Input前行所需的信息、前向推理接口
 * @date 2024-04-11
 * 
 * @copyright Copyright (c) 2024 Shanghai Fudan Microelectronics Group Co., Ltd. All rights reserved.
 * 
 */

#ifndef _ICRAFT_INPUT_H_
#define _ICRAFT_INPUT_H_

#include "icraft_type.h"
#include "icraft_errno.h"

#include "icraft_ftmp.h"
#include "icraft_basic_op.h"

/**
* @brief Input执行前向推理所需的信息
*/
typedef struct icraft_input_t{
    icraft_basic_op_info_t *basic_info;
    icraft_return (*free) (struct icraft_input_t* self);
    icraft_return (*forward) (struct icraft_input_t* self);
    icraft_return (*init) (struct icraft_input_t* self);
} icraft_input_t;


icraft_return 
icraft_input_forward(struct icraft_input_t* inputs);

icraft_return 
icraft_input_init(struct icraft_input_t* inputs);

icraft_return freeInput(struct icraft_input_t* self);

#endif // _ICRAFT_INPUT_H_