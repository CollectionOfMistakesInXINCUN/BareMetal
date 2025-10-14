/**
 * @file hardop.h
 * @author Junkun Wang (wangjunkun@fmsh.com.cn)
 * @brief Icraft3.x HardOp前行所需的信息、前向推理接口
 * @date 2024-04-10
 * 
 * @copyright Copyright (c) 2024 Shanghai Fudan Microelectronics Group Co., Ltd. All rights reserved.
 * 
 */

#ifndef _ICRAFT_HARDOP_H_
#define _ICRAFT_HARDOP_H_

#include "icraft_basic_op.h"
#include "icraft_ftmp.h"
#include "utils/icraft_print.h"


/**
* @brief HardOp执行前向推理所需的信息
*/
typedef struct icraft_hardop_t{
    icraft_basic_op_info_t *basic_info;
    icraft_return (*free) (struct icraft_hardop_t* self);
    icraft_return (*forward) (struct icraft_hardop_t* self);
    icraft_return (*init) (struct icraft_hardop_t* self);

    uint32_t   sync_index;         ///< hardop的同步index
    uint32_t   layer_count;        ///< hardop导致的ICORE执行计数的变化量
    uint8_t     *instr_data;        ///< hardop的指令数据
    uint8_t     *param_data;        ///< hardop的权重数据
    uint8_t    dtype;              ///< hardop的精度，INT8或INT16
    uint32_t   instr_logic_addr;   ///< hardop指令起始逻辑地址（mmu模式）
    uint32_t   instr_phy_addr;     ///< hardop指令起始物理地址
    uint32_t   instr_size;         ///< 指令的字节大小
    uint32_t   param_addr;         ///< 权重的物理地址（与是否开mmu无关）
    uint32_t   param_size;         ///< 权重的字节大小
} icraft_hardop_t;


icraft_return 
icraft_hardop_forward(struct icraft_hardop_t* hardop);

icraft_return 
icraft_hardop_init(struct icraft_hardop_t* hardop);

icraft_return freeHardop(struct icraft_hardop_t* self);


#endif // ! _ICRAFT_HARDOP_H_