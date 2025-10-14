#include "ops/icraft_detpost.h"
#include "icraft_network.h"


icraft_return
icraft_detpost_init(icraft_detpost_t  *detpost)
{
    uint32_t detpost_version;
    uint32_t right_version = 0x20220623;
    uint64_t reg_base = detpost->detpost_reg_base;
    if(reg_base == 0){
        reg_base = 0x100000800;
    }
    icraft_print("[DETPOST] reg base: 0x%llX\r\n", reg_base);

    icraft_return dev_ret;
    dev_ret = icraft_device_read_reg_relative(reg_base + 0x068, &detpost_version);
    if(dev_ret){
        icraft_print("[DETPOST] get version failed, reg: 0x%llX\r\n", reg_base + 0x068);
        return ICRAFT_DETPOST_GET_VERSION_FAIL;
    }
    if(detpost_version != right_version){
        icraft_print("[DETPOST] wrong version of detpost, get(0x%X), should be (0x%X)\r\n", detpost_version, right_version);
        return ICRAFT_DETPOST_INVALID_VERSION;
    }
    icraft_print("[DETPOST] check detpost version(=0x%X) success!\r\n", detpost_version);

    detpost->result_data = (uint32_t*)aligned_alloc(CACHE_ALIGN_SIZE, (detpost->basic_info->ofms_num + 1) * sizeof(uint32_t));
    dev_ret = icraft_device_write_reg_relative(reg_base + 0x004, 1);

    int IcorePostLayerEn = detpost->detpost_layer_en;
	int IcorePostChn1st = detpost->detpost_chn_1st - 1;
	int IcorePostChn2nd = detpost->detpost_chn_2nd - 1;
	int IcorePostChn3rd = detpost->detpost_chn_3rd - 1;
	int IcorePostAnchorNum =detpost->detpost_anchor_num - 1;
	int IcorePostCmpEn = detpost->detpost_cmp_en;
	int Mask0 = detpost->detpost_cmp_mask0;
	int Mask1 = detpost->detpost_cmp_mask1;
	int IcorePostValidChn = detpost->detpost_valid_chn - 1;
	int IcorePostCmpChn = detpost->detpost_cmp_chn - 1;

    uint32_t ftmp_num_per_group  = 0;
    uint32_t anchor_box_bytesize = 0;

    
    dev_ret = icraft_device_write_reg_relative(reg_base + 0x004, 0);
    dev_ret = icraft_device_write_reg_relative(reg_base + 0x010, IcorePostLayerEn);
    dev_ret = icraft_device_write_reg_relative(reg_base + 0x028, IcorePostChn1st);
    dev_ret = icraft_device_write_reg_relative(reg_base + 0x02C, IcorePostChn2nd);
    dev_ret = icraft_device_write_reg_relative(reg_base + 0x030, IcorePostChn3rd);
    dev_ret = icraft_device_write_reg_relative(reg_base + 0x038, IcorePostAnchorNum);
    dev_ret = icraft_device_write_reg_relative(reg_base + 0x048,  IcorePostCmpEn);
    dev_ret = icraft_device_write_reg_relative(reg_base + 0x050, Mask0);
    dev_ret = icraft_device_write_reg_relative(reg_base + 0x054, Mask1);
    dev_ret = icraft_device_write_reg_relative(reg_base + 0x05C, IcorePostValidChn);
    dev_ret = icraft_device_write_reg_relative(reg_base + 0x074, IcorePostCmpChn);
    if(dev_ret){
        icraft_print("init detpost hardwares fail, device err code: %u\r\n", dev_ret);
        return ICRAFT_DETPOST_START_FAIL;
    }   
    detpost->post_result = (uint32_t*)aligned_alloc(CACHE_ALIGN_SIZE, 5 * 1024 * 1024);
    memset(detpost->post_result, 0, 5 * 1024 * 1024);
    Fmsh_DCacheFlushRange((uint32_t)detpost->post_result, 5 * 1024 * 1024);     
    icraft_print("malloc addr:0x%llX, detpost set addr:0x%llX\r\n", 
        detpost->post_result, (uint64_t)(detpost->post_result) >> 3);
    detpost->mapped_base = 0;
    detpost->gic_round = 0;
    return ICRAFT_SUCCESS;
}


