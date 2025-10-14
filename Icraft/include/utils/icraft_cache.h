/**
 * @file icraft_cache.h
 * @author Junkun Wang (wangjunkun@fmsh.com.cn)
 * @brief Icraft3.x flush and invalidate cache
 * @date 2024-05-20
 * 
 * @copyright Copyright (c) 2023 Shanghai Fudan Microelectronics Group Co., Ltd. All rights reserved.
 * 
 */

#ifndef _ICRAFT_CACHE_H_
#define _ICRAFT_CACHE_H_

#include "fmsh_cache.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CACHE_ALIGN_SIZE 64

#define icraft_cache_flush       Fmsh_DCacheFlush   
#define icraft_cache_invalidate  Fmsh_DCacheInvalidateRange

#ifdef __cplusplus
}
#endif // __cplusplus

#endif