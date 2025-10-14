#include "icraft_device.h"



const uint32_t gp0_base = GP0_BOTTOM;
const uint32_t gp1_base = GP1_BOTTOM;
uint8_t g_icraft_device_isopen = ICRAFT_FALSE;
uint8_t g_icraft_device_mmu_isopen = ICRAFT_FALSE;
icraft_device_version_t g_icraft_dev_versions = {0};




icraft_return icraft_device_open(){
    if(g_icraft_device_isopen == ICRAFT_TRUE){
        icraft_print("device has already been open\r\n");
        return ICRAFT_SUCCESS;
    }
    icraft_return rc;
    g_icraft_device_isopen = ICRAFT_TRUE;
    rc = icraft_device_reset(0);
    if(rc){
        icraft_print("reset device failed, ---err code %d\r\n", rc);
        return rc;
    }
    uint32_t layer_cnt;
    rc = icraft_device_get_layercount(&layer_cnt);
    if(rc || layer_cnt != 0){
        icraft_print("reset device layer count failed, layer count=%u\r\n now", layer_cnt);
        return ICRAFT_DEVICE_RESET_ERROR;
    }
    icraft_print("reset device success\r\n");
    icraft_print(">>>>>>>>>> layer count: %u\r\n", layer_cnt);
    rc = icraft_device_get_version(&g_icraft_dev_versions);
    if(rc){
        icraft_print("get device version failed, ---err code %d\r\n", rc);
        return rc;
    }
    icraft_print("get device version success\r\n");
    icraft_print("device version=0x%X, icore version=%s, mmu version=0x%X\r\n", 
        g_icraft_dev_versions.device_version, 
        g_icraft_dev_versions.icore_version, 
        g_icraft_dev_versions.mmu_version);
    return ICRAFT_SUCCESS;
}

icraft_return
icraft_device_get_version(icraft_device_version_t *versions){
    uint32_t ver_info[3];
    icraft_return rc;
    rc = icraft_device_read_reg_relative(BYAI_VERSION1, &ver_info[0]);
    rc = icraft_device_read_reg_relative(BYAI_VERSION2, &ver_info[1]);
    rc = icraft_device_read_reg_relative(BYAI_VERSION3, &ver_info[2]);
    char* vchar = (char*)ver_info;
	versions->icore_version[0] = vchar[3];
	versions->icore_version[1] = vchar[2];
	versions->icore_version[2] = vchar[1];
	versions->icore_version[3] = vchar[0];
	versions->icore_version[4] = vchar[7];
	versions->icore_version[5] = vchar[6];
	versions->icore_version[6] = vchar[5];
	versions->icore_version[7] = vchar[4];
	versions->icore_version[8] = '-';
    sprintf(versions->icore_version + 9, "%08X", ver_info[2]);
    versions->icore_version[17] = '\0';
    rc = icraft_device_read_reg_relative(PRJ_VER, &versions->device_version);
    rc = icraft_device_read_reg_relative(MMU_VERSION, &versions->mmu_version);
    return ICRAFT_SUCCESS;
}

icraft_return
icraft_device_read_reg(const uint64_t addr, uint32_t *value){
    icraft_print("read reg ---0x%llX with abs addr\r\n", addr);
    if(!g_icraft_device_isopen){
        icraft_print("device not open\r\n");
        return ICRAFT_DEVICE_NOT_OPEN;
    }
    uint64_t offset = addr - gp0_base;
    if(offset < MEM_MAP_SIZE){
        *value = *(volatile uint32_t*)(gp0_base + offset);
        return ICRAFT_SUCCESS;
    }
    offset = addr - gp1_base;
    if(offset < MEM_MAP_SIZE){
        *value = *(volatile uint32_t*)(gp1_base + offset);
        return ICRAFT_SUCCESS;
    }
    icraft_print("reg addr ---0x%llX out of bound\r\n", addr);
    return ICRAFT_DEVICE_REG_ADDR_OUT_OF_BOUND;
}

