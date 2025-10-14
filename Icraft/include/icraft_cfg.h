/**
 * @file icraft_cfg.h
 * @author Junkun Wang (wangjunkun@fmsh.com.cn)
 * @brief Icraft3.x Icraft工程中所有的配置选项，通过配置对应的宏来开启或关闭相应的功能
 * @date 2024-06-30
 * 
 * @copyright Copyright (c) 2023 Shanghai Fudan Microelectronics Group Co., Ltd. All rights reserved.
 * 
 */

#ifndef _ICRAFT_CFG_H_
#define _ICRAFT_CFG_H_

#define ICRAFT_NETWORK_NO_MEMCPY   1     //> 加载网络时权重和指令是否需要拷贝一份到Network。1：不拷贝；2：拷贝, 默认不拷贝，debug用
#define ICRAFT_LOG_TIME            1     //> 是否开启计时功能。1：开启；0：关闭
#define DUMP_AFTER_EACH_OP         0     //> 是否在每一层前向执行完成后导出结果数据。1：导出；0：不导出
#define ICRAFT_DEBUG               0     //> 是否输出详细过程log
#define PLIN                       0     //> IMK是否使用PL上的数据作为输入

#define CAM_H                      1080  //> PLIN camera height
#define CAM_W                      1920  //> PLIN camera width


#endif