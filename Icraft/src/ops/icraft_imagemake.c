#include "ops/icraft_imagemake.h"
#include "icraft_network.h"
#include <math.h>

icraft_return
icraft_imagemake_init(icraft_imagemake_t * imagemake)
{
    icraft_return imk_ret;

    
    
    uint64_t reg_base = 0x100000400;
    uint32_t custom_reg_base = imagemake->imk_reg_base;
    uint32_t one_fast = (imagemake->imk_one_fast == TRUE) ? 1 : 0;
    if(custom_reg_base != 0){
        reg_base = custom_reg_base;
    }
    else if(imagemake->imk_input_port == 0){
        reg_base = 0x100000400;
    }
    else if(imagemake->imk_input_port == 1){
        reg_base = 0x100040000;
    }
    else if(imagemake->imk_input_port == 2){
        reg_base = 0x100040400;
    }
    else if(imagemake->imk_input_port == 3){
        reg_base = 0x100040800;
    }
    else{
        icraft_print("[IMAGEMAKE_INIT] invalid imagemake input port(got %u), which ranges in {0,1,2,3}\r\n");
        return ICRAFT_IMAGEMAKE_INVALID_INPUT_PORT;
    }


    uint32_t imagemake_version;
    icraft_device_read_reg_relative(reg_base + 0x234, &imagemake_version);
    if(imagemake_version != ICRAFT_IMAGEMAKE_VERSION){
        icraft_print("[IMAGEMAKE_INIT] detect no valid imagemake hardware, got one with version: 0x%X\r\n", imagemake_version);
        return ICRAFT_IMAGEMAKE_INVALID_VERSION;
    }
    icraft_print("[IMAGEMAKE_INIT] imagemake version: 0x%llX\r\n", imagemake_version);
    
    icraft_ftmp_info_t *ifm = imagemake->basic_info->ifms_ptr[0];
    icraft_ftmp_info_t *ofm = imagemake->basic_info->ofms_ptr[0];
    if(ifm==NULL){
        icraft_print("[IMAGEMAKE_INIT] ifm is null\r\n");
        return ICRAFT_IMAGEMAKE_INVALID_INPUT_FTMPS;
    }
    if(ofm==NULL){
        icraft_print("[IMAGEMAKE_INIT] ofm is null\r\n");
        return ICRAFT_IMAGEMAKE_INVALID_OUTPUT_FTMPS;
    }


    uint32_t pad_r = imagemake->imk_pad_size[0];
    uint32_t pad_l = imagemake->imk_pad_size[1];
    uint32_t pad_b = imagemake->imk_pad_size[2];
    uint32_t pad_t = imagemake->imk_pad_size[3];

    uint32_t imagemake_channel = ifm->shape[3];
    uint32_t imagemake_width = ifm->shape[2];
    uint32_t imagemake_height = ifm->shape[1];
    imagemake->channel = imagemake_channel;
    imagemake->width = imagemake_width;
    imagemake->height = imagemake_height;


    uint32_t imagemake_channel_each = 0;
    if(imagemake_channel % 4 == 0){
        imagemake_channel_each = 4;
    }
    else if(imagemake_channel % 3 == 0){
        imagemake_channel_each = 3;
    }
    else if(imagemake_channel % 2 == 0){
        imagemake_channel_each = 2;
    }
    else{
        imagemake_channel_each = 1;
    }

    uint32_t imagemake_channel_times = imagemake_channel / imagemake_channel_each;
    if(imagemake_channel_times != 1){
        icraft_print("[IMAGEMAKE_INIT] invalid channel times, got %u, only support channel_times=1 now\r\n", 
                    imagemake_channel_times);
        return ICRAFT_IMAGEMAKE_INVALID_CHANNEL_TIMES;
    }
    imagemake->channel_each = imagemake_channel_each;
    imagemake->channel_times = imagemake_channel_times;
    
    int32_t *premean = (int32_t*)aligned_alloc(CACHE_ALIGN_SIZE, align64(sizeof(int32_t) * imagemake_channel));
    int32_t *prescale = (int32_t*)aligned_alloc(CACHE_ALIGN_SIZE, align64(sizeof(int32_t) * imagemake_channel));
    for(uint32_t i = 0; i < imagemake_channel; ++i){
        premean[i] = (int32_t)(-imagemake->imk_pre_mean[i]);
        prescale[i] = (int32_t)((1.f / imagemake->imk_pre_scale[i]) * pow(2, 10));
        icraft_print("[IMAGEMAKE_INIT] premean[%u] set to hardward: %d\r\n", i, premean[i]);
        icraft_print("[IMAGEMAKE_INIT] prescale[%u] set to hardward: %d\r\n", i, prescale[i]);
    }


    uint32_t *imagemake_msc = (uint32_t*)aligned_alloc(CACHE_ALIGN_SIZE, align64(sizeof(uint32_t) * imagemake_channel));
    if(imagemake_msc == NULL){
        icraft_print("[IMAGEMAKE_INIT] alloc memory for msc failed, size=%u\r\n", sizeof(uint32_t) * imagemake_channel);
        return ICRAFT_IMAGEMAKE_MALLOC_FAIL;
    }
    for(uint32_t i = 0; i< imagemake_channel; ++i){
        imagemake_msc[i] = (int16_t)prescale[i] | (int16_t)premean[i] << 16;
    }
    imagemake->hw_msc = imagemake_msc;



    
    uint32_t imagemake_wddr_base = ofm->addr;
    uint32_t imagemake_pad_reg = pad_t + (pad_b << 8) + (pad_l << 16) + (pad_r << 24);
    imagemake->wddr_base = imagemake_wddr_base;
    imagemake->pad_reg = imagemake_pad_reg;


    uint32_t imagemake_data_arrange;
    if(ofm->dtype == ICRAFT_DTYPE_SINT8 || ofm->dtype == ICRAFT_DTYPE_UINT8){
        imagemake_data_arrange = 0;
    }
    else if(ofm->dtype == ICRAFT_DTYPE_SINT16 || ofm->dtype == ICRAFT_DTYPE_UINT16){
        imagemake_data_arrange = 1;
    }
    else{
        icraft_print("[IMAGEMAKE_INIT] unsupported output ftmp dtype of imagemake, got %u, see icraft_type.h\r\n", ofm->dtype);
    }
    imagemake->data_arrange = imagemake_data_arrange;
    
    icraft_device_write_reg_relative(reg_base + 0x004, one_fast);

    // 调用device的写寄存器的接口，配置硬件需要的寄存器，其中writeReg和readReg中最后一个参数为true，用于整体寄存器空间偏移配置
    // 各寄存器表示含义介绍，参照ImakeMake硬算子用户手册
    // 0x014~0x110
    for (uint32_t i = 0; i < 4; i++) {
        if (i < imagemake_channel) {
            icraft_device_write_reg_relative(reg_base + (0x14 + i * 4), imagemake_msc[i]);
        }
        else {
            icraft_device_write_reg_relative(reg_base + (0x14 + i * 4), (uint32_t)(0));
        }
    }


    icraft_device_write_reg_relative(reg_base + 0x114, imagemake_wddr_base);
    icraft_device_write_reg_relative(reg_base + 0x118, imagemake_width);
    icraft_device_write_reg_relative(reg_base + 0x11C, imagemake_height);
    icraft_device_write_reg_relative(reg_base + 0x124, imagemake_pad_reg);

    // //0x128~0x224
    for(uint32_t i = 0; i < imagemake_channel; ++i){
        icraft_device_write_reg_relative(reg_base + (0x128 + i * 4), 0);
    }

    icraft_device_write_reg_relative(reg_base + 0x240, imagemake_channel_each);
    icraft_device_write_reg_relative(reg_base + 0x244, imagemake_channel_times);
    icraft_device_write_reg_relative(reg_base + 0x254, imagemake_data_arrange);

    if(PLIN){
          uint32_t RATIO_W = CAM_W / imagemake->width;
          uint32_t RATIO_H = CAM_H / imagemake->height;
          uint32_t IMG_W = RATIO_W * imagemake->width;
          uint32_t IMG_H = RATIO_H * imagemake->height;
          uint32_t BIAS_W = (CAM_W - IMG_W) / 2;
          uint32_t BIAS_H = (CAM_H - IMG_H) / 2;

          icraft_print("[IMK PLIN] ratio_w:%u, ratio_h:%u, img_w:%u, img_h:%u\r\n", RATIO_W, RATIO_H, IMG_W, IMG_H);
          
          icraft_device_write_reg(0x40080018, 1);
          icraft_device_write_reg(0x40080020, RATIO_W);			
          icraft_device_write_reg(0x40080024, RATIO_H);			
          icraft_device_write_reg(0x40080028, BIAS_W);			
          icraft_device_write_reg(0x4008002C, BIAS_H);			
          icraft_device_write_reg(0x40080030, IMG_W + BIAS_W - 1);	    
          icraft_device_write_reg(0x40080034, IMG_H + BIAS_H - 1);		
          icraft_device_write_reg(0x40080038, CAM_W);	   
          icraft_device_write_reg(0x4008003C, CAM_H);	        
    }

    free(premean);
    free(prescale);


    icraft_print("[IMAGEMAKE_INIT] imagemake_wddr_base: 0x%llx\r\n", imagemake_wddr_base);
    icraft_print("[IMAGEMAKE_INIT] imagemake_width: %u\r\n", imagemake_width);
    icraft_print("[IMAGEMAKE_INIT] imagemake_height: %u\r\n", imagemake_height);
    icraft_print("[IMAGEMAKE_INIT] imagemake_pad_reg: 0x%llx\r\n", imagemake_pad_reg);
    icraft_print("[IMAGEMAKE_INIT] imagemake_channel_each: %u\r\n", imagemake_channel_each);
    icraft_print("[IMAGEMAKE_INIT] imagemake_channel_times: %u\r\n", imagemake_channel_times);
    icraft_print("[IMAGEMAKE_INIT] imagemake_data_arrange: %u\r\n", imagemake_data_arrange);

    return ICRAFT_SUCCESS;
}

