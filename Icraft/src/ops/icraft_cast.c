#include "ops/icraft_cast.h"
#include "icraft_network.h"
#include <math.h>

const char* const_layout = "NHWCIOnhwcio*";

typedef struct {
	uint32_t ct;
	uint32_t cu;
	uint32_t element_num_before_ct;
	uint32_t element_num_between_cuct;
} layout_info;

int32_t getIdxOf(char* src, char find, uint32_t src_len) {
    int32_t idx = 0;
    if (strchr(const_layout, find) == NULL) return -1;
    if (strchr(src, find) == NULL) return -2;
    for(int i = 0; i < src_len; i++) {
        if (src[i] == find) return idx;
        if (strchr(const_layout, src[i]) != NULL) {
            idx += 1;
        }
    }
}

void getLayoutOInfo(const icraft_ftmp_info_t* ftmp, layout_info* layout_t) {
    char* layout = ftmp->layout;
    uint32_t layout_len = ftmp->layout_len;
    int32_t ct_index = getIdxOf(layout, 'C', layout_len);
    int32_t cu_index = getIdxOf(layout, 'c', layout_len);
    if (ct_index < 0) {
      fmsh_print("wrong ct_index:%i in layout:%s\r\n", ct_index, layout);
      fmsh_print("wrong cu_index:%i in layout:%s\r\n", cu_index, layout);
    }
    int32_t ct = ftmp->shape[ct_index];
    int32_t cu = ftmp->shape[cu_index];

    int32_t element_num_between_cuct = 1; 
    int32_t element_num_before_ct = 1;
    for(uint32_t i = 0; i < ct_index; ++i){
	    element_num_before_ct *= ftmp->shape[i];
    }
    for(uint32_t i = ct_index + 1; i < cu_index; ++i){
	    element_num_between_cuct *= ftmp->shape[i];
    }

    layout_t->ct = ct;
    layout_t->cu = cu;
    layout_t->element_num_before_ct = element_num_before_ct;
    layout_t->element_num_between_cuct = element_num_between_cuct;

    return;
}


