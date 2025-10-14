/**
 * @file icraft_type.h
 * @author Junkun Wang (wangjunkun@fmsh.com.cn)
 * @brief Icraft3.x Types
 * @date 2024-04-10
 * 
 * @copyright Copyright (c) 2023 Shanghai Fudan Microelectronics Group Co., Ltd. All rights reserved.
 * 
 */

#ifndef ICRAFT_TYPE_H
#define ICRAFT_TYPE_H

#include "fmsh_common_types.h"

#define ICRAFT_TRUE   TRUE
#define ICRAFT_FALSE  FALSE

/**
 * @brief Icraft的布尔类型
 */
typedef BOOL icraft_bool_t; 

/**
 * @brief Icraft算子类型的枚举类型
 * @note  这里的顺序需要和fbs中的算子顺序对齐，否则将解析错误
 */
typedef enum{
    ICRAFT_OP_HARDOP      = 1,
    ICRAFT_OP_CAST        = 2,
    ICRAFT_OP_IMAGEMAKE   = 3,
    ICRAFT_OP_DETPOST     = 4,
    ICRAFT_OP_INPUT       = 5,
    ICRAFT_OP_OUTPUT      = 6,
    ICRAFT_OP_SEGPOST     = 15,
    ICRAFT_OP_WARPAFFINE  = 16
}icraft_op_t;

/**
 * @brief MMU的开关状态
*/
typedef enum{
    ICRAFT_DEVICE_MMU_CLOSE              = 0,    ///< DISABLE
    ICRAFT_DEVICE_MMU_OPEN               = 1,    ///< ENABLE
}icraft_mmu_mode_t;

/**
 * @brief 算子运行的后端资源类型
 */
typedef enum{
    ICRAFT_COMPILE_TARGET_BUYI             = 0,
    ICRAFT_COMPILE_TARGET_FPGA             = 1,
    ICRAFT_COMPILE_TARGET_CPU              = 2
}icraft_compile_target_t;

/**
 * @brief 数据存储的位置类型
 */
typedef enum{
    ICRAFT_MTYPE_ETM               = 0,
    ICRAFT_MTYPE_OCM               = 1,
    ICRAFT_MTYPE_HOST              = 2
}icraft_mtype_t;

/**
 * @brief 数据的精度类型
 */
typedef enum{
    ICRAFT_DTYPE_SINT8             = 0,
    ICRAFT_DTYPE_UINT8             = 1,
    ICRAFT_DTYPE_SINT16            = 2,
    ICRAFT_DTYPE_UINT16            = 3,
    ICRAFT_DTYPE_FP32              = 4
}icraft_dtype_t;

#endif // ICRAFT_TYPE_H
