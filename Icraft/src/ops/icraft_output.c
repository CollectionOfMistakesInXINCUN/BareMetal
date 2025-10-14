#include "ops/icraft_basic_op.h"
#include "ops/icraft_output.h"
#include "utils/icraft_print.h"
#include "icraft_network.h"

icraft_return 
icraft_output_forward(struct icraft_output_t* output)
{
#if ICRAFT_LOG_TIME
    global_timer_enable();
    uint64_t interface_time_begin = get_current_time();
#endif
    icraft_return dev_ret;
    icraft_return output_ret;
    icraft_return ftmp_ret;
    // check invalidation
    uint32_t ifms_num = output->basic_info->ifms_num;
    uint32_t ofms_num = output->basic_info->ofms_num;
    icraft_ftmp_info_t **ifms = output->basic_info->ifms_ptr;
    icraft_ftmp_info_t **ofms = output->basic_info->ofms_ptr;
    for(uint32_t i = 0; i < ifms_num; ++i){
        if(ifms[i] == NULL){
            fmsh_print("op(id=%u), ifms[%u]=NULL\r\n", 
                output->basic_info->op_id, i);
            return ICRAFT_OUTPUT_INVALID_INPUT_FTMPS;
        }
    }
    for(uint32_t i = 0; i < ofms_num; ++i){
        if(ofms[i] == NULL){
            fmsh_print("op(id=%u), ofms[%u]=NULL\r\n", 
                output->basic_info->op_id, i);
            return ICRAFT_OUTPUT_INVALID_OUTPUT_FTMPS;
        }
    }
    if((output->basic_info->compile_target == ICRAFT_COMPILE_TARGET_FPGA) || 
       (output->basic_info->compile_target == ICRAFT_COMPILE_TARGET_BUYI))
    {
        fmsh_print("unsupported input compile target, op id=%u, compile target=%u\r\n",
            output->basic_info->op_id, (uint32_t)output->basic_info->compile_target);
        return ICRAFT_OUTPUT_INVALID_COMPILE_TARGET;
    }

    output->basic_info->network_p->current_op = output->basic_info->op_id;
/*
    //sync
    for(uint32_t i = 0; i < ifms_num; ++i){
        icraft_print("start sync op(id=%u) ifm(vid=%u), sync idx: %u\r\n",
            output->basic_info->op_id, ifms[i]->vid, ifms[i]->sync_idx);
        ftmp_ret = icraft_ftmp_sync(ifms[i]->sync_idx);
        if(ftmp_ret){
            uint32_t layer_count;
            icraft_device_get_layercount(&layer_count);
            icraft_print("sync op(id=%u) ifm(vid=%u) failed, icore layer count: %u, sync idx: %u, ftmp err code: %u\r\n", 
            output->basic_info->op_id, ifms[i]->vid, layer_count, ifms[i]->sync_idx, ftmp_ret);
            return ICRAFT_OUTPUT_SYNC_FAIL;
        }
        icraft_print("sync op(id=%u) ifm(vid=%u) success.\r\n", output->basic_info->op_id, ifms[i]->vid);
    }
*/
#if ICRAFT_LOG_TIME
    uint64_t interface_time_end = get_current_time();
    output->basic_info->time_log->interface_time = (float)(interface_time_end - interface_time_begin) / 1000.0 / GTC_CLK_FREQ;
#endif
    return ICRAFT_SUCCESS;
}

icraft_return freeOutput(struct icraft_output_t* self) {
    
    icraft_return ret = self->basic_info->free(self->basic_info);
    free(self->basic_info);
    if (ret) {
        icraft_print("[OUTPUT FREE ERROR] error code %u\r\n", ret);
        return ICRAFT_OUTPUT_DECONSTRUCT_FAIL;
    }
    return ICRAFT_SUCCESS;
}

icraft_return 
icraft_output_init(struct icraft_output_t* output) {
    return ICRAFT_SUCCESS;
}