icraft_return
icraft_device_read_reg_relative(const uint64_t addr, uint32_t *value){
    //icraft_print("read reg ---0x%d (relative) begin\r\n", addr);
    if(!g_icraft_device_isopen){
        icraft_print("device not open\r\n");
        return ICRAFT_DEVICE_NOT_OPEN;
    }
    uint64_t rel_addr = addr & 0xFFFFFFFF;
    if(rel_addr >= MEM_MAP_SIZE) {
        return ICRAFT_DEVICE_REG_ADDR_OUT_OF_BOUND;
    }
    int64_t gp_base = (addr & 0x100000000) ? gp1_base : gp0_base;
    *value =  *(volatile uint32_t*)(gp_base + rel_addr);
    //icraft_print("read reg ---0x%X with relative addr finish\r\n", addr);
    return ICRAFT_SUCCESS;
}

icraft_return
icraft_device_write_reg(const uint64_t addr, const uint32_t value){
    if(!g_icraft_device_isopen){
        icraft_print("device not open\r\n");
        return ICRAFT_DEVICE_NOT_OPEN;
    }
    uint64_t offset = addr - gp0_base;
    if(offset < MEM_MAP_SIZE){
        *(volatile uint32_t*)(gp0_base + offset) = value;
        return ICRAFT_SUCCESS;
    }
    offset = addr - gp1_base;
    if(offset < MEM_MAP_SIZE){
        *(volatile uint32_t*)(gp1_base + offset) = value;
        return ICRAFT_SUCCESS;
    }
    return ICRAFT_DEVICE_REG_ADDR_OUT_OF_BOUND;    
}

icraft_return
icraft_device_write_reg_relative(const uint64_t addr, const uint32_t value){
    if(!g_icraft_device_isopen){
        icraft_print("device not open\r\n");
        return ICRAFT_DEVICE_NOT_OPEN;
    }
    uint64_t rel_addr = addr & 0xFFFFFFFF;
    if(rel_addr >= MEM_MAP_SIZE) {
        return ICRAFT_DEVICE_REG_ADDR_OUT_OF_BOUND;
    }
    uint32_t gp_base = (addr & 0x100000000) ? gp1_base : gp0_base;
    *(volatile uint32_t*)(gp_base + rel_addr) = value;
    return ICRAFT_SUCCESS;        
}

icraft_return
_icraft_device_wait_cdma_done(const uint32_t time_ms){
    uint32_t st;
    double counts = time_ms * 1000 * GTC_CLK_FREQ;
    global_timer_enable();
    uint64_t begin = get_current_time();
    while((get_current_time() - begin) < counts){
        icraft_device_read_reg_relative(DMA_STATE_ADDR, &st);
        if((st & DMA_ERROR_MASK) != DMA_ERROR_EXPECT){
            icraft_print("cdma state: 0x%X, after mask: %X\r\n", st, st & DMA_ERROR_MASK);
            return ICRAFT_DEVICE_CDMA_ERROR;  
        }
        if((st & DMA_STATE_MASK) == DMA_STATE_EXPECT){
            icraft_print("[DEVICE] cdma state: 0x%X, after mask: %X\r\n", st, st & DMA_ERROR_MASK);
            return ICRAFT_SUCCESS;
        }
    }
    uint32_t v1, v2, v3, v4, v5, v6;
    icraft_device_read_reg_relative(DEV_STATE_ADDR, &v1);
    icraft_device_read_reg_relative(DMA_STATE_ADDR, &v2);
    icraft_device_read_reg_relative(HP_RDMA_NUM, &v3);
    icraft_device_read_reg_relative(HP_WDMA_NUM, &v4);
    icraft_device_read_reg_relative(DDR_RDMA_NUM, &v5);
    icraft_device_read_reg_relative(DDR_WDMA_NUM, &v6);
    fmsh_print("cmda timeout!\n PLDDR_STATUS: 0x%X, DMA_STATE_ADDR(expected:0x2200): 0x%X\n, HP_RDMA_NUM: %d, HP_WDMA_NUM: %d, DDR_RDMA_NUM: %d, DDR_WDMA_NUM: %d\r\n",
               v1, v2, v3, v4, v5, v6);
    return ICRAFT_DEVICE_CDMA_TIMEOUT;
}

