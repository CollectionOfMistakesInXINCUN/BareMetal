/**
 * @file icraft_ftmp.h
 * @author Junkun Wang (wangjunkun@fmsh.com.cn)
 * @brief Icraft Ftmp，描述了算子、网络的输入和输出
 * @date 2024-04-12
 * 
 * @copyright Copyright (c) 2024 Shanghai Fudan Microelectronics Group Co., Ltd. All rights reserved.
 * 
 */


#ifndef _ICRAFT_FTMP_H_
#define _ICRAFT_FTMP_H_

#include "icraft_errno.h"
#include "icraft_type.h"
#include "icraft_device.h"
#include "utils/icraft_ff.h"

//同步等待报错时间
#define TIMEOUT_THRES  (10000)


/**
* @brief Ftmp完成前向所需的信息
*/
typedef struct {
    icraft_dtype_t    dtype;            //< ftmp的数据类型
    icraft_mtype_t    mtype;            //< ftmp数据的存储位置
    uint32_t          vid;              //< ftmp的id
    uint32_t          addr;             //< ftmp的物理地址，只有存储在ETM上
    uint32_t          size;             //< ftmp的bytesize
    uint32_t          shape_len;        //< shape的长度
    int32_t           *shape;           //< ftmp的shape
    uint32_t          sync_idx;         //< ftmp的同步量
    //icraft_mtype_t    cur_mtype;        //< ftmp当前的内存类型
    uint8_t           *data;            //< ftmp的数据，只有在host才有
    uint32_t          normratio_len;    //< ftmp的normratio_data的长度
    float             *normratio_data;  //< ftmp的normratio, 主要用于量化
    uint32_t          layout_len;       //< ftmp的layout长度
    char              *layout;          //< ftmp的layout
} icraft_ftmp_info_t;


/**
 * @brief   Ftmp的同步接口，用来判断Icore是否执行完成
 * 
 * @param   [in]    sync_idx     同步量，即当Icore的layercount达到该同步量才会判定指令执行完成
 * 
 * @return  错误码
 * @retval  成功时返回ICRAFT_SUCCESS，失败时返回其它错误状态，详见icraft_errno.h
 */
icraft_return
icraft_ftmp_sync(const uint32_t sync_idx);


/**
 * @brief   Ftmp的同步接口，用来判断Icore是否执行完成
 * 
 * @param   [in]    ftmp          同步量，即当Icore的layercount达到该同步量才会判定指令执行完成
 * @param   [in]    folder_name   子文件夹名称
 * 
 * @return  错误码
 * @retval  成功时返回ICRAFT_SUCCESS，失败时返回其它错误状态，详见icraft_errno.h
 * 
 * @note    [1] 接口会先在主磁盘分区下建立dump文件夹。不会覆盖已创建的dump文件夹。
 * @note    [2] folder_name是为了区分dump目录下的多网络dump数据，通常传入网络的名称
 */
icraft_return
icraft_ftmp_dump(icraft_ftmp_info_t const * const ftmp, const char *folder_name);

icraft_return _freeFtmp(icraft_ftmp_info_t* self);


#endif // _ICRAFT_FTMP_H_