#include "ops/icraft_warpaffine.h"
#include "icraft_network.h"
#include <math.h>
//#include <fcntl.h>


icraft_return
icraft_warpaffine_forward(struct icraft_warpaffine_t* self) {
	
	self->basic_info->network_p->current_op = self->basic_info->op_id;
	uint64_t reg_base = self->warpaffine_reg_base;
	
	// soft reset
	icraft_device_write_reg_relative(reg_base + 0x040, 1);
	icraft_device_write_reg_relative(reg_base + 0x040, 1);
	icraft_device_write_reg_relative(reg_base + 0x040, 0);		
	
	icraft_ftmp_info_t *ifm = self->basic_info->ifms_ptr[0];
	
	uint32_t ifm_channel = ifm->shape[4];
	int remap_grid_bytesize = self->grid_byte_size;
	int ftmp_w = ifm->shape[3];
	int ftmp_h = ifm->shape[2];

	
    icraft_ftmp_info_t **ifms = self->basic_info->ifms_ptr;
    icraft_ftmp_info_t **ofms = self->basic_info->ofms_ptr;
	
	uint32_t ftmp_addr_rfrom = self->basic_info->ifms_ptr[0]->addr;
	uint32_t grid_addr_rfrom = self->warpaffine_plddr_addr;
	uint32_t ftmp_addr_wfrom  = self->basic_info->ofms_ptr[0]->addr;
	
	 
	int data_type;
	icraft_dtype_t dtype = ifms[0]->dtype;
	if(dtype==ICRAFT_DTYPE_SINT8){
		dtype = 0;
	}
	else if(dtype == ICRAFT_DTYPE_SINT16){
		dtype = 1;
	}
	else{
		fmsh_print("invalid dtype: %u, see icraft_dtype_t\r\n", dtype);
		return ICRAFT_WARPAFFINE_INVALID_DTYPE;
	}
	
	int remap_ifm_bytesize = ftmp_w * ftmp_h * ifm_channel * (dtype ? 2 : 1);
	
	uint32_t grid_addr_rto = ceil((float)(grid_addr_rfrom + remap_grid_bytesize) / 64) * 64 - 64;
    uint32_t ftmp_addr_rto = ceil((float)(ftmp_addr_rfrom + remap_ifm_bytesize) / 64) * 64 - 64; 
	
	int flags = (self->flags == 0) ? 1 : 0;              //��Ӳ��������0Ϊ˫���� 1Ϊ���ڽ�
	int border_mode = (self->borderMode == 0) ? 1 : 0;   //��Ӳ��������0Ϊpad�߽�ֵ��1Ϊpad�̶�ֵ
	int border_value = self->borderValue;
	int mode = dtype + (flags << 1) + (border_mode << 2) + (border_value << 3);

	icraft_device_write_reg_relative(reg_base, grid_addr_rfrom);
	icraft_device_write_reg_relative(reg_base + 0x004, grid_addr_rto);
	icraft_device_write_reg_relative(reg_base + 0x008, ftmp_addr_rfrom);
	icraft_device_write_reg_relative(reg_base + 0x00C, ftmp_addr_rto);
	icraft_device_write_reg_relative(reg_base + 0x010, ftmp_w);
	icraft_device_write_reg_relative(reg_base + 0x014, ftmp_h);
	icraft_device_write_reg_relative(reg_base + 0x018, ftmp_addr_wfrom);
	icraft_device_write_reg_relative(reg_base + 0x04C, mode);
	//��������
	icraft_device_write_reg_relative(reg_base + 0x01C, 1); //�����Ĵ���
	
#if ICRAFT_DEBUG
	icraft_print("[WARPAFFINE_FORWARD] mode: %u\r\n", mode);
	icraft_print("[WARPAFFINE_FORWARD] grid_addr_rfrom: %u\r\n", grid_addr_rfrom);
	icraft_print("[WARPAFFINE_FORWARD] grid_addr_rto: %u\r\n", grid_addr_rto);
	icraft_print("[WARPAFFINE_FORWARD] ftmp_addr_rfrom: %u\r\n", ftmp_addr_rfrom);
	icraft_print("[WARPAFFINE_FORWARD] ftmp_addr_rto: %u\r\n", ftmp_addr_rto);
	icraft_print("[WARPAFFINE_FORWARD] ftmp_w: %u\r\n", ftmp_w);
	icraft_print("[WARPAFFINE_FORWARD] ftmp_h: %u\r\n", ftmp_h);
	icraft_print("[WARPAFFINE_FORWARD] remap_ifm_bytesize: %u\r\n", remap_ifm_bytesize);
	icraft_print("[WARPAFFINE_FORWARD] remap_grid_bytesize: %u\r\n", remap_grid_bytesize);
	icraft_print("[WARPAFFINE_FORWARD] ftmp_addr_wfrom: %u\r\n", ftmp_addr_wfrom);
#endif
	//wait done
/*
	uint32_t done_sig = 0;
	INT64 start, end, duration;  //ns
	
#if ACORE_OS653MP
    int get_time_ret;
    GET_TIME(&start, &get_time_ret);
#else
	ACoreOs_clock_get_systime(&start);
	
#endif
	
	while (done_sig != 1) {
#if ACORE_OS653MP
	GET_TIME(&end, &get_time_ret);
#else
	ACoreOs_clock_get_systime(&end);
#endif	
	
	duration = (end - start) / 1000 / 1000;
	if(duration > TIMEOUT_THRES ) {
		   fmsh_print("[WARPAFFINE_FORWARD] WARPAFFIN timecount!!\n");
		   return ICRAFT_WARPAFFINE_TIMEOUT;
	   }
	   icraft_device_read_reg_relative(reg_base + 0x020, &done_sig);
   }
	
#if ICRAFT_LOG_TIME	
	if (backend->time_profile) {
		uint32_t time_cnt = icraft_device_read_reg_relative(reg_base + 0x024, &time_cnt);		//Ӳ����ʱ
		double hard_time = (time_cnt * 10) / 1000000.0;
	}
#endif

	icraft_device_write_reg_relative(reg_base + 0x020, 1);  //��done
*/
	
	//���������tensor
    return ICRAFT_SUCCESS;
}