icraft_return
_icraft_device_read_cdma(char *dest, uint32_t src, uint32_t size){
    icraft_print("[DEVICE] cdma read ---from %u(0x%X)(plddr), ---to %u(0x%X)(psddr), ---size %d(0x%X)\r\n", 
                src, src, dest, dest, size, size);

    uint32_t hp0_wdma_from = (uint32_t)dest;  // udma buffer
    uint32_t hp0_wdma_to = hp0_wdma_from + size - HP0_DMA_BL;
    uint32_t ddr_rdma_from = src;
    uint32_t ddr_rdma_to = ddr_rdma_from + size - DDR_DMA_BL;

    icraft_print("[DEVICE] hp0_wdma_from: %u(0x%X), hp0_wdma_to: %u(0x%X), ddr_rdma_from: %u(0x%X), ddr_rdma_to: %u(0x%X)\r\n", 
                hp0_wdma_from, hp0_wdma_from, hp0_wdma_to, hp0_wdma_to, 
                ddr_rdma_from, ddr_rdma_from, ddr_rdma_to, ddr_rdma_to);

    icraft_return ret;

    Fmsh_DCacheFlushRange(hp0_wdma_from, size);
    //icraft_cache_invalidate(hp0_wdma_from, size);

    icraft_device_write_reg_relative(HP0_WDMA_FROM_ADDR, hp0_wdma_from);
    icraft_device_write_reg_relative(HP0_WDMA_TO_ADDR, hp0_wdma_to);
    icraft_device_write_reg_relative(DDR_RDMA_FROM_ADDR, ddr_rdma_from);
    icraft_device_write_reg_relative(DDR_RDMA_TO_ADDR, ddr_rdma_to);
    icraft_device_write_reg_relative(HOST_CMD_ADDR, DDR_2_HP0_CMD);
    icraft_device_write_reg_relative(HOST_CMD_ADDR, 0);

    uint32_t check_hp0_wdma_from;
    icraft_device_read_reg_relative(HP0_WDMA_FROM_ADDR, &check_hp0_wdma_from);
    icraft_print(">>>>check hp0 wdma from: %u(0x%X)\r\n", check_hp0_wdma_from, check_hp0_wdma_from);

    ret = _icraft_device_wait_cdma_done(10000);
    if(ret){
        icraft_print("cdma read failed, err code: %d\r\n", ret);
        return ret;
    }
    return ICRAFT_SUCCESS;
}


icraft_return
_icraft_device_write_cdma(uint32_t dest, char *src, uint32_t size){
    icraft_print("[DEVICE] cmda write ---from %u(0x%X)(psddr), ---to %u(0x%X)(plddr), ---size%u(0x%X)\r\n", 
                src, src, dest, dest, size, size);

    uint32_t hp0_rdma_from = (uint32_t)src;
    uint32_t hp0_rdma_to = hp0_rdma_from + size - HP0_DMA_BL;
    uint32_t ddr_wdma_from = dest;
    uint32_t ddr_wdma_to = ddr_wdma_from + size - DDR_DMA_BL;    

    icraft_return ret;

    Fmsh_DCacheFlushRange(hp0_rdma_from, size);
    //icraft_cache_invalidate(hp0_rdma_from, size);

    icraft_device_write_reg_relative(HP0_RDMA_FROM_ADDR, hp0_rdma_from);
    icraft_device_write_reg_relative(HP0_RDMA_TO_ADDR, hp0_rdma_to);
    icraft_device_write_reg_relative(DDR_WDMA_FROM_ADDR, ddr_wdma_from);
    icraft_device_write_reg_relative(DDR_WDMA_TO_ADDR, ddr_wdma_to);
    icraft_device_write_reg_relative(HOST_CMD_ADDR, HP0_2_DDR_CMD);
    icraft_device_write_reg_relative(HOST_CMD_ADDR, 0);

    ret = _icraft_device_wait_cdma_done(10000);
    if(ret){
        icraft_print("cdma write failed, err code: %d\r\n", ret);
        return ret;
    }
    return ICRAFT_SUCCESS;
}


