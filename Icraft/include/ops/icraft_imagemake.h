/**
 * @file icraft_imagemake.h
 * @author Junkun Wang (wangjunkun@fmsh.com.cn)
 * @brief Icraft3.x ImageMake前行所需的信息、前向推理接口
 * @date 2024-04-10
 * 
 * @copyright Copyright (c) 2024 Shanghai Fudan Microelectronics Group Co., Ltd. All rights reserved.
 * 
 */

#ifndef _ICRAFT_IMAGEMAKE_H_
#define _ICRAFT_IMAGEMAKE_H_

#include "icraft_type.h"
#include "icraft_errno.h"
#include "icraft_basic_op.h"
#include "icraft_network.h"
#include "icraft_version.h"




/**
* @brief HardOp执行前向推理所需的信息
*/
typedef struct icraft_imagemake_t{
    icraft_basic_op_info_t *basic_info;
    icraft_return (*free) (struct icraft_imagemake_t* self);
    icraft_return (*forward) (struct icraft_imagemake_t* self);
    icraft_return (*init) (struct icraft_imagemake_t* self);

    uint64_t   imk_reg_base;
    uint32_t   imk_input_port;
    uint32_t   imk_pad_size_len;
    uint32_t   *imk_pad_size;
    uint32_t   imk_pre_mean_len;
    float     *imk_pre_mean;
    uint32_t   imk_pre_scale_len;
    float     *imk_pre_scale;
    BOOL       imk_one_fast;
    int32_t    imk_input_bits;

    uint32_t   data_arrange;   //0:8bit;1:16bit
    uint32_t   *hw_msc;
    uint32_t   channel_times;
    uint32_t   channel_each;
    uint32_t   channel;
    uint32_t   height;
    uint32_t   width;
    uint32_t   wddr_base;
    uint32_t   pad_reg;
    uint64_t   reg_base;
} icraft_imagemake_t;


icraft_return
icraft_imagemake_init(icraft_imagemake_t * imagemake);

icraft_return
icraft_imagemake_forward(struct icraft_imagemake_t* imagemake);

/*
icraft_return
icraft_imagemake_preset(icraft_imagemake_t const * const imagemake);
*/
icraft_return freeImagemake(struct icraft_imagemake_t* self);



#endif // ! _ICRAFT_IMAGEMAKE_H_