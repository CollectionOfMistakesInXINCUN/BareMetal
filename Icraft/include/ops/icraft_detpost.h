/**
 * @file icraft_detpost.h
 * @author Junkun Wang (wangjunkun@fmsh.com.cn)
 * @brief Icraft3.x DetPost前行所需的信息、前向推理接口
 * @date 2024-05-24
 * 
 * @copyright Copyright (c) 2024 Shanghai Fudan Microelectronics Group Co., Ltd. All rights reserved.
 * 
 */

#ifndef _ICRAFT_DETPOST_H_
#define _ICRAFT_DETPOST_H_

#include "icraft_basic_op.h"
#include "icraft_ftmp.h"
#include "utils/icraft_hash_table.h"

/**
* @brief HardOp执行前向推理所需的信息
*/
typedef struct icraft_detpost_t{
	icraft_basic_op_info_t *basic_info;
	icraft_return (*free) (struct icraft_detpost_t* self);
	icraft_return (*forward) (struct icraft_detpost_t* self);
	icraft_return (*init) (struct icraft_detpost_t* self);

	int32_t detpost_layer_en;
	int32_t detpost_chn_1st;
	int32_t detpost_chn_2nd;
	int32_t detpost_chn_3rd;
	int32_t detpost_anchor_num;
	int32_t detpost_cmp_chn;
	int32_t detpost_cmp_en;
	int32_t detpost_cmp_mask0;
	int32_t detpost_cmp_mask1;
	int32_t detpost_valid_chn;
	uint32_t detpost_data_thr_len;
	int32_t *detpost_data_thr;
	uint64_t detpost_reg_base;

	uint32_t group_num; 
	uint32_t gic_round; 
	uint32_t anchor_box_bytesize; 
	uint32_t ftmp_num_per_group;
	uint32_t *result_data;      	//< 存放每个group输出的anchor_box个数
	uint32_t detpost_ps_base;   	//< detpost的base地址，64byte地址
	uint32_t mapped_base;       	//< group的ps的地址，byte地址
	uint32_t *post_result;      	//< 输出ftmp的首地址，byte地址           
} icraft_detpost_t;

icraft_return
icraft_detpost_init(icraft_detpost_t  *detpost);


icraft_return
icraft_detpost_forward(struct icraft_detpost_t* detpost);

icraft_return
icraft_gic_detpost_update_base(icraft_detpost_t *detpost);

icraft_return freeDetpost(struct icraft_detpost_t* self);


#endif // ! _ICRAFT_DETPOST_H_