icraft_return 
icraft_device_read_plmem(char *dest, uint32_t src, uint32_t size){
    icraft_print("[DEVICE] reading memory ---from %u(0x%X)(plddr), ---to %u(0x%X)(psddr), ---size %d(0x%X)...\r\n",
                src, src, dest, dest, size, size);
    int8_t* rem_buf = (int8_t*)aligned_alloc(CACHE_ALIGN_SIZE, 64);
    if(rem_buf == NULL){
        icraft_print("malloc rem_buf failed, size ---0x%X\r\n", 64);
        return ICRAFT_DEVICE_MALLOC_ERROR;
    }
    icraft_return ret;
    uint32_t addr0_ofst = src % 64;
    if(addr0_ofst > 0){
        uint32_t size0 = 64 - addr0_ofst;
        src -= addr0_ofst;
        ret = _icraft_device_read_cdma(rem_buf, src, 64);
        if(ret){
            free(rem_buf);
            return ret;
        }
        if(size <= size0){
            memcpy(dest, rem_buf + addr0_ofst, size);
            free(rem_buf);
            icraft_print("[DEVICE] read memory ---from %u(0x%X)(plddr), ---to %u(0x%X)(psddr), ---size %d(0x%X) success!\r\n",
                        src, src, dest, dest, size, size);
            return ICRAFT_SUCCESS;
        }
        else{
            memcpy(dest, rem_buf + addr0_ofst, size0);
            size -= size0;
            dest += size0;
            src += 64;
        }
    }
    uint32_t size_rem = size % 64;
    size = size - size_rem;
    if(size > 0){
        int8_t *output_ptr = dest;
        if(addr0_ofst > 0){
            int8_t *buffer = (int8_t*)aligned_alloc(CACHE_ALIGN_SIZE, size); // size一定是64整数倍
            output_ptr = buffer;
        }
        uint32_t cur_base = src;
        uint32_t cur_size = size;
        int8_t *cur_buf = output_ptr;
        while(cur_size > DMA_SWAP_SIZE){
            icraft_print("cur_size bigger than DMA_SWAP_SIZE\r\n");
            _icraft_device_read_cdma(cur_buf, cur_base, DMA_SWAP_SIZE);
            cur_buf = cur_buf + DMA_SWAP_SIZE;
            cur_base = cur_base + DMA_SWAP_SIZE;
            cur_size = cur_size - DMA_SWAP_SIZE;
        }
        icraft_print("cur_size smaller than DMA_SWAP_SIZE\r\n");
        _icraft_device_read_cdma(cur_buf, cur_base, cur_size);
        if(addr0_ofst > 0){
            memcpy(dest, output_ptr, size);
            free(output_ptr);
        }
        src += size;
        dest += size;
    }
    if(size_rem > 0){
        _icraft_device_read_cdma(rem_buf, src, 64);
        memcpy(dest, rem_buf, size_rem);
    }
    free(rem_buf);
    icraft_print("[DEVICE] read memory ---from %u(0x%X)(plddr), ---to %u(0x%X)(psddr), ---size %u(0x%X) success!\r\n",
                src-size, src-size, dest-size, dest-size, size, size);
    return ICRAFT_SUCCESS;
}

