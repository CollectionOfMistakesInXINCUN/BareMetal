/**
 * @file icraft_output.h
 * @author Junkun Wang (wangjunkun@fmsh.com.cn)
 * @brief Icraft3.x Output前行所需的信息、前向推理接口
 * @date 2024-04-11
 * 
 * @copyright Copyright (c) 2024 Shanghai Fudan Microelectronics Group Co., Ltd. All rights reserved.
 * 
 */

#ifndef _ICRAFT_OUTPUT_H_
#define _ICRAFT_OUTPUT_H_

#include "icraft_basic_op.h"
#include "icraft_type.h"
#include "icraft_errno.h"
#include "icraft_ftmp.h"


typedef struct {
    uint32_t normratio_len;
    float *normratios;      
} icraft_normratio_t;


/**
* @brief Input执行前向推理所需的信息
*/
typedef struct icraft_output_t{
    icraft_basic_op_info_t *basic_info;
    icraft_return (*free) (struct icraft_output_t* self);
    icraft_return (*forward) (struct icraft_output_t* self);
    icraft_return (*init) (struct icraft_output_t* self);

    // uint32_t normratios_num;
    // icraft_normratio_t *normratios;   ///< 输出层的归一化系数，由于硬件输出得到的是定点数，后处理往往需要通过归一化系数将结果转化为浮点数
} icraft_output_t;

icraft_return 
icraft_output_forward(struct icraft_output_t* output);

icraft_return 
icraft_output_init(struct icraft_output_t* output);

icraft_return freeOutput(struct icraft_output_t* self);

#endif // ! _ICRAFT_OUTPUT_H_