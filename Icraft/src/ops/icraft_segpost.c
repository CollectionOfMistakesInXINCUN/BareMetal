#include "ops/icraft_segpost.h"
#include "utils/icraft_print.h"
#include <math.h>
#include "icraft_network.h"

long long double2longBY(double u) {
	long long r = u * pow(2,16);
	return r;
}

icraft_return 
icraft_segpost_forward(struct icraft_segpost_t* segpost)
{
      segpost->basic_info->network_p->current_op = segpost->basic_info->op_id;
  // wait input sync
      icraft_ftmp_info_t *ifm = segpost->basic_info->ifms_ptr[0];

//    icraft_print("[SEGPOST_FORWARD]==================\n");

	uint64_t reg_base = 0x100002400;
	
	int mode = segpost->mode;
	int src_ysize = segpost->src_ysize;
	int src_xsize = segpost->src_xsize;
	int dst_ysize = segpost->dst_ysize;
	int dst_xsize = segpost->dst_xsize;
	
	//多线程由外部配置ps_base，当外部没有配置的时候，会使用默认的预留空间的地址作为SegPost写PS DDR的地址

        uint64_t ps_awbase = (uint32_t)segpost->ps_addr;
	
	int target_ysize = (mode == 1) ? dst_ysize : src_ysize;
	int target_xsize = (mode == 1) ? dst_xsize : src_xsize;
	uint64_t last_awaddr_num = ceil(((float)target_ysize * (float)target_xsize)/2) * 8 - 8;
	int last_awaddr = ps_awbase + last_awaddr_num; 
	int awbase = ps_awbase;

	uint64_t arbase 	 = ifm->addr;

	icraft_device_write_reg_relative(reg_base + 0x048, last_awaddr);
	icraft_device_write_reg_relative(reg_base + 0x044, awbase);
	icraft_device_write_reg_relative(reg_base + 0x040, arbase);

	icraft_print("[SEGPOST_FORWARD] last_awaddr = 0x%x\r\n", last_awaddr);
	icraft_print("[SEGPOST_FORWARD] awbase = 0x%x\r\n", awbase);
	icraft_print("[SEGPOST_FORWARD] arbase = 0x%x\r\n", arbase);


        uint32_t SegPostPsBase;
        icraft_device_read_reg_relative(reg_base + 0x044, &SegPostPsBase);
        uint32_t mapped_base_ = SegPostPsBase - ps_awbase;

	
	// 软复位
	icraft_device_write_reg_relative(reg_base + 0x014, 1);


	icraft_device_write_reg_relative(reg_base + 0x014, 0);

	icraft_ftmp_info_t **ofms = segpost->basic_info->ofms_ptr;
	
        ofms[0]->data =  (uint8_t*)(ps_awbase + mapped_base_);


        ofms[0]->size = target_xsize * target_ysize * 4;
        
        Fmsh_DCacheFlushRange((uint32_t)ofms[0]->data, ofms[0]->size);
	
//    icraft_cache_invalidate(segpost->ps_addr, target_xsize * target_ysize);
	
	// 启动
	icraft_device_write_reg_relative(reg_base + 0x000, 1);
	icraft_device_write_reg_relative(reg_base + 0x000, 0);
	


    
	
	
    return ICRAFT_SUCCESS;

	
}

icraft_return freeSegpost(struct icraft_segpost_t* self) {
    
    icraft_return ret = self->basic_info->free(self->basic_info);
    free(self->ps_addr);
    free(self->basic_info);
    if (ret) {
        icraft_print("[INPUT FREE ERROR] error code %u\r\n", ret);
        return ICRAFT_INPUT_DECONSTRUCT_FAIL;
    }
    return ICRAFT_SUCCESS;
}

