/**
 * @file icraft_device.h
 * @author Junkun Wang (wangjunkun@fmsh.com.cn)
 * @brief Icraft3.x Icraft Device相关结构及其方法
 * @date 2024-06-30
 * 
 * @copyright Copyright (c) 2023 Shanghai Fudan Microelectronics Group Co., Ltd. All rights reserved.
 * 
 */

#ifndef _ICRAFT_DEVICE_H_
#define _ICRAFT_DEVICE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdlib.h>

#include "ps_init.h"
#include "fmsh_common.h"

#include "icraft_reg.h"
#include "icraft_errno.h"
#include "icraft_type.h"
#include "platform/fmsh_print.h"
#include "utils/icraft_print.h"
#include "utils/icraft_cache.h"
#include "utils/icraft_common.h"

#define MIN_PLDDR_ADDR              (uint32_t)(4096)
#define MAX_PLDDR_ADDR              (uint32_t)(2*1024*1024*1024)
#define MAX_PLDDR_DATA_SIZE         (MAX_PLDDR_ADDR - MIN_PLDDR_ADDR)
#define ICRAFT_DEVICE_MMU_CAPACITY  4  

extern const uint32_t gp0_base;             //> 0x40000000
extern const uint32_t gp1_base;             //> 0x80000000
extern uint8_t g_icraft_device_isopen;      //> 全局变量：device是否已打开
extern uint8_t g_icraft_device_mmu_isopen;  //> 全局变量：icore的mmu是否已使能

/**
 * @brief Device版本信息
 */
typedef struct{
    uint32_t device_version;
    char icore_version[18];
    uint32_t mmu_version;
}icraft_device_version_t;


extern icraft_device_version_t g_icraft_dev_versions;  //> 已打开device的版本信息


/**
 * @brief Icore使用的MMU表
 * 
 */
typedef struct{
    uint32_t logic_bases[MMU_SIZE];   //< 逻辑地址
    uint32_t phy_bases[MMU_SIZE];     //< 映射成的物理地址
}icraft_device_mmu_table_t;


/**
 * @brief   打开Device
 * 
 * @return  错误码
 * @retval  成功时返回ICRAFT_SUCCESS，失败时返回其它错误状态，详见icraft_errno.h   
 * 
 * @note    打开成功后会将g_icraft_device_isopen置为ICRAFT_TRUE
 */
icraft_return
icraft_device_open();

/**
 * @brief   关闭Device
 * @note    关闭后会将g_icraft_device_isopen置为ICRAFT_FALSE
 * @return  无
 */
void icraft_device_close();

/**
 * @brief   重置Device状态
 * 
 * @param   [in]    level    0： 重置所有状态；1：仅重置DDR状态
 * @return  错误码
 * @retval  成功时返回ICRAFT_SUCCESS，失败时返回其它错误状态，详见icraft_errno.h   
 */
icraft_return
icraft_device_reset(const uint32_t level);

/**
 * @brief   通过绝对地址读取寄存器
 * 
 * @param   [in]    addr    寄存器的绝对地址
 * @param   [out]   value   读取到的数值
 * 
 * @return  错误码
 * @retval  成功时返回ICRAFT_SUCCESS，失败时返回其它错误状态，详见icraft_errno.h  
 */
icraft_return
icraft_device_read_reg(const uint64_t addr, uint32_t *value);

/**
 * @brief   通过相对地址读取寄存器
 * 
 * @param   [in]    addr    寄存器的绝对地址
 * @param   [out]   value   读取到的数值
 * 
 * @return  错误码
 * @retval  成功时返回ICRAFT_SUCCESS，失败时返回其它错误状态，详见icraft_errno.h  
 * 
 * @note    当前相对地址需要33位来表示，因此使用64位无符号整型来保存地址
 */
icraft_return
icraft_device_read_reg_relative(const uint64_t addr, uint32_t *value);


/**
 * @brief   通过绝对地址写寄存器
 * 
 * @param   [in]    addr    寄存器的绝对地址
 * @param   [in]    value   写到寄存器的数值
 * 
 * @return  错误码
 * @retval  成功时返回ICRAFT_SUCCESS，失败时返回其它错误状态，详见icraft_errno.h  
 */