icraft_return 
icraft_device_write_plmem(uint32_t dest, char *src, uint32_t size){
    icraft_print("[DEVICE] writing memory ---from %u(0x%X)(psddr), ---to %u(0x%X)(plddr), ---size %u(0x%X)...\r\n", 
                src, src, dest, dest, size, size);

    icraft_return ret;
    int8_t* rem_buf = (int8_t*)aligned_alloc(CACHE_ALIGN_SIZE, 64);
    uint32_t addr0_ofst = dest % 64;
    if(addr0_ofst > 0){
        uint32_t size0 = 64 - addr0_ofst;
        dest -= addr0_ofst;
        ret = _icraft_device_read_cdma(rem_buf, dest, 64);
        if(ret){
            fmsh_print("device read cdma fail, device err code:%u\r\n", ret);
            free(rem_buf);
            return ICRAFT_DEVICE_READ_CDMA_FAIL;
        }
        if(size < size0){
            memcpy(rem_buf + addr0_ofst, src, size);
            ret = _icraft_device_write_cdma(dest, rem_buf, 64);
            if(ret){
                fmsh_print("device write cdma fail, device err code:%u\r\n", ret);
                free(rem_buf);
                return ICRAFT_DEVICE_WRITE_CDMA_FAIL;
            }
            icraft_print("[DEVICE] write memory ---from %u(0x%X)(psddr), ---to %u(0x%X)(plddr), ---size %d(0x%X) success!\r\n", 
            src, src, dest, dest, size, size);
            return ICRAFT_SUCCESS;
        }
        else{
            memcpy(rem_buf + addr0_ofst, src, size0);
            ret = _icraft_device_write_cdma(dest, rem_buf, 64);
            if(ret){
                fmsh_print("device write cdma fail, device err code:%u\r\n", ret);
                free(rem_buf);
                return ICRAFT_DEVICE_WRITE_CDMA_FAIL;
            }
            size -= size0;
            dest += 64;
            src += size0;
        }
    }
    uint32_t size_rem = size % 64;
    size -= size_rem;
    if(size > 0){
        uint32_t cur_base = dest;
        uint32_t cur_size = size;
        int8_t *cur_buf = src;
        while(cur_size > DMA_SWAP_SIZE){
            ret = _icraft_device_write_cdma(cur_base, cur_buf, DMA_SWAP_SIZE);
            if(ret){
                fmsh_print("device write cdma fail, device err code:%u\r\n", ret);
                free(rem_buf);
                return ICRAFT_DEVICE_WRITE_CDMA_FAIL;
            }
            cur_buf = cur_buf + DMA_SWAP_SIZE;
            cur_base = cur_base + DMA_SWAP_SIZE;
            cur_size = cur_size - DMA_SWAP_SIZE;
        }
        ret = _icraft_device_write_cdma(cur_base, cur_buf, cur_size);
        if(ret){
            fmsh_print("device write cdma fail, device err code:%u\r\n", ret);
            free(rem_buf);
            return ICRAFT_DEVICE_WRITE_CDMA_FAIL;
        }
        src += size;
        dest += size;
    }
    if(size_rem){
        ret = _icraft_device_read_cdma(rem_buf, dest, 64);
        if(ret){
            fmsh_print("device read cdma fail, device err code:%u\r\n", ret);
            free(rem_buf);
            return ICRAFT_DEVICE_READ_CDMA_FAIL;
        }
        memcpy(rem_buf, src, size_rem);
        ret = _icraft_device_write_cdma(dest, rem_buf, 64);
        if(ret){
            fmsh_print("device write cdma fail, device err code:%u\r\n", ret);
            free(rem_buf);
            return ICRAFT_DEVICE_WRITE_CDMA_FAIL;
        }
    }
    free(rem_buf);
    icraft_print("[DEVICE] write memory ---from %u(0x%X)(psddr), ---to %u(0x%X)(plddr), ---size %d(0x%X) success!\r\n", 
    src-size, src-size, dest-size, dest-size, size, size);
    return ICRAFT_SUCCESS;
}


icraft_return 
icraft_device_reset(const uint32_t level){
    icraft_print("reset device with level %d begin...\r\n", level);
    icraft_return ret;
    switch (level){
    case 0:
		icraft_device_write_reg_relative(FPGA_RESET_REG, 1);
		icraft_device_write_reg_relative(BUYI_RESET_REG, 1);
		icraft_device_write_reg_relative(MMU_SOFT_RESET, 1);
		icraft_device_write_reg_relative(FPGA_RESET_REG, 0);
		icraft_device_write_reg_relative(BUYI_RESET_REG, 0);
		icraft_device_write_reg_relative(MMU_SOFT_RESET, 0);
		icraft_device_write_reg_relative(DDR_STATE_CLR_REQ, 1);
		icraft_device_write_reg_relative(DDR_STATE_CLR_REQ, 0);
        icraft_print("[DEVICE] reset device(level %u) success!\r\n", level);
        return ICRAFT_SUCCESS;
    case 1:
		icraft_device_write_reg_relative(DDR_STATE_CLR_REQ, 1);
		icraft_device_write_reg_relative(DDR_STATE_CLR_REQ, 0);
        icraft_print("[DEVICE] reset device(level %u) success!\r\n", level);
        return ICRAFT_SUCCESS;
    default:
        icraft_print("[DEVICE] invalid reset level (get %u), only supports level 0 or 1\r\n", level);
        return ICRAFT_DEVICE_INVALID_RESET_LEVEL;
    }
}