icraft_return freeWarpaffine(struct icraft_warpaffine_t* self) {
   
	// free matrix inverse
	for (int i = 0; i < self->MInversedArray_size; i++) {
		free(self->M_inversed[i]); // ���ͷ�ÿһ�е��ڴ�
	}
	free(self->M_inversed); // ���ͷ�ָ���е�ָ������
	
	icraft_return ret = self->basic_info->free(self->basic_info);
	free(self->basic_info);
	if (ret) {
		fmsh_print("[HARDOP FREE ERROR] error code %u\r\n", ret);
		return ICRAFT_HARDOP_DECONSTRUCT_FAIL;
	}
	
    return ICRAFT_SUCCESS;
     
}


icraft_return
icraft_warpaffine_init(struct icraft_warpaffine_t* self) {
	
	// version check
	uint64_t reg_base = 0x100002800;
	uint32_t warpaffine_version;
	icraft_device_read_reg_relative(WARPAFFINE_REG_BASE + 0x028, &warpaffine_version);
	if(warpaffine_version != ICRAFT_WARPAFFINE_VERSION){
		fmsh_print("[WARPAFFINE_INIT] detect no valid warpaffine hardware, got one with version: 0x%X\r\n", warpaffine_version);
		return ICRAFT_WARPAFFINE_INVALID_VERSION;
	}
	
#if ICRAFT_DEBUG
	icraft_print("[WARPAFFINE_INIT] warpaffine version: 0x%X\r\n", warpaffine_version);
	icraft_print("[WARPAFFINE_INIT] warpaffine flags: 0x%X\r\n", self->flags);
	icraft_print("[WARPAFFINE_INIT] warpaffine borderMode: 0x%X\r\n", self->borderMode);
	icraft_print("[WARPAFFINE_INIT] warpaffine borderValue: 0x%X\r\n", self->borderValue);
#endif
	self->warpaffine_reg_base = reg_base;
	
	icraft_ftmp_info_t *ofm = self->basic_info->ofms_ptr[0];
    uint32_t ofm_channel = ofm->shape[3];
	uint32_t ofm_w = ofm->shape[3];
	uint32_t ofm_h = ofm->shape[2];
	
	int16_t *grid_data = (int16_t*)aligned_alloc(CACHE_ALIGN_SIZE, align64(sizeof(int16_t) * 2 * ofm_h * ofm_w));

	for (int j = 0; j < ofm_h;j++) {
		for (int i = 0;i < ofm_w;i++) {
			for (int c = 0;c < 2;c++) {
				if (c == 0) 
					grid_data[j * ofm_w * 2 + i * 2 + c] = i;
				else
					grid_data[j * ofm_w * 2 + i * 2 + c] = j;
			}
		}
	}
	self->grid_byte_size = ((ofm_w * ofm_h * 2 * 2 ) + 63) / 64 * 64;
	
	// write to pl ddr
	// TODO grid write to where?
    icraft_return dev_ret = icraft_device_write_plmem(self->warpaffine_plddr_addr, (char *)grid_data, self->grid_byte_size);
    if(dev_ret){
		fmsh_print("[WARPAFFINE] upload grid data failed, device err code=%u\r\n", 
			dev_ret);
		return ICRAFT_WARPAFFINE_UPLOAD_GRID_DATA_FAIL;
	}
    
 
	//���þ���
    int64_t coef_a = (self->M_inversed[0][0] * pow(2, 15));
    int64_t coef_b = (self->M_inversed[0][1] * pow(2, 15));
    int64_t coef_c = (self->M_inversed[0][2] * 2);
    int64_t coef_d = (self->M_inversed[1][0] * pow(2, 15));
    int64_t coef_e = (self->M_inversed[1][1] * pow(2, 15));
    int64_t coef_f = (self->M_inversed[1][2] * 2);
    
	icraft_device_write_reg_relative(reg_base + 0x030, coef_a);
	icraft_device_write_reg_relative(reg_base + 0x034, coef_c);
	icraft_device_write_reg_relative(reg_base + 0x038, coef_e);
	icraft_device_write_reg_relative(reg_base + 0x03C, coef_f);
	icraft_device_write_reg_relative(reg_base + 0x044, coef_b);
	icraft_device_write_reg_relative(reg_base + 0x048, coef_d);
	
#if ICRAFT_DEBUG
	icraft_print("[WARPAFFINE_INIT] warpaffine_reg_base: 0x%llX\r\n", reg_base);
	icraft_print("[WARPAFFINE_INIT] pl chunck addr: %u\r\n", self->warpaffine_plddr_addr);
	icraft_print("[WARPAFFINE_INIT] coef_a: %d\r\n",coef_a);
	icraft_print("[WARPAFFINE_INIT] coef_b: %d\r\n",coef_b);
	icraft_print("[WARPAFFINE_INIT] coef_c: %d\r\n",coef_c);
	icraft_print("[WARPAFFINE_INIT] coef_d: %d\r\n",coef_d);
	icraft_print("[WARPAFFINE_INIT] coef_e: %d\r\n",coef_e);
	icraft_print("[WARPAFFINE_INIT] coef_f: %d\r\n",coef_f);
#endif
	
	free(grid_data);

    return ICRAFT_SUCCESS;
}