icraft_return
icraft_device_write_reg(const uint64_t addr, const uint32_t value);


/**
 * @brief   通过相对地址写寄存器
 * 
 * @param   [in]    addr    寄存器的绝对地址
 * @param   [in]    value   写到寄存器的数值
 * 
 * @return  错误码
 * @retval  成功时返回ICRAFT_SUCCESS，失败时返回其它错误状态，详见icraft_errno.h 
 * 
 * @note    当前相对地址需要33位来表示，因此使用64位无符号整型来保存地址 
 */
icraft_return
icraft_device_write_reg_relative(const uint64_t addr, const uint32_t value);


/**
 * @brief   通过cdma读取PL DDR数据到PS DDR
 * 
 * @param   [in]    dest    PS DDR存储数据的地址（目标地址）
 * @param   [in]    src     PL DDR的地址（源地址）
 * @param   [out]   size    读取的数据长度   
 * 
 * @return  错误码
 * @retval  成功时返回ICRAFT_SUCCESS，失败时返回其它错误状态，详见icraft_errno.h 
 * 
 * @note    [1] 当前PL DDR最大也就支持到4G地址空间，因此使用32位无符号数表示src 
 * @note    [2] 内部接口
 */
icraft_return
_icraft_device_read_cdma(char *dest, uint32_t src, uint32_t size);

/**
 * @brief   通过cdma写PS DDR数据到PL DDR
 * 
 * @param   [in]    dest    PL DDR的地址（目标地址）
 * @param   [in]    src     PS DDR的地址（源地址）
 * @param   [out]   size    读取的数据长度   
 * 
 * @return  错误码
 * @retval  成功时返回ICRAFT_SUCCESS，失败时返回其它错误状态，详见icraft_errno.h 
 * 
 * @note    [1] 当前PL DDR最大也就支持到4G地址空间，因此使用32位无符号数表示src 
 * @note    [2] 内部接口
 */
icraft_return
_icraft_device_write_cdma(uint32_t dest, char *src, uint32_t size);

/**
 * @brief   等待cdma完成读/写操作的接口
 * 
 * @param   [in]    time_ms    判断cdma超时的阈值
 * 
 * @return  错误码
 * @retval  成功时返回ICRAFT_SUCCESS，失败时返回其它错误状态，详见icraft_errno.h 
 * 
 * @note    [2] 内部接口
 */
icraft_return 
_icraft_device_wait_cdma_done(const uint32_t time_ms);


/**
 * @brief   通过cdma读取PL DDR数据到PS DDR
 * 
 * @param   [in]    dest    PS DDR存储数据的地址（目标地址）
 * @param   [in]    src     PL DDR的地址（源地址）
 * @param   [out]   size    读取的数据长度   
 * 
 * @return  错误码
 * @retval  成功时返回ICRAFT_SUCCESS，失败时返回其它错误状态，详见icraft_errno.h 
 * 
 * @note    [1] 当前PL DDR最大也就支持到4G地址空间，因此使用32位无符号数表示src和size
 */
icraft_return
icraft_device_read_plmem(char *dest, uint32_t src, uint32_t size);


/**
 * @brief   通过cdma写PS DDR数据到PL DDR
 * 
 * @param   [in]    dest    PL DDR的地址（目标地址）
 * @param   [in]    src     PS DDR的地址（源地址）
 * @param   [out]   size    读取的数据长度   
 * 
 * @return  错误码
 * @retval  成功时返回ICRAFT_SUCCESS，失败时返回其它错误状态，详见icraft_errno.h 
 * 
 * @note    [1] 当前PL DDR最大也就支持到4G地址空间，因此使用32位无符号数表示src和size
 */
icraft_return
icraft_device_write_plmem(uint32_t dest, char *src, uint32_t size);