icraft_return
icraft_device_dma_check(const uint32_t byte_size, const uint32_t dest){
    int8_t *data = (int8_t*)aligned_alloc(CACHE_ALIGN_SIZE, align64(byte_size));
    if(data == NULL){
        icraft_print("malloc data for dma check failed, size=%u\r\n", align64(byte_size));
        return ICRAFT_DEVICE_MALLOC_ERROR;
    }
    for(uint32_t i = 0; i < byte_size; ++i){
        data[i] = 0x40 + (int8_t)(byte_size % i);
    }
    icraft_device_write_plmem(dest, (char*)data, byte_size);

    int8_t *buffer = (int8_t*)aligned_alloc(CACHE_ALIGN_SIZE, align64(byte_size));
    if(buffer == NULL){
        icraft_print("malloc buffer for dma check failed, size=%u\r\n", align64(byte_size));
        return ICRAFT_DEVICE_MALLOC_ERROR;
    }
    icraft_device_read_plmem(buffer, dest, byte_size);

    icraft_print("comparing golden data(0x%llX) with dma data(0x%llX), with size %u...\r\n", data, buffer, byte_size);
    icraft_print_mem(data, 64);
    icraft_print_mem(buffer, 64);
    int ret = memcmp(data, buffer, byte_size);  
    if(ret != 0){
        free(buffer);
        free(data);
        icraft_print("cdma check ---plddr addr=%d, ---byte size=%d failed, data inconsistent\r\n", dest, byte_size);
        return ICRAFT_DEVICE_DMA_DATA_NOT_SAME;
    }
    icraft_print("compare golden data with dma data success!\r\n");
    free(buffer);
    free(data);
    return ICRAFT_SUCCESS;
}

icraft_return
icraft_device_icore_calc(const uint32_t addr, const uint32_t size){
#if ICRAFT_DEBUG
    icraft_print("run icore ---addr=%d, ---size=%d\r\n", addr, size);
#endif
    if(addr % 32 != 0 ){
        fmsh_print("addr ---0x%d is not aligned to 32\r\n", addr);
        return ICRAFT_DEVICE_AICORE_INVALID_ADDR;
    }
    if(size % 32 != 0 || size == 0){
        fmsh_print("size ---0x%d is not aligned to 32\r\n", size);
        return ICRAFT_DEVICE_AICORE_INVALID_SIZE;
    }
    icraft_device_write_reg_relative(BUYI_REG_BASE + 2 * 4, addr / 32);
    icraft_device_write_reg_relative(BUYI_REG_BASE + 3 * 4, size / 32);
    icraft_device_write_reg_relative(BUYI_REG_BASE + 4 * 4, 1);	    // run instr  
    return ICRAFT_SUCCESS;
}

icraft_return
icraft_device_get_layercount(uint32_t *layer_cnt){
    icraft_return ret;
    ret = icraft_device_read_reg_relative(AWLAYER_CNT, layer_cnt);
    if(ret){
        return ICRAFT_DEVICE_GET_LAYERCOUNT_FAIL;
    }
    return ICRAFT_SUCCESS;
}

icraft_return
icraft_device_set_dtype(const uint8_t dtype){
    icraft_return ret;
    uint32_t dtype_reg = BUYI_REG_BASE + 0x8000 * 4;
    if(dtype == ICRAFT_DTYPE_SINT8){
        ret = icraft_device_write_reg_relative(dtype_reg, 0);
        if(ret){
            icraft_print("set dtype=0x%X failed\r\n", dtype);
            return ICRAFT_DEVICE_SET_DTYPE_FAIL;
        }
    }
    else if(dtype == ICRAFT_DTYPE_SINT16){
        ret = icraft_device_write_reg_relative(dtype_reg, 1);
        if(ret){
            icraft_print("set dtype=0x%X failed\r\n", dtype);
            return ICRAFT_DEVICE_SET_DTYPE_FAIL;
        }        
    }
    else{
        icraft_print("invalid dytpe=0x%X\r\n", dtype);
        return ICRAFT_DEVICE_INVALID_DTYPE;
    }
    return ICRAFT_SUCCESS;
}