icraft_return
icraft_cast_forward(struct icraft_cast_t* cast) {

    if (cast->basic_info->compile_target == ICRAFT_COMPILE_TARGET_CPU) {
        return ICRAFT_SUCCESS;
    }
    icraft_return dev_ret;
    icraft_return cast_ret;
    icraft_return ftmp_ret;
    uint32_t ifms_num = cast->basic_info->ifms_num;
    uint32_t ofms_num = cast->basic_info->ofms_num;
    icraft_ftmp_info_t **ifms = cast->basic_info->ifms_ptr;
    icraft_ftmp_info_t **ofms = cast->basic_info->ofms_ptr;
    for(uint32_t i = 0; i < ifms_num; ++i){
        if(ifms[i] == NULL || ifms[i]->mtype != ICRAFT_MTYPE_ETM){
            icraft_print("op(id=%u), ifms[%u]=NULL\r\n", 
                cast->basic_info->op_id, i);
            return ICRAFT_CAST_INVALID_INPUT_FTMPS;
        }
    }
    for(uint32_t i = 0; i < ofms_num; ++i){
        if(ofms[i] == NULL || ofms[i]->mtype != ICRAFT_MTYPE_ETM){
            icraft_print("op(id=%u), ofms[%u]=NULL\r\n", 
                cast->basic_info->op_id, i);
            return ICRAFT_CAST_INVALID_OUTPUT_FTMPS;
        }
    }

    if (ifms_num != 1) return ICRAFT_CAST_INVALID_INPUT_FTMPS;
    if (ofms_num != 1) return ICRAFT_CAST_INVALID_OUTPUT_FTMPS;

    float memcpy_time = 0;
    for(uint32_t i = 0; i < ifms_num; ++i){
        icraft_print("start sync cast(opid=%u) ifm(vid=%u), sync idx: %u\r\n",
            cast->basic_info->op_id, ifms[i]->vid, ifms[i]->sync_idx);
        ftmp_ret = icraft_ftmp_sync(ifms[i]->sync_idx);
        if(ftmp_ret){
            uint32_t layer_count;
            icraft_device_get_layercount(&layer_count);
            icraft_print("[CAST(opid=%u)] sync ifm(vid=%u) failed, icore layer count: %u, sync idx: %u, ftmp err code: %u\r\n", 
            cast->basic_info->op_id, ifms[i]->vid, layer_count, ifms[i]->sync_idx, ftmp_ret);
            return ICRAFT_CAST_SYNC_FAIL;
        }
        icraft_print("[CAST] sync CAST(opid=%u) ifm(vid=%u) success!\r\n", cast->basic_info->op_id, ifms[i]->vid);
    }

    cast->basic_info->network_p->current_op = cast->basic_info->op_id;

    icraft_print("ifm_vid: %u, ifm dtype:%u, ifm norm:%f, ifm layout:%s\r\n",ifms[0]->vid, ifms[0]->dtype, ifms[0]->normratio_data[0], ifms[0]->layout);
    icraft_print("ofm_vid: %u, ofm dtype:%u, ofm norm:%f, ofm layout:%s\r\n",ofms[0]->vid, ofms[0]->dtype, ofms[0]->normratio_data[0], ofms[0]->layout);

    icraft_dtype_t itype = ifms[0]->dtype;
    icraft_dtype_t otype = ofms[0]->dtype;

    float inorm = ifms[0]->normratio_data[0];
    float onorm = ofms[0]->normratio_data[0];

    uint64_t reg_base = 0x100001000;
    uint32_t dtc_mode;
    uint32_t dtc_shift = 0;
    uint32_t shift = log2(round(inorm / onorm));

    layout_info* input_layout_info = (layout_info*)aligned_alloc(CACHE_ALIGN_SIZE, align64_sizeof(layout_info));
    layout_info* output_layout_info = (layout_info*)aligned_alloc(CACHE_ALIGN_SIZE, align64_sizeof(layout_info));

    getLayoutOInfo(ifms[0], input_layout_info);
    getLayoutOInfo(ofms[0], output_layout_info);

    uint32_t iftmp_ct = input_layout_info->ct;
    uint32_t oftmp_ct = output_layout_info->ct;
    uint32_t iftmp_cu = input_layout_info->cu;
    uint32_t oftmp_cu = output_layout_info->cu;
    uint32_t diff_cu = (iftmp_cu != oftmp_cu);



    if (itype == ICRAFT_DTYPE_UINT8 || itype == ICRAFT_DTYPE_SINT8) {
        dtc_mode = 0;
        dtc_shift += shift;
    }
    else if (itype == ICRAFT_DTYPE_UINT16 || itype == ICRAFT_DTYPE_SINT16) {
        dtc_mode = 1;
        shift = log2(round(onorm / inorm));
        dtc_shift = (dtc_shift + shift) << 4;
    }
    else {
        icraft_print("invalid input ftmp dtype\r\n");
        return ICRAFT_CAST_INVALID_INPUT_FTMPS;
    }

    uint32_t dtc_version;
    icraft_device_read_reg_relative(reg_base + 0x004, &dtc_version);

    if (dtc_version != 0x20240310) {
        icraft_print("DTC Version Error!!! Version: %d\r\n", dtc_version);
        return ICRAFT_CAST_DTC_WRONG_VERSION;
    }

    icraft_device_write_reg_relative(reg_base + 0x03C, 1);
    icraft_device_write_reg_relative(reg_base + 0x03C, 0);
    icraft_device_write_reg_relative(reg_base + 0x000, (diff_cu << 1) + dtc_mode);
    icraft_device_write_reg_relative(reg_base + 0x010, dtc_shift);

    uint32_t iftmp_base = ifms[0]->addr;
    uint32_t iftmp_size = ifms[0]->size;
    uint32_t iftmp_to = (uint32_t)((ceilf((double)(iftmp_base + iftmp_size) / 64.f) - 1) * 64);

    if (dtc_mode == 1 && diff_cu) {
        iftmp_to = (uint32_t)(iftmp_base + iftmp_size - 32);
    }

    uint32_t oftmp_base = ofms[0]->addr;
    uint32_t oftmp_size = ofms[0]->size;
    uint32_t oftmp_to = (uint32_t)((ceilf((double)(oftmp_base + oftmp_size) / 64.f) - 1) * 64);

    if (dtc_mode == 0 && diff_cu) {
        oftmp_to = (uint32_t)(oftmp_base + oftmp_size - 32);
    }

    icraft_device_write_reg_relative(reg_base + 0x008, iftmp_base);
    icraft_device_write_reg_relative(reg_base + 0x020, iftmp_to);
    icraft_device_write_reg_relative(reg_base + 0x00C, oftmp_base);
    icraft_device_write_reg_relative(reg_base + 0x024, oftmp_to);

    uint64_t last_en;
    uint32_t last_bytesize = oftmp_size % 64;
    if (last_bytesize == 0) {
        last_en = UINT64_MAX;
    }
    else {
        last_en = (uint64_t)pow(2, last_bytesize) - 1;
    }
    uint32_t last_en_l = (uint32_t)last_en;
    uint32_t last_en_h = (uint32_t)(last_en >> 32);

    icraft_device_write_reg_relative(reg_base + 0x02C, last_en_l);
    icraft_device_write_reg_relative(reg_base + 0x030, last_en_h);

    uint32_t len_between_cuct = input_layout_info->element_num_between_cuct - 1;
    icraft_device_write_reg_relative(reg_base + 0x028, len_between_cuct);

    uint32_t ct_len = 0;
    if (dtc_mode == 0) {
        ct_len = ((oftmp_ct % 2) << 31) + ((oftmp_ct + 1) / 2 - 1);
    }
    else {
        ct_len = ((iftmp_ct % 2) << 31) + ((iftmp_ct + 1) / 2 - 1);
    }
    icraft_device_write_reg_relative(reg_base + 0x034, ct_len);
    icraft_device_write_reg_relative(reg_base + 0x014, 1);

    icraft_print("[CAST] cast itype:%u\r\n", itype);
    icraft_print("[CAST] cast dtype:%u\r\n", otype);
    icraft_print("[CAST] cast inorm:%f\r\n", inorm);
    icraft_print("[CAST] cast onorm:%f\r\n", onorm);
    icraft_print("[CAST] cast dtc_mode:%u\r\n", dtc_mode);
    icraft_print("[CAST] cast dtc_shift:%u\r\n", dtc_shift);
    icraft_print("[CAST] cast diff_cu:%u\r\n", diff_cu);
    icraft_print("[CAST] cast iftmp_base:%u\r\n", iftmp_base);
    icraft_print("[CAST] cast iftmp_size:%u\r\n", iftmp_size);
    icraft_print("[CAST] cast iftmp_to:%u\r\n", iftmp_to);
    icraft_print("[CAST] cast oftmp_base:%u\r\n", oftmp_base);
    icraft_print("[CAST] cast oftmp_size:%u\r\n", oftmp_size);
    icraft_print("[CAST] cast oftmp_to:%u\r\n", oftmp_to);

    /*
    uint32_t done_sig = 0;
    while(done_sig == 0){
        if((get_current_time() - begin) > counts){
            icraft_print("cast timeout...\r\n");
            return ICRAFT_CAST_TIME_OUT;
        }
        icraft_device_read_reg_relative(reg_base +  0x018, &done_sig);
    }
    icraft_device_write_reg_relative(reg_base + 0x018, 1);
*/
    free(input_layout_info);
    free(output_layout_info);


    icraft_print("cast forward success!\r\n");
    return ICRAFT_SUCCESS;
}


icraft_return freeCast(struct icraft_cast_t* self) {
    
    icraft_return ret = self->basic_info->free(self->basic_info);
    free(self->basic_info);
    if (ret) {
        icraft_print("[CAST FREE ERROR] error code %u\r\n", ret);
        return ICRAFT_CAST_DECONSTRUCT_FAIL;
    }
    return ICRAFT_SUCCESS;
     
}

icraft_return
icraft_cast_init(struct icraft_cast_t* cast) {
    icraft_return ret;
    uint32_t cast_version;
    icraft_device_read_reg_relative(CAST_REG_BASE + 0x004, &cast_version);
    if(cast_version != ICRAFT_DTC_VERSION){
        icraft_print("[CAST_INIT] detect no valid cast hardware, got one with version: 0x%X\r\n", cast_version);
        return ICRAFT_CAST_INVALID_VERSION;
    }
    return ICRAFT_SUCCESS;
}