icraft_return
icraft_detpost_forward(struct icraft_detpost_t* detpost)
{
    icraft_return dev_ret;
    icraft_return detpost_ret;
    icraft_return ftmp_ret;

    uint32_t ifms_num = detpost->basic_info->ifms_num;
    uint32_t ofms_num = detpost->basic_info->ofms_num;
    icraft_ftmp_info_t **ifms = detpost->basic_info->ifms_ptr;
    icraft_ftmp_info_t **ofms = detpost->basic_info->ofms_ptr;
    for(uint32_t i = 0; i < ifms_num; ++i){
        if(ifms[i] == NULL){
            fmsh_print("op(id=%u), ifms[%u]=NULL\r\n", 
                detpost->basic_info->op_id, i);
            return ICRAFT_DETPOST_INVALID_INPUT_FTMPS;
        }
    }
    for(uint32_t i = 0; i < ofms_num; ++i){
        if(ifms[i] == NULL){
            fmsh_print("op(id=%u), ofms[%u]=NULL\r\n", 
                detpost->basic_info->op_id, i);
            return ICRAFT_DETPOST_INVALID_OUTPUT_FTMPS;
        }
    }
    if((detpost->basic_info->compile_target == ICRAFT_COMPILE_TARGET_CPU) || 
       (detpost->basic_info->compile_target == ICRAFT_COMPILE_TARGET_BUYI))
    {
        fmsh_print("unsupported compile target for DETPOST(opid=%u), current compile target=%u, must be ICRAFT_COMPILE_TARGET_FPGA.\r\n",
            detpost->basic_info->op_id, (uint32_t)detpost->basic_info->compile_target);
        return ICRAFT_DETPOST_INVALID_COMPILE_TARGET;
    }

    detpost->basic_info->network_p->current_op = detpost->basic_info->op_id;

    int IcorePostLayerEn = detpost->detpost_layer_en;
	int IcorePostChn1st = detpost->detpost_chn_1st - 1;
	int IcorePostChn2nd = detpost->detpost_chn_2nd - 1;
	int IcorePostChn3rd = detpost->detpost_chn_3rd - 1;
	int IcorePostAnchorNum =detpost->detpost_anchor_num - 1;
	int IcorePostCmpEn = detpost->detpost_cmp_en;
	int Mask0 = detpost->detpost_cmp_mask0;
	int Mask1 = detpost->detpost_cmp_mask1;
	int IcorePostValidChn = detpost->detpost_valid_chn - 1;
	int IcorePostCmpChn = detpost->detpost_cmp_chn - 1;
    uint64_t reg_base_ = detpost->detpost_reg_base;
    uint64_t reg_base = 0x100000800;

    uint64_t IcorePostPlBase_vec[3] = {reg_base + 0x14, reg_base + 0x18, reg_base + 0x01C};
    uint64_t IcorePostAnchorStr_vec[3] = { reg_base + 0x3C, reg_base + 0x40, reg_base + 0x44 };
    uint32_t IcorePostChn_vec[3] = { IcorePostChn1st, IcorePostChn2nd, IcorePostChn3rd };

	if (reg_base_ != 0) {
		reg_base = reg_base_;
	}


    if(detpost->gic_round == 0){
        detpost->detpost_ps_base = (uint32_t)detpost->post_result >> 3;
        icraft_print("[DETPOST] round 0, ps base: 0x%llX\r\n", (uint32_t)detpost->detpost_ps_base << 3);
        Fmsh_DCacheFlushRange((uint32_t)detpost->post_result, 5 * 1024 * 1024);
    }

    icraft_dtype_t dtype = ifms[0]->dtype;
    if(dtype==ICRAFT_DTYPE_SINT8){
        dtype = 0;
    }
    else if(dtype == ICRAFT_DTYPE_SINT16){
        dtype = 1;
    }
    else{
        fmsh_print("invalid dtype: %u, see icraft_dtype_t\r\n", dtype);
        return ICRAFT_DETPOST_INVALID_DTYPE;
    }
#if ICRAFT_DEBUG
    icraft_print("[DETPOST] dtype: %u\r\n", dtype);
#endif
if(detpost->gic_round == 0){
    uint32_t ftmp_num_per_group = 0;			// 每组有多少特征图
	uint32_t anchor_box_ByteSize = 0;		// 每个anchor有多少比特
	if (IcorePostLayerEn == 1) {
		ftmp_num_per_group = 1;
		anchor_box_ByteSize = (IcorePostChn1st * 8 + (IcorePostValidChn + 1) + IcorePostCmpEn) * 8;
	}
	else if (IcorePostLayerEn == 3) {
		ftmp_num_per_group = 2;
		anchor_box_ByteSize = ((IcorePostChn1st + 1) * 8 + IcorePostChn2nd * 8 + (IcorePostValidChn + 1) + IcorePostCmpEn) * 8;
	}
	else {
		ftmp_num_per_group = 3;
		anchor_box_ByteSize = ((IcorePostChn1st + 1 + IcorePostChn2nd + 1) * 8 + IcorePostChn3rd * 8 + (IcorePostValidChn + 1) + IcorePostCmpEn) * 8;
	}


    detpost->anchor_box_bytesize = anchor_box_ByteSize;
	detpost->ftmp_num_per_group = ftmp_num_per_group;
	detpost->group_num = ifms_num / ftmp_num_per_group;	// 一共分了多少组
			

    dev_ret = icraft_device_write_reg_relative(reg_base + 0x004, 0);
    dev_ret = icraft_device_write_reg_relative(reg_base + 0x010, IcorePostLayerEn);
    dev_ret = icraft_device_write_reg_relative(reg_base + 0x028, IcorePostChn1st);
    dev_ret = icraft_device_write_reg_relative(reg_base + 0x02C, IcorePostChn2nd);
    dev_ret = icraft_device_write_reg_relative(reg_base + 0x030, IcorePostChn3rd);
    dev_ret = icraft_device_write_reg_relative(reg_base + 0x038, IcorePostAnchorNum);
    dev_ret = icraft_device_write_reg_relative(reg_base + 0x048,  IcorePostCmpEn);
    dev_ret = icraft_device_write_reg_relative(reg_base + 0x050, Mask0);
    dev_ret = icraft_device_write_reg_relative(reg_base + 0x054, Mask1);
    dev_ret = icraft_device_write_reg_relative(reg_base + 0x05C, IcorePostValidChn);
    dev_ret = icraft_device_write_reg_relative(reg_base + 0x074, IcorePostCmpChn);
    if(dev_ret){
        fmsh_print("init detpost hardwares fail, device err code: %u\r\n", dev_ret);
        return ICRAFT_DETPOST_START_FAIL;
    }   
 }

    uint32_t IcorePostPsBase = detpost->detpost_ps_base;
    icraft_return hash_ret;
    
    uint32_t group_index = detpost->gic_round;

    float hw_time = 0;   
   
        //获取ftmp，得到HW
        uint32_t this_group_base = group_index * detpost->ftmp_num_per_group;

        // hard code
        uint32_t h = ifms[this_group_base]->shape[2];
        uint32_t w = ifms[this_group_base]->shape[3];
        for(uint32_t ftmp_index = 0; ftmp_index < detpost->ftmp_num_per_group; ++ftmp_index){     
            icraft_device_write_reg_relative(IcorePostPlBase_vec[ftmp_index], (ifms[this_group_base + ftmp_index]->addr) >> 5);
            icraft_device_write_reg_relative(IcorePostAnchorStr_vec[ftmp_index], h*w*(IcorePostChn_vec[ftmp_index]+1)*2);
        }
        icraft_device_write_reg_relative(reg_base + 0x020, w-1);
        icraft_device_write_reg_relative(reg_base + 0x024, h-1);
        icraft_device_write_reg_relative(reg_base + 0x034, h*w*2);
        icraft_device_write_reg_relative(reg_base + 0x04C, detpost->detpost_data_thr[group_index]);
        icraft_device_write_reg_relative(reg_base + 0x058, IcorePostPsBase);
        icraft_device_write_reg_relative(reg_base + 0x080, h*w*4+dtype);
        icraft_device_write_reg_relative(reg_base + 0x008, 1);

        icraft_print("[DETPOST] round %u success! group num: %u\r\n", detpost->gic_round, detpost->group_num);
    
        detpost->gic_round = detpost->gic_round + 1;


//         global_timer_enable();
//         double counts = TIMEOUT_THRES * 1000 * GTC_CLK_FREQ;
//         uint64_t begin = get_current_time();
//         uint32_t done_sig = 0;
//         while(done_sig == 0){
//             if((get_current_time() - begin) > counts){
//                 fmsh_print("detpost timeout, group index=%u\r\n", group_index);
//                 return ICRAFT_DETPOST_TIME_OUT;
//             }
//             icraft_device_read_reg_relative(reg_base + 0x00C, &done_sig);
//         }
            
//         icraft_device_write_reg_relative(reg_base + 0x00C, 1);

//         uint32_t detpost_hw_time_cnt;
//         uint32_t detpost_gt_th_num;
//         uint32_t detpost_post_num;
//         uint32_t detpost_wr_num;
//         icraft_device_read_reg_relative(reg_base + 0x060, &detpost_hw_time_cnt);
//         icraft_device_read_reg_relative(reg_base + 0x070, &detpost_gt_th_num);
//         icraft_device_read_reg_relative(reg_base + 0x078, &detpost_post_num);
//         icraft_device_read_reg_relative(reg_base + 0x07C, &detpost_wr_num);
//         float r = (1 - (float)detpost_gt_th_num / (h * w * (detpost->detpost_anchor_num + 1))) * 100;
//         double detpost_hw_time = (detpost_hw_time_cnt * 5) / 1000000.0;
//         if((detpost_gt_th_num != detpost_post_num) || 
//             ((detpost_gt_th_num * anchor_box_ByteSize) != detpost_wr_num * 8)){
//             fmsh_print("detpost processed an incorrect amount of data\r\n");
//             fmsh_print("box num should be %u, but got %u\r\n", detpost_gt_th_num, detpost_post_num);
//             fmsh_print("box data size should be %u, but got %u\r\n", 
//                 detpost_gt_th_num * anchor_box_ByteSize, detpost_wr_num * 8);
//             return ICRAFT_DETPOST_WRONG_DATA_SIZE;
//         }
                
//         detpost->result_data[group_index] = detpost_gt_th_num;
// #if ICRAFT_DEBUG
//         icraft_print(">>>>>>> DETPOST group index %u result info\r\n", group_index);
//         icraft_print(">>>>>>> %u anchor boxes greater than threshold, filt %.2f%% boxes\r\n", 
//             detpost_gt_th_num, r);
//         icraft_print(">>>>>>> each anchor length is %u bytes\r\n", anchor_box_ByteSize);
//         icraft_print(">>>>>>> group %u PL time counts is %u, equals to hard time %.2f ms\r\n", 
//             group_index, detpost_hw_time_cnt, detpost_hw_time);
//         icraft_print(">>>>>>> mapped base addr: %llX\r\n", mapped_base_);
// #endif
// #if ICRAFT_LOG_TIME
//         hw_time += detpost_hw_time;
// #endif
//         // 构造输出：将ofms的数据指针指向当前group的ps起始地址
        
//         ofms[group_index]->data = (uint8_t*)(mapped_base_ + mapped_base);
//         ofms[group_index]->size = detpost_gt_th_num * anchor_box_ByteSize;
        
//         // 更新ps地址
//         IcorePostPsBase = IcorePostPsBase + detpost_gt_th_num * anchor_box_ByteSize / 8;
//         mapped_base_ = mapped_base_ + detpost_gt_th_num * anchor_box_ByteSize;   
// #if ICRAFT_DEBUG
//         icraft_print("[DETPOST] group %u, updated ps_base: 0x%llX, updated mapped_base: 0x%llX\r\n", 
//                 group_index, IcorePostPsBase, mapped_base_);
// #endif
//         if(group_index == group_num - 1){
//             icraft_device_write_reg_relative(reg_base + 0x058, mapped_base_ >> 3);
//         }            

    
//     icraft_print("DETPOST forward success!\r\n");
// #if ICRAFT_LOG_TIME
//     uint64_t interface_time_end = get_current_time();
//     detpost->basic_info->time_log->hw_time = hw_time;
//     detpost->basic_info->time_log->interface_time = (float)(interface_time_end - interface_time_begin) / 1000.0 / GTC_CLK_FREQ;
// #endif
    return ICRAFT_SUCCESS;
}

