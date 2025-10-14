#include "ops/icraft_input.h"
#include "utils/icraft_print.h"
#include "icraft_network.h"

icraft_return 
icraft_input_forward(struct icraft_input_t* input)
{
#if PLIN
    return ICRAFT_SUCCESS;
#endif
#if ICRAFT_LOG_TIME
    global_timer_enable();
    uint64_t interface_time_begin = get_current_time();
#endif
    icraft_return dev_ret;
    icraft_return input_ret;
    icraft_return ftmp_ret;
    // check invalidation
    uint32_t ifms_num = input->basic_info->ifms_num;
    uint32_t ofms_num = input->basic_info->ofms_num;
    icraft_ftmp_info_t **ifms = input->basic_info->ifms_ptr;
    icraft_ftmp_info_t **ofms = input->basic_info->ofms_ptr;
    for(uint32_t i = 0; i < ifms_num; ++i){
        if(ifms[i] == NULL){
            icraft_print("op(id=%u), ifms[%u]=NULL\r\n", 
                input->basic_info->op_id, i);
            return ICRAFT_INPUT_INVALID_INPUT_FTMPS;
        }
    }
    for(uint32_t i = 0; i < ofms_num; ++i){
        if(ofms[i] == NULL){
            icraft_print("op(id=%u), ofms[%u]=NULL\r\n", 
                input->basic_info->op_id, i);
            return ICRAFT_INPUT_INVALID_OUTPUT_FTMPS;
        }
    }
    if((input->basic_info->compile_target == ICRAFT_COMPILE_TARGET_FPGA) || 
       (input->basic_info->compile_target == ICRAFT_COMPILE_TARGET_BUYI))
    {
        icraft_print("unsupported input compile target, op id=%u, compile target=%u\r\n",
            input->basic_info->op_id, (uint32_t)input->basic_info->compile_target);
        return ICRAFT_INPUT_INVALID_COMPILE_TARGET;
    }

    input->basic_info->network_p->current_op = input->basic_info->op_id;
#if ICRAFT_LOG_TIME
    uint64_t interface_time_end = get_current_time();
    input->basic_info->time_log->interface_time = (float)(interface_time_end - interface_time_begin) / 1000.0 / GTC_CLK_FREQ;
#endif

/*
#if DUMP_AFTER_EACH_OP
    for(uint32_t i = 0; i < input->basic_info->ofms_num; ++i){
        ftmp_ret = icraft_ftmp_dump(input->basic_info->ofms_ptr[i], input->basic_info->network_p->name);
        if(ftmp_ret){
            icraft_print("[INPUT] dump INPUT(opid=%u) ofm(vid=%u) failed, ftmp err code: %u\r\n", 
                input->basic_info->op_id, input->basic_info->ofms_ptr[i]->vid, ftmp_ret);
            return ICRAFT_NETWORK_DUMP_FAIL;
        }
        icraft_print("[INPUT] dump ofms for INPUT(opid=%u) success!\r\n", input->basic_info->op_id);
    }
#endif
*/

    return ICRAFT_SUCCESS;
}

icraft_return freeInput(struct icraft_input_t* self) {
    
    icraft_return ret = self->basic_info->free(self->basic_info);
    free(self->basic_info);
    if (ret) {
        icraft_print("[INPUT FREE ERROR] error code %u\r\n", ret);
        return ICRAFT_INPUT_DECONSTRUCT_FAIL;
    }
    return ICRAFT_SUCCESS;
}

icraft_return 
icraft_input_init(struct icraft_input_t* inputs) {
    return ICRAFT_SUCCESS;
}