#include "ops/icraft_hardop.h"
#include "icraft_network.h"
#include <math.h>

#define TIMEOUT_THRES  (10000)

//typedef icraft_return (*icraft_ftmp_sync_func_t)();

// #define icraft_ftmp_check_func(x) \
// icraft_return icraft_ftmp_check_func_##x(){ \
// icraft_return ret; \
// uint32_t layer_count; \
// double counts = TIMEOUT_THRES * 1000 * GTC_CLK_FREQ; \
// uint64_t begin = get_current_time; \ 
// while((get_current_time() - begin) < counts) { \
//     ret = icraft_device_get_layercount(&layer_count); \
//     if(ret){ \
//         icraft_print("get layer count failed, err code: %u\r\n", ret); \
//         return ICRAFT_HARDOP_GET_LAYERCOUNT_FAIL; \
//     } \
//     if(layer_count >= (x)){ \
//         return ICRAFT_SUCCESS; \
//     } \
//     icraft_print("sync failed, layer count=%u, sync index=%u\r\n", layer_count, (x)); \
//     return ICRAFT_HARDOP_SYNC_FAIL; \
// } \
// }


icraft_return 
icraft_hardop_forward(struct icraft_hardop_t* hardop)
{

    icraft_return dev_ret;
    icraft_return hardop_ret;
    icraft_return ftmp_ret;
    uint32_t ifms_num = hardop->basic_info->ifms_num;
    uint32_t ofms_num = hardop->basic_info->ofms_num;
    icraft_ftmp_info_t **ifms = hardop->basic_info->ifms_ptr;
    icraft_ftmp_info_t **ofms = hardop->basic_info->ofms_ptr;
    for(uint32_t i = 0; i < ifms_num; ++i){
        if(ifms[i] == NULL){
            fmsh_print("[HARDOP_FORWARD] op(id=%u), ifms[%u]=NULL\r\n", 
                hardop->basic_info->op_id, i);
            return ICRAFT_HARDOP_INVALID_INPUT_FTMPS;
        }
    }
    for(uint32_t i = 0; i < ofms_num; ++i){
        if(ofms[i] == NULL){
            fmsh_print("[HARDOP_FORWARD] op(id=%u), ofms[%u]=NULL\r\n", 
                hardop->basic_info->op_id, i);
            return ICRAFT_HARDOP_INVALID_OUTPUT_FTMPS;
        }
    }
    if(hardop->basic_info->compile_target == ICRAFT_COMPILE_TARGET_CPU){
        fmsh_print("[HARDOP_FORWARD] unsupported hardop compile target, op id=%u, compile target=%u\r\n",
            hardop->basic_info->op_id, (uint32_t)hardop->basic_info->compile_target);
        return ICRAFT_HARDOP_INVALID_COMPILE_TARGET;
    }

    hardop->basic_info->network_p->current_op = hardop->basic_info->op_id;
    
   
        //fmsh_print("hardop: op_id:%u start\r\n", hardop->basic_info->op_id);
    
 
    // icore sync each input ftmp
    float memcpy_time = 0;
    for(uint32_t i = 0; i < ifms_num; ++i){

        if(ifms[i]->mtype == (uint8_t)ICRAFT_MTYPE_HOST) {
            fmsh_print("[HARDOP_FORWARD] ifm(vid=%u) mtype is host, please check snapshot\r\n");
            return ICRAFT_HARDOP_INVALID_INPUT_MTYPE;
        }
        /*
        if((ifms[i]->cur_mtype == (uint8_t)ICRAFT_MTYPE_HOST) && (ifms[i]->mtype == (uint8_t)ICRAFT_MTYPE_ETM)){

            dev_ret = icraft_device_write_plmem(ifms[i]->addr, ifms[i]->data, ifms[i]->size);
            if(dev_ret){
                fmsh_print("[HARDOP_FORWARD] memcpy ifm(vid=%u) from host to plddr failed, err code: %u\r\n", ifms[i]->vid, dev_ret);
                return ICRAFT_HARDOP_MEMCPY_FAIL;
            }

            ifms[i]->cur_mtype = ICRAFT_MTYPE_ETM;
        }
        */
    }

    if (hardop->basic_info->compile_target == ICRAFT_COMPILE_TARGET_FPGA) {
        uint64_t reg_base = 0x100001400;
        uint32_t instr_addr = hardop->instr_phy_addr;

        uint32_t input_byte_size = (uint32_t)(ceilf((float)ifms[0]->size / 64.F) * 64.F);

        icraft_device_write_reg_relative(reg_base + 0x14, 1);
        icraft_device_write_reg_relative(reg_base + 0x14, 0);
        icraft_device_write_reg_relative(reg_base + 0x18, instr_addr);
        icraft_device_write_reg_relative(reg_base + 0x1C, ifms[0]->addr);
        icraft_device_write_reg_relative(reg_base + 0x20, ifms[0]->addr + input_byte_size);
        icraft_device_write_reg_relative(reg_base + 0x24, ofms[0]->addr);
        icraft_device_write_reg_relative(reg_base + 0x28, ofms[0]->addr + input_byte_size);
        icraft_device_write_reg_relative(reg_base, 1);
        
        
        /*
        global_timer_enable();
        double counts = TIMEOUT_THRES * 1000 * GTC_CLK_FREQ;
        uint64_t begin = get_current_time();
        uint32_t done_sig = 0;
        while(done_sig == 0){
            if((get_current_time() - begin) > counts){
                fmsh_print("FPGA HardOp op_id:%d timeout...\r\n", hardop->basic_info->op_id);
                return ICRAFT_HARDOP_FPGA_TIMEOUT;
            }
            icraft_device_read_reg_relative(reg_base + 0x04, &done_sig);
        }

        uint32_t v1 = 0;
        uint32_t v2 = 0;
        
            //icraft_device_read_reg_relative(0x100002C00, &v1);
            icraft_device_read_reg_relative(0x100002C04, &v2);
            fmsh_print("gic reg base:%u, base+4:%u\r\n", v1, v2);
        */
        
        

    return  ICRAFT_SUCCESS;
    }

    
    // mmu mode
    uint32_t mmu_mode;
    dev_ret = icraft_device_mmu_get_mode(&mmu_mode);
    if(dev_ret){
        fmsh_print("[HARDOP_FORWARD] get mmu mode failed, err code: %u\r\n", dev_ret);
        return ICRAFT_HARDOP_GET_MMU_MODE_FAIL;
    }
    if (mmu_mode != hardop->basic_info->network_p->mmu_mode) {
        fmsh_print("[HARDOP_FORWARD] network mmu_mode: %u not match device mmu_mode: %u err code: %u\r\n",
         hardop->basic_info->network_p->mmu_mode, mmu_mode);
        return ICRAFT_HARDOP_MMU_MODE_NOT_MATCH;
    }
#if ICRAFT_DEBUG
    icraft_print("[HARDOP_FORWARD] mmu mode: %u\r\n", mmu_mode);
#endif



    // set dtype
    uint8_t dtype = ifms[0]->dtype;
    if(dtype == ICRAFT_DTYPE_UINT8 || dtype == ICRAFT_DTYPE_UINT16 || dtype == ICRAFT_DTYPE_FP32){
        icraft_print("[HARDOP_FORWARD] invalid dtype: %u, see icraft_dtype.h\r\n", dtype);
        return ICRAFT_HARDOP_INVALID_FTMP_DTYPE;
    }
    dev_ret = icraft_device_set_dtype(dtype);
    if(dev_ret){
        icraft_print("[HARDOP_FORWARD] set op(id=%u) dtype=%u failed, device err code: %u\r\n", 
            hardop->basic_info->op_id, dtype, dev_ret);
        return ICRAFT_HARDOP_SET_DTYPE_FAIL;
    }
#if ICRAFT_DEBUG
    icraft_print("[HARDOP_FORWARD] set op(id=%u) dtype=%u success\r\n", hardop->basic_info->op_id, dtype);
#endif
    //set row&col

    icraft_device_enable_mpe(hardop->basic_info->network_p->mpe_rows, hardop->basic_info->network_p->mpe_rows);

    // start icore
    if(hardop->instr_size != 0){
#if ICRAFT_DEBUG
        icraft_print("[HARDOP_FORWARD] set time log layer cnt: %u\r\n", hardop->sync_index + hardop->layer_count);
#endif
#if ICRAFT_LOG_TIME
        dev_ret = icraft_device_write_reg_relative(0xCC, hardop->sync_index + hardop->layer_count);
        if(dev_ret){
            fmsh_print("[HARDOP_FORWARD] set layercount for log time failed, device err code: %u\r\n", dev_ret);
            return ICRAFT_HARDOP_FORWARD_FAIL;
        }
#endif
        if(mmu_mode == (uint32_t)ICRAFT_DEVICE_MMU_OPEN) {
            icraft_device_mmu_table_t *mmu_table  = hardop->basic_info->network_p->mmu_table;
            if (mmu_table == NULL) {
                fmsh_print("[HARDOP_FORWARD] mmu table is null, error code: %u\r\n", 
                ICRAFT_HARDOP_NULL_MMU_TABLE);
                return ICRAFT_HARDOP_NULL_MMU_TABLE;
            }
#if ICRAFT_DEBUG
            icraft_print("[HARDOP_FORWARD] icore run hardop(id=%u), instr addr: %u, instr size: %u, mmu mode: %u\r\n", 
                hardop->basic_info->op_id, hardop->instr_logic_addr, hardop->instr_size, mmu_mode);
#endif
            dev_ret = icraft_device_mmu_update_table(mmu_table);
            if(dev_ret){
                fmsh_print("[HARDOP_FORWARD] update mmu table for HARDOP(id=%u) failed, device err code: %u\r\n", 
                    hardop->basic_info->op_id, dev_ret);
                return ICRAFT_HARDOP_FORWARD_FAIL;
            }
            dev_ret = icraft_device_icore_calc(hardop->instr_logic_addr, hardop->instr_size);
            if(dev_ret){
                fmsh_print("[HARDOP_FORWARD] icore run forward failed, err code: %u\r\n", dev_ret);
                return ICRAFT_HARDOP_FORWARD_FAIL;
            }
#if ICRAFT_DEBUG
            icraft_print("[HARDOP_FORWARD] start icore forward op(id=%u) success\r\n", hardop->basic_info->op_id);
#endif
        }
        if(mmu_mode == (uint32_t)ICRAFT_DEVICE_MMU_CLOSE){
#if ICRAFT_DEBUG
            icraft_print("[HARDOP_FORWARD] icore run hardop(id=%u), instr addr: %u, instr size: %u, mmu mode: %u\r\n", 
                hardop->basic_info->op_id, hardop->instr_phy_addr, hardop->instr_size, mmu_mode);
#endif
            dev_ret = icraft_device_icore_calc(hardop->instr_phy_addr, hardop->instr_size);
            if(dev_ret){
                fmsh_print("[HARDOP_FORWARD] icore run forward failed, err code: %u\r\n", dev_ret);
                return ICRAFT_HARDOP_FORWARD_FAIL;
            }
#if ICRAFT_DEBUG
            icraft_print("[HARDOP_FORWARD] start icore forward op(id=%u) success\r\n", hardop->basic_info->op_id);
#endif
        }
    }
    else{
#if ICRAFT_DEBUG
        icraft_print("[HARDOP_FORWARD] hardop(id=%u), instr size = %u, no need to start icore\r\n", 
            hardop->basic_info->op_id, hardop->instr_size);
#endif        
    }
    // prepare output
    uint32_t sync_idx = hardop->sync_index;
    for(uint32_t i = 0; i < ofms_num; ++i){
        uint32_t ofm_vid = hardop->basic_info->ofms[i]; 
        ofms[i]->sync_idx = hardop->sync_index + hardop->layer_count;
#if ICRAFT_DEBUG
        icraft_print("[HARDOP(%u)] set ftmp(vid=%u) sync index with (%u)\r\n", 
            hardop->basic_info->op_id, ofms[i]->vid, ofms[i]->sync_idx);
#endif
    }
    /*
    for(uint32_t i = 0; i < ofms_num; ++i){
        ftmp_ret = icraft_ftmp_sync(ofms[i]->sync_idx);
        if(ftmp_ret){
            uint32_t layer_count;
            icraft_device_get_layercount(&layer_count);
            icraft_print("[HARDOP_FORWARD] HARDOP(%u) sync ofm(vid=%u) failed, icore layer count: %u, sync idx: %u, ftmp err code: %u\r\n", 
            hardop->basic_info->op_id, ofms[i]->vid, layer_count, ofms[i]->sync_idx, ftmp_ret);
            return ICRAFT_HARDOP_SYNC_FAIL;
        }
    }
    */

    return ICRAFT_SUCCESS;
}

icraft_return freeHardop(struct icraft_hardop_t* self) {
    
    icraft_return ret = self->basic_info->free(self->basic_info);
    free(self->basic_info);
    if (ret) {
        icraft_print("[HARDOP FREE ERROR] error code %u\r\n", ret);
        return ICRAFT_HARDOP_DECONSTRUCT_FAIL;
    }
    return ICRAFT_SUCCESS;
}

icraft_return icraft_hardop_init(struct icraft_hardop_t* self) {
    icraft_return ret;
    if (self->basic_info->compile_target == ICRAFT_COMPILE_TARGET_FPGA) {
        uint32_t hardop_fpga_version;
        icraft_device_read_reg_relative(HARDOP_FPGA_REG_BASE + 0x008, &hardop_fpga_version);
        if(hardop_fpga_version != ICRAFT_HARDOP_VERSION){
            icraft_print("[HARDOP_INIT] detect no valid hardop fpga hardware, got one with version: 0x%X\r\n", hardop_fpga_version);
            return ICRAFT_HARDOP_INVALID_VERSION;
        }
    }
    return ICRAFT_SUCCESS;
}