icraft_return 
icraft_device_enable_mpe(const uint32_t rows, const uint32_t cols){
    if(rows > 4 || rows == 0){
        icraft_print("invalid mpe rows ---%d, only supports rows=1, 2, 3.\r\n");
        return ICRAFT_DEVICE_INVALID_MPE_PARAM;
    }
    if(cols > 4 || cols == 0){
        icraft_print("invalid mpe cols ---%d, only supports cols=1, 2, 3.\r\n");
        return ICRAFT_DEVICE_INVALID_MPE_PARAM;
    } 
	icraft_print("enable mpe with ---rows=%d, ---cols=%d\r\n", rows, cols);
    icraft_return ret;
    uint32_t value = 0x0;
	for (uint32_t i = rows; i < 4; i++) {
		uint32_t pos = 1 << (4 + i);
		value |= pos;
	}
	for (uint32_t i = cols; i < 4; i++) {
		uint32_t pos = 1 << i;
		value |= pos;
	}
    ret = icraft_device_write_reg_relative(MPE_REG, value);
    if(ret){
        icraft_print("write reg ---0x%X failed\r\n", MPE_REG);
        icraft_print("enable mpe ---rows=%d, ---cols=%d failed\r\n", rows, cols);
        return ICRAFT_DEVICE_ENABLE_MPE_FAIL;
    }
    return ICRAFT_SUCCESS;
}

icraft_return 
icraft_device_mmu_get_mode(uint32_t *mode){
    icraft_return ret;
    ret = icraft_device_read_reg_relative(MMU_MODE, mode);
    if(ret){
        icraft_print("get mmu mode failed, device err code: %u\r\n", ret);
        return ICRAFT_DEVICE_GET_MMU_MODE_FAIL;
    }
    if(((*mode) != (uint32_t)ICRAFT_DEVICE_MMU_OPEN) && 
        (*mode) != (uint32_t)ICRAFT_DEVICE_MMU_CLOSE){
        icraft_print("unknown mmu state: 0x%X\r\n", *mode);
        return ICRAFT_DEVICE_MMU_UNKNOWN_MODE;
    }
    return ICRAFT_SUCCESS;
}


icraft_return 
icraft_device_mmu_open(){
    icraft_return ret;
    uint32_t mode = (uint32_t)ICRAFT_DEVICE_MMU_OPEN;
    ret = icraft_device_write_reg_relative(MMU_MODE, mode);
    if(ret){
        icraft_print("[DEVICE] write mmu mode reg failed, err code: %u\r\n", ret);
        return ICRAFT_DEVICE_OPEN_MMU_FAIL;
    }    
#if ICRAFT_DEBUG
    uint32_t test;
    ret = icraft_device_read_reg_relative(MMU_MODE, &test);
    if(test != (uint32_t)ICRAFT_DEVICE_MMU_OPEN){
        icraft_print("[DEVICE] open mmu failed, mmu mode is still %u\r\n", test);
    }
#endif
    icraft_print("[DEVICE] open mmu success!\r\n");
    return ICRAFT_SUCCESS;
}

icraft_return 
icraft_device_mmu_close(){    
    icraft_return ret;
    uint32_t mode = (uint32_t)ICRAFT_DEVICE_MMU_CLOSE;
    ret = icraft_device_write_reg_relative(MMU_MODE, mode);
    if(ret){
        icraft_print("[DEVICE] write mmu mode reg failed, err code: %u\r\n", ret);
        return ICRAFT_DEVICE_CLOSE_MMU_FAIL;
    }    
#if ICRAFT_DEBUG
    uint32_t test;
    ret = icraft_device_read_reg_relative(MMU_MODE, &test);
    if(test != (uint32_t)ICRAFT_DEVICE_MMU_CLOSE){
        icraft_print("[DEVICE] close mmu failed, mmu mode is still %u\r\n", test);
    }
#endif
    return ICRAFT_SUCCESS;
}

icraft_return
icraft_device_mmu_update_table(const icraft_device_mmu_table_t* const mmu_table){
    
    
    uint64_t logic_buf_addr = MMU_LOG_BASE0;
    uint64_t pldf_buf_addr = MMU_PHY_BASE0;
    for(uint32_t i = 0; i < MMU_SIZE; ++i){
        uint32_t diff = mmu_table->phy_bases[i] - 
            mmu_table->logic_bases[i];
        icraft_device_write_reg_relative(logic_buf_addr, mmu_table->logic_bases[i]);
        icraft_device_write_reg_relative(pldf_buf_addr, diff);
        logic_buf_addr += 4;
        pldf_buf_addr += 4;
    }
    icraft_device_write_reg_relative(MMU_GP_START, 0);
    return ICRAFT_SUCCESS;
    
}


void icraft_device_close(){
    g_icraft_device_isopen = ICRAFT_FALSE;
}