/**
 * @brief   启动Icore运行存储在PL DDR上的指令
 * 
 * @param   [in]    addr     指令在PL DDR上的起始地址
 * @param   [in]    size     指令大小
 * 
 * @return  错误码
 * @retval  成功时返回ICRAFT_SUCCESS，失败时返回其它错误状态，详见icraft_errno.h 
 * 
 * @note    [1] 异步接口
 * @note    [2] 如果需要人为确保Icore执行完成，需要在外部调用icraft_ftmp_sync对输出ftmp进行同步
 */
icraft_return
icraft_device_icore_calc(const uint32_t addr, const uint32_t size);

/**
 * @brief   获取Icore当前已执行的层数
 * 
 * @param   [out]    layer_cnt     已执行的层数
 * 
 * @return  错误码
 * @retval  成功时返回ICRAFT_SUCCESS，失败时返回其它错误状态，详见icraft_errno.h 
 */
icraft_return
icraft_device_get_layercount(uint32_t *layer_cnt);


/**
 * @brief   设置Icore计算的数据类型
 * 
 * @param   [in]    dtype     要设置的数据类型，参考icraft_dtype_t（icraft_type.h)
 * 
 * @return  错误码
 * @retval  成功时返回ICRAFT_SUCCESS，失败时返回其它错误状态，详见icraft_errno.h 
 */
icraft_return
icraft_device_set_dtype(const uint8_t dtype);




/**
 * @brief   Device的DMA读写正确性测试函数
 * 
 * @param   [in]    byte_size     测试的读写数据量
 * @param   [in]    dest          数据要写到的PL DDR地址
 * 
 * @return  错误码
 * @retval  成功时返回ICRAFT_SUCCESS，失败时返回其它错误状态，详见icraft_errno.h 
 */
icraft_return
icraft_device_dma_check(const uint32_t byte_size, const uint32_t dest);

/**
 * @brief   获取Device的相关版本信息
 * 
 * @param   [out]    versions     版本信息，包含Device、Icore、MMU版本信息
 * 
 * @return  错误码
 * @retval  成功时返回ICRAFT_SUCCESS，失败时返回其它错误状态，详见icraft_errno.h 
 */
icraft_return
icraft_device_get_version(icraft_device_version_t *versions);


/**
 * @brief   设置Icore计算时MPE的行和列
 * 
 * @param   [in]    rows     MPE行数
 * @param   [in]    cols     MPE列数
 * 
 * @return  错误码
 * @retval  成功时返回ICRAFT_SUCCESS，失败时返回其它错误状态，详见icraft_errno.h 
 * 
 * @note    行数和列数取值范围为1、2、3、4
 */
icraft_return 
icraft_device_enable_mpe(const uint32_t rows, const uint32_t cols);

/**
 * @brief   获取MMU当前的状态，开启或关闭
 * 
 * @param   [out]    mode     MMU的状态，参考icraft_mmu_mode_t(icraft_type.h)
 * 
 * @return  错误码
 * @retval  成功时返回ICRAFT_SUCCESS，失败时返回其它错误状态，详见icraft_errno.h 
 */
icraft_return 
icraft_device_mmu_get_mode(uint32_t *mode);


/**
 * @brief   打开Icore的MMU
 * 
 * @return  错误码
 * @retval  成功时返回ICRAFT_SUCCESS，失败时返回其它错误状态，详见icraft_errno.h 
 */
icraft_return 
icraft_device_mmu_open();

/**
 * @brief   关闭Icore的MMU
 * 
 * @return  错误码
 * @retval  成功时返回ICRAFT_SUCCESS，失败时返回其它错误状态，详见icraft_errno.h 
 */
icraft_return 
icraft_device_mmu_close();

/**
 * @brief   更新MMU映射表
 * 
 * @param   [in]  table_idx   当前所使用的映射表的idx
 * 
 * @return  错误码
 * @retval  成功时返回ICRAFT_SUCCESS，失败时返回其它错误状态，详见icraft_errno.h 
 */
icraft_return
icraft_device_mmu_update_table(const icraft_device_mmu_table_t* const mmu_table);



#ifdef __cplusplus
}
#endif // __cplusplus

#endif // _ICRAFT_DEVICE_H_