/*
icraft_return
icraft_imagemake_forward(struct icraft_imagemake_t* imagemake)
{
#if ICRAFT_LOG_TIME
    uint64_t interface_time_begin = get_current_time();
#endif
    icraft_ftmp_info_t **ifms = imagemake->basic_info->ifms_ptr;
    icraft_ftmp_info_t **ofms = imagemake->basic_info->ofms_ptr;

    uint64_t reg_base = 0x100000400;
    uint32_t custom_reg_base = imagemake->imk_reg_base;
    if(custom_reg_base != 0){
        reg_base = custom_reg_base;
    }
    else if(imagemake->imk_input_port == 0){
        reg_base = 0x100000400;
    }
    else if(imagemake->imk_input_port == 1){
        reg_base = 0x100040000;
    }
    else if(imagemake->imk_input_port == 2){
        reg_base = 0x100040400;
    }
    else if(imagemake->imk_input_port == 3){
        reg_base = 0x100040800;
    }
    else{
        fmsh_print("[IMAGEMAKE_INIT] invalid imagemake input port(got %u), which ranges in {0,1,2,3}\r\n");
        return ICRAFT_IMAGEMAKE_INVALID_INPUT_PORT;
    }

    icraft_device_write_reg_relative(reg_base + 0x114, ofms[0]->addr);

    return ICRAFT_SUCCESS;
}
*/


