/**
 * @file icraft_print.h
 * @author Junkun Wang (wangjunkun@fmsh.com.cn)
 * @brief Icraft3.x print
 * @date 2024-05-20
 * 
 * @copyright Copyright (c) 2023 Shanghai Fudan Microelectronics Group Co., Ltd. All rights reserved.
 * 
 */

#ifndef _ICRAFT_PRINT_H_
#define _ICRAFT_PRINT_H_

#include "platform/fmsh_print.h"
#include "icraft_cfg.h"

#ifdef __cplusplus
extern "C" {
#endif

#if ICRAFT_DEBUG
#define icraft_print(format, ...) fmsh_print("[DEBUG]" format, ##__VA_ARGS__)
#else
#define icraft_print(format, ...)
#endif

void icraft_print_mem(void *addr, uint32_t size);

void icraft_buddha_bless();

#ifdef __cplusplus
}
#endif

#endif