icraft_return 
icraft_segpost_init(struct icraft_segpost_t* segpost) {
	
#if ICRAFT_LOG_TIME
    global_timer_enable();
    uint64_t interface_time_begin = get_current_time();
#endif
    icraft_return dev_ret;
    icraft_return input_ret;
    
    icraft_ftmp_info_t *ifm = segpost->basic_info->ifms_ptr[0];
    icraft_ftmp_info_t *ofm = segpost->basic_info->ofms_ptr[0];
	
    int mode        =  segpost->mode;
    int src_ysize	 = segpost->src_ysize;
    int src_xsize	 = segpost->src_xsize;
    int dst_ysize	 = segpost->dst_ysize;
    int dst_xsize	 = segpost->dst_xsize;
    BOOL align_corner = segpost->align_corner;
	
//	auto ifm_shape   = segpost->inputs[0].tensorType()->shape;
//	auto ct_stride   = ifm_shape[-2] * ifm_shape[-3];
	
	int ct_stride = ifm->shape[2] * ifm->shape[3];
    
//  auto arbase 	 = backend->forward_info->value_map.at(op->inputs[0]->v_id)->phy_addr;
//	auto ct_size	 = segpost->inputs[0].tensorType().getDimByAxis('C');
    
	uint64_t arbase 	 = ifm->addr;
	int ct_size	 =ifm->shape[1];
	
	float src_ystep   = align_corner ? ((float)(src_ysize - 1) / (float)(dst_ysize - 1)) : ((float)(src_ysize) / (float)(dst_ysize));
	float src_xstep   = align_corner ? ((float)(src_xsize - 1) / (float)(dst_xsize - 1)) : ((float)(src_xsize) / (float)(dst_xsize));
	float src_ybase   = align_corner ? 0.f : 0.5f * (src_ystep - 1.f);
	float src_xbase   = align_corner ? 0.f : 0.5f * (src_xstep - 1.f);
	
	int sleep_time_ = segpost->sleep_time;
	int reg_base_   = segpost->reg_base;
	
//	int dstw_tile   = int(floor(ceil(double(src_xsize) / 64.0) * (64.f/ src_xstep)));
        int dstw_tile = (int)floor(ceil((float)src_xsize / 64.0) * (64.f / src_xstep));
	
	uint64_t reg_base = 0x100002400;
	
	segpost->reg_base = reg_base;

//	if (reg_base_ != 0)
//		reg_base = reg_base_;
	//检查硬件的版本信息是否正确，不正确会抛出错误

	uint32_t version = 0;
	icraft_device_read_reg_relative(reg_base + 0x008, &version);
	
	icraft_print("SegPost HardWare version=0x%x\r\n", version);

	if (0x20250617 != version) {
		fmsh_print("ERROR :: No SegPost HardWare\r\n");
		return ICRAFT_SEGPOST_INVALID_VERSION;
	}
	
	// soft reset
	icraft_device_write_reg_relative(reg_base + 0x014, 1);
//	usleep(1000);
	icraft_device_write_reg_relative(reg_base + 0x014, 0);

	long long reg_src_ystep = double2longBY(src_ystep);
	long long reg_src_xstep = double2longBY(src_xstep);
	long long reg_src_ybase = double2longBY(src_ybase);
	long long reg_src_xbase = double2longBY(src_xbase);
	
    icraft_ftmp_info_t **ifms = segpost->basic_info->ifms_ptr;
	int reg_data_type = 0;
	
	icraft_dtype_t dtype = ifms[0]->dtype;
	
	if( dtype == ICRAFT_DTYPE_SINT16 ) {
		reg_data_type = 1;
	}
	
//#if ICRAFT_DEBUG
	icraft_print("[SEGPOST_INIT] src_ystep=%f\r\n", src_ystep);
	icraft_print("[SEGPOST_INIT] src_xstep=%f\r\n", src_xstep);
	icraft_print("[SEGPOST_INIT] align corner=%d\r\n", align_corner);
	icraft_print("[SEGPOST_INIT] dstw_tile=%d\r\n", dstw_tile);
	icraft_print("[SEGPOST_INIT] mode=%d\r\n", mode);
	icraft_print("[SEGPOST_INIT] reg_data_type=%d\r\n", reg_data_type);
	icraft_print("[SEGPOST_INIT] ct_stride=%d\r\n", ct_stride);
	icraft_print("[SEGPOST_INIT] arbase=%d\r\n", arbase);
	icraft_print("[SEGPOST_INIT] ct_size=%d\r\n", ct_size);
	icraft_print("[SEGPOST_INIT] reg_src_ystep=%d\r\n", reg_src_ystep);
	icraft_print("[SEGPOST_INIT] reg_src_xstep=%d\r\n", reg_src_xstep);
	icraft_print("[SEGPOST_INIT] src_ysize=%d\r\n", src_ysize);
	icraft_print("[SEGPOST_INIT] src_xsize=%d\r\n", src_xsize);
	icraft_print("[SEGPOST_INIT] dst_ysize=%d\r\n", dst_ysize);
	icraft_print("[SEGPOST_INIT] dst_xsize=%d\r\n", dst_xsize);
	icraft_print("[SEGPOST_INIT] reg_src_ybase=%d\r\n", reg_src_ybase);
	icraft_print("[SEGPOST_INIT] reg_src_xbase=%d\r\n", reg_src_xbase);
//#endif
	

        segpost->ps_addr = (uint32_t*)aligned_alloc(CACHE_ALIGN_SIZE,10000000);
        //memset(segpost->ps_addr, 0, 5000000);
        //memset(segpost->ps_addr, 0, oysize * oxsize);
//    icraft_cache_invalidate((dma_mmaped_ptr + 0x2000000), oysize * oxsize);

//	segpost->ps_addr = (uint32_t*)memalign(CACHE_ALIGN_SIZE, oysize * oxsize);
//    icraft_cache_invalidate(segpost->ps_addr, oysize * oxsize);

	icraft_device_write_reg_relative(reg_base + 0x05C, dstw_tile);
	icraft_device_write_reg_relative(reg_base + 0x058, mode);
	icraft_device_write_reg_relative(reg_base + 0x054, reg_data_type);
	icraft_device_write_reg_relative(reg_base + 0x050, ct_stride);
	icraft_device_write_reg_relative(reg_base + 0x040, arbase);
	icraft_device_write_reg_relative(reg_base + 0x03C, ct_size);
	icraft_device_write_reg_relative(reg_base + 0x038, reg_src_ystep);
	icraft_device_write_reg_relative(reg_base + 0x034, reg_src_xstep);
	icraft_device_write_reg_relative(reg_base + 0x030, src_ysize);
	icraft_device_write_reg_relative(reg_base + 0x02C, src_xsize);
	icraft_device_write_reg_relative(reg_base + 0x028, dst_ysize);
	icraft_device_write_reg_relative(reg_base + 0x024, dst_xsize);
	icraft_device_write_reg_relative(reg_base + 0x020, reg_src_ybase);
	icraft_device_write_reg_relative(reg_base + 0x01C, reg_src_xbase);
	icraft_device_write_reg_relative(reg_base + 0x010, 0);
   	
	
    return ICRAFT_SUCCESS;
}