icraft_return
icraft_imagemake_forward(struct icraft_imagemake_t* imagemake)
{
    if (PLIN) {
        imagemake->basic_info->network_p->current_op = imagemake->basic_info->op_id;
        icraft_device_write_reg(0x40080004, 1);
    }
    else {
    icraft_return dev_ret;

    imagemake->basic_info->network_p->current_op = imagemake->basic_info->op_id;

    icraft_ftmp_info_t **ifms = imagemake->basic_info->ifms_ptr;

    uint32_t imagemake_rlen = 0;
    uint32_t imagemake_last_sft = 0;
    
    icraft_dtype_t ifm_dtype = ifms[0]->dtype;
    
    uint64_t reg_base = imagemake->reg_base;
    uint64_t reg_base_preset = 0x1000C0000;
    
    
    if (ifm_dtype == ICRAFT_DTYPE_SINT16 || ifm_dtype == ICRAFT_DTYPE_UINT16) {
        icraft_device_write_reg_relative(reg_base_preset + 0x20, 0x2);
        uint32_t totalBytes = imagemake->width * imagemake->height * imagemake->channel * 2;
        uint32_t minBlocks = (totalBytes + 7) / 8;
        imagemake_rlen  =  ((minBlocks + 2) / 3) * 3;
        
        uint32_t pixelsPer24Bytes = 24 / (2 * imagemake->channel);
	uint32_t totalPixels = imagemake->width * imagemake->height;
	imagemake_last_sft = totalPixels - (imagemake_rlen / 3 - 1) * pixelsPer24Bytes;
    }
    else {
        imagemake_rlen = ((imagemake->width * imagemake->height - 1) / (24 / imagemake->channel) + 1) * 3;
        imagemake_last_sft = imagemake->width * imagemake->height - (imagemake_rlen - 3) / 3 * (24 / imagemake->channel);
        icraft_device_write_reg_relative(reg_base_preset + 0x20, 0);
    }
    
    uint32_t imagemake_rddr_base = (uint32_t)(ifms[0]->data);   // ps ddr数据地址

    


    
    //fmsh_print("(intptr_t)ifms[0]->size:%u, imagemake_rddr_base:%llX\r\n", (intptr_t)ifms[0]->size, imagemake_rddr_base);

    Fmsh_DCacheFlushRange(imagemake_rddr_base, (intptr_t)ifms[0]->size);
 
    icraft_device_write_reg_relative(reg_base_preset + 0x4, imagemake_rddr_base);
    icraft_device_write_reg_relative(reg_base_preset + 0x8, imagemake_rlen);
    icraft_device_write_reg_relative(reg_base_preset + 0xC, imagemake_last_sft);
    icraft_device_write_reg_relative(reg_base_preset + 0x10, imagemake->channel);
    icraft_device_write_reg_relative(reg_base_preset + 0x1C, 1);
    //icraft_device_write_reg_relative(reg_base_preset + 0x20, 0);
    
#if ICRAFT_DEBUG
    icraft_print("[IMAGEMAKE] presetting IMAGEMAKE(reg base=0x%llX, preset reg base=0x%llX)...\r\n", reg_base, reg_base_preset);
    icraft_print("[IMAGEMAKE] set channel: %u, width: %u, height: %u\r\n", 
                imagemake->channel, imagemake->width, imagemake->height);
    icraft_print("[IMAGEMAKE] set rlen: %u, last sft: %u\r\n", imagemake_rlen, imagemake_last_sft);
    icraft_print("[IMAGEMAKE] set pad_reg: %u\r\n", imagemake->pad_reg);
    icraft_print("[IMAGEMAKE] set channel_each: %u\r\n", imagemake->channel_each);
    icraft_print("[IMAGEMAKE] set channel_times: %u\r\n", imagemake->channel_times);
    icraft_print("[IMAGEMAKE] set data_arrange: %u\r\n", imagemake->data_arrange);
    icraft_print("[IMAGEMAKE] set rddr base: 0x%llX\r\n", imagemake_rddr_base);
    icraft_print("[IMAGEMAKE] set wddr base: 0x%llX\r\n", imagemake->wddr_base);
#endif

    icraft_device_write_reg_relative(reg_base_preset, 1);
    }
    return ICRAFT_SUCCESS;
}

icraft_return freeImagemake(struct icraft_imagemake_t* self) {
    free(self->hw_msc);
    
    icraft_return ret = self->basic_info->free(self->basic_info);
    free(self->basic_info);
    if (ret) {
        fmsh_print("[HARDOP FREE ERROR] error code %u\r\n", ret);
        return ICRAFT_HARDOP_DECONSTRUCT_FAIL;
    }
    return ICRAFT_SUCCESS;
}