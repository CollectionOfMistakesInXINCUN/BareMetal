#include "icraft_device.h"
#include "platform/fmsh_print.h"
#include "utils/icraft_print.h"
#include "icraft_type.h"

void test_device_open(){
    fmsh_print("=================TEST OPEN DEVICE===============\r\n");
    icraft_return ret;
    ret = icraft_device_open();
    if(ret){
        fmsh_print("icraft device open failed, err code: %d\r\n", ret);
    }
    else{
        fmsh_print("icraft device open success!\r\n");
        fmsh_print("device version: 0x%X\r\n", g_icraft_dev_versions.device_version);
        fmsh_print("mmu version: 0x%X\r\n", g_icraft_dev_versions.mmu_version);
    }
    if(ret == ICRAFT_SUCCESS){
        fmsh_print("[result] TEST OPEN DEVICE SUCCESS!\r\n");
    }
}

void test_device_dma(){
    fmsh_print("=================TEST CDMA===============\r\n");
    icraft_return ret;
    ret = icraft_device_open();
    if(ret){
        fmsh_print("icraft device open failed, err code: %d\r\n", ret);
    }
    uint32_t check_sizes[8] = {1, 63, 65, 1023, 1024, 9999, 1024*1024*10-3, 915776};
    for(uint32_t i = 0; i < 8; ++i){
        fmsh_print(">>>>>>>>>>>>Round %d>>>>>>>>>>>>>\r\n", i);
        ret = icraft_device_dma_check(check_sizes[i], 7336448);
        if(ret != ICRAFT_SUCCESS){
            fmsh_print("[TEST RESULT] dma check [---size %d] failed \r\n", check_sizes[i]);
        }
        else{
            fmsh_print("[TEST RESULT] dma check [---size %d] success\r\n", check_sizes[i]);
        }
    }
    if(ret == ICRAFT_SUCCESS){
        fmsh_print("[result] TEST CDMA SUCCESS!\r\n");
    }
}

void test_device_get_layercount(){
    fmsh_print("=================TEST CDMA===============\r\n");
    icraft_return ret;
    ret = icraft_device_open();
    if(ret){
        fmsh_print("icraft device open failed, err code: %d\r\n", ret);
    }
    uint32_t layercount = 999;
    ret = icraft_device_get_layercount(&layercount);
    fmsh_print("[result] layer cnt: %d\r\n", layercount);
    if(ret == ICRAFT_SUCCESS){
        fmsh_print("[result] TEST GET LAYERCOUNT SUCCESS!\r\n");
    }
}

// 不要跑这个测试，dtype reg是个广播寄存器，读出来是全F
void test_device_set_dtype(){
    fmsh_print("=================TEST SET DTYPE===============\r\n"); 
    icraft_return ret;
    ret = icraft_device_open();
    if(ret){
        fmsh_print("icraft device open failed, err code: %d\r\n", ret);
    }

    fmsh_print(">>>>>>>>>>>>TEST SET INT8>>>>>>>>>>>>>\r\n");
    uint8_t dtype = ICRAFT_DTYPE_UINT8;
    ret = icraft_device_set_dtype(dtype);  
    if(ret){
        fmsh_print("set dtype failed, err code: %d\r\n", ret);
    }
    uint32_t reg_addr = BUYI_REG_BASE + 0x8000 * 4;
    uint32_t reg_value;
    ret = icraft_device_read_reg_relative(reg_addr, &reg_value);
    if(ret){
        ret = 999;
        fmsh_print("read dtype reg failed, err code: %d\r\n", ret);
    }
    if(reg_value != ICRAFT_DTYPE_UINT8){
        fmsh_print("compare dtype reg value failed, read 0x%X from device\r\n", reg_value);
    }

    fmsh_print(">>>>>>>>>>>>TEST SET INT16>>>>>>>>>>>>>\r\n");
    dtype = ICRAFT_DTYPE_UINT16;
    ret = icraft_device_set_dtype(dtype);  
    if(ret){
        fmsh_print("set dtype failed, err code: %d\r\n", ret);
    }
    ret = icraft_device_read_reg_relative(reg_addr, &reg_value);
    if(ret){
        fmsh_print("read dtype reg failed, err code: %d\r\n", ret);
    }
    if(reg_value != ICRAFT_DTYPE_UINT16){
        ret = 999;
        fmsh_print("compare dtype reg value failed, read 0x%X from device\r\n", reg_value);
    }

    if(ret == ICRAFT_SUCCESS){
        fmsh_print("[result] TEST SET DTYPE SUCCESS!\r\n");
    }
    else{
        fmsh_print("[result] TEST SET DTYPE FAILED!\r\n");
    }
}

void test_device_enable_mpe(){
    fmsh_print("=================TEST ENABLE MPE===============\r\n"); 
    icraft_return ret;
    ret = icraft_device_open();
    if(ret){
        fmsh_print("icraft device open failed, err code: %d\r\n", ret);
    }   
    uint32_t reg_value;
    
    uint32_t mpe_rc[16][2] = {
        {1, 1},
        {1, 2},
        {1, 3},
        {1, 4},
        {2, 1},
        {2, 2},
        {2, 3},
        {2, 4},
        {3, 1},
        {3, 2},
        {3, 3},
        {3, 4},
        {4, 1},
        {4, 2},
        {4, 3},
        {4, 4}
    };

    uint32_t golden[16] = {
        0xEE,
        0xEC,
        0xE8,
        0xE0,
        0xCE,
        0xCC,
        0xC8,
        0xC0,
        0x8E,
        0x8C,
        0x88,
        0x80,
        0x0E,
        0x0C,
        0x08,
        0x00
    };

    for(uint32_t i = 0; i < 16; ++i){
        uint32_t rows = mpe_rc[i][0];
        uint32_t cols = mpe_rc[i][1];
        ret = icraft_device_enable_mpe(rows, cols);
        if(ret){
            fmsh_print("[RESULT] enable mpe failed, err code: %d\r\n", ret);
        }
        ret = icraft_device_read_reg_relative(MPE_REG, &reg_value);
        if(ret){
            fmsh_print("read mpe reg failed, err code: %d\r\n", ret);
        }        
        if(reg_value != golden[i]){
            ret = 999;
            fmsh_print("reg compare failed, ---read %X, ---golden %X\r\n", reg_value, golden[i]);
        }
        if(ret == ICRAFT_SUCCESS)
        {
            fmsh_print("[RESULT] compare reg ---rows %d, ---cols %d success\r\n", rows, cols);
        }
        else{
            fmsh_print("[RESULT] compare reg ---rows %d, ---cols %d fail\r\n", rows, cols);
        }
    }
    if(ret == ICRAFT_SUCCESS)
    {
        fmsh_print("[RESULT] TEST ENABLE MPE SUCCESS\r\n");
    }
    else{
        fmsh_print("[RESULT] TEST ENABLE MPE FAIL\r\n");
    }    
}