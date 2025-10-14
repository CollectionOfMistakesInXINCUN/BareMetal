/**
 * @file icraft_cache.h
 * @author Junkun Wang (wangjunkun@fmsh.com.cn)
 * @brief Icraft3.x 内联函数
 * @date 2024-05-20
 * 
 * @copyright Copyright (c) 2023 Shanghai Fudan Microelectronics Group Co., Ltd. All rights reserved.
 * 
 */

#ifndef _ICRAFT_COMMON_H_
#define _ICRAFT_COMMON_H_

#ifdef __cplusplus
extern "C" {
#endif

#define align64(x) 64 * (x / 64 + 1)
#define align64_sizeof(T) 64 * (sizeof(T) / 64 + 1)

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // _ICRAFT_COMMON_H_