icraft_return freeDetpost(struct icraft_detpost_t* self) {
    free(self->post_result);
    free(self->result_data);
    
    icraft_return ret = self->basic_info->free(self->basic_info);
    free(self->basic_info);
    if (ret) {
        fmsh_print("[DETPOST FREE ERROR] error code %u\r\n", ret);
        return ICRAFT_DETPOST_DECONSTRUCT_FAIL;
    }
    return ICRAFT_SUCCESS;
}

icraft_return
icraft_gic_detpost_update_base(icraft_detpost_t *detpost){
    icraft_return ret;
    uint64_t reg_base = detpost->detpost_reg_base;
    if(reg_base == 0){
        reg_base = 0x100000800;
    }

    uint32_t group_index = detpost->gic_round - 1;
    uint32_t reg_post_time_cnt;
    uint32_t reg_post_gt_th_num;
    uint32_t reg_post_post_num;
    uint32_t reg_post_wr_num;

    icraft_ftmp_info_t **ifms = detpost->basic_info->ifms_ptr;
    icraft_ftmp_info_t **ofms = detpost->basic_info->ofms_ptr;
    uint32_t this_group_base = group_index * detpost->ftmp_num_per_group;
    uint32_t h = ifms[this_group_base]->shape[2];
    uint32_t w = ifms[this_group_base]->shape[3];

    ret = icraft_device_read_reg_relative(reg_base + 0x060, &reg_post_time_cnt);
    ret = icraft_device_read_reg_relative(reg_base + 0x070, &reg_post_gt_th_num);
    ret = icraft_device_read_reg_relative(reg_base + 0x078, &reg_post_post_num);
    ret = icraft_device_read_reg_relative(reg_base + 0x07C, &reg_post_wr_num);

	detpost->result_data[group_index] = reg_post_gt_th_num;
    float r = (1 - (float)reg_post_gt_th_num / (h * w * (detpost->detpost_anchor_num + 1))) * 100;
    float detpost_hw_time = (reg_post_time_cnt * 5) / 1000000.0;
    detpost->basic_info->time_log->hw_time += detpost_hw_time;
    icraft_print(">>>>>>> DETPOST group index %u result info\r\n", group_index);
    icraft_print(">>>>>>> %u anchor boxes greater than threshold, filt %.2f%% boxes\r\n", 
             reg_post_gt_th_num, r);
            
	
	if (reg_post_gt_th_num != reg_post_post_num) {
		icraft_print("ERROR : reg_post_gt_th_num != reg_post_post_num, reg_post_gt_th_num is : %d, reg_post_post_num is : %d\n", 
            reg_post_gt_th_num, reg_post_post_num);
	}
	if ((reg_post_gt_th_num * detpost->anchor_box_bytesize) != reg_post_wr_num * 8) {
		icraft_print("ERROR : anchor_box_TotalByteSize != reg_post_wr_num*8, anchor_box_TotalByteSize is : %d, reg_post_wr_num*8 is : %d\n", 
            (reg_post_gt_th_num * detpost->anchor_box_bytesize), reg_post_wr_num * 8);
	}
    ofms[group_index]->data = (uint8_t*)((uint32_t)detpost->detpost_ps_base << 3);
    ofms[group_index]->size = reg_post_gt_th_num * detpost->anchor_box_bytesize;
    detpost->detpost_ps_base = detpost->detpost_ps_base + reg_post_gt_th_num * detpost->anchor_box_bytesize / 8;
    if (group_index == (detpost->group_num - 1)) {
        icraft_device_write_reg_relative(reg_base + 0x058, detpost->detpost_ps_base);
    }
    icraft_print("[DETPOST UPDATE] group_index: %u, reg_post_gt_th_num: %u, ps base: 0x%llX\r\n", 
            group_index,reg_post_gt_th_num, (uint32_t)detpost->detpost_ps_base << 3);
    
    return ret;
    
}