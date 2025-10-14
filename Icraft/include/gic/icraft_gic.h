#ifndef _ICRAFT_GIC_H_
#define _ICRAFT_GIC_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "fmsh_gic.h"
#include "icraft_errno.h"

#include "icraft_network.h"
#include "utils/icraft_queue.h"


#define ICRAFT_GIC   1


extern int32_t g_icraft_gic_imk_id;
extern int32_t g_icraft_gic_icore_id;
extern int32_t g_icraft_gic_imk_buffer_size;
extern icraft_queue_t *g_icraft_gic_imk_queue;
extern icraft_queue_t *g_icraft_gic_detpost_queue;
extern icraft_bool_t g_icraft_gic_imk_pause;
extern icraft_bool_t g_icraft_gic_icore_ing;
extern uint64_t op_time_begin;
extern uint64_t network_time_begin;


/**
 * @brief 初始化中断服务函数到中断控制器
 * 
 * @param [IN] interrupt_id        中断控制号 
 * @param [IN] handler             中断处理函数 
 * @param [IN] trigger_type        中断的触发方式
 * @param [IN] priority            中断优先级
 * @param [IN] handler_argument    中断处理函数的输入参数
 * 
 * @return icraft_gic_return       Icraft中断错误号
 * @note   当前icraft中所有中断的触发方式均为b'11，即3，为上升沿触发。
 * @note   中断优先级取值范围为[0, 8, 16, 32 ..., 242, 248]，数值越小等级越高，
 * @note   当前Detpost优先级较高，设置为8；ImageMake和Icore较低，设置为16。
 */
icraft_return
icraft_gic_handler_init(
    uint32_t interrupt_id, 
    FMSH_InterruptHandler handler, 
    uint8_t trigger_type, 
    uint8_t priority, 
    void *handler_argument
);


void icraft_gic_imagemake_handler(void *networks);
void icraft_gic_detpost_handler(void *networks);
void icraft_gic_icore_handler(void *networks);

void icraft_gic_handler_single(void *data);
void icraft_gic_handler_pipeline_icore(void *data);
void icraft_gic_handler_pipeline_others(void *data);

icraft_return
icraft_gic_pipeline_imagemake_handler(icraft_network_info_t **net_list);

icraft_return
icraft_gic_pipeline_detpost_handler(icraft_network_info_t **net_list);

icraft_return
icraft_gic_pipeline_others_handler(icraft_network_info_t **net_list);



/**
 * @brief 裸机网络前向接口
 * 
 * @param [in] network          创建完成的网络
 * @return 错误码 
 */
icraft_return 
icraft_network_gic_forward(icraft_network_info_t *network);

/**
 * @brief 检查网络是否有imagemake以及detpost算子，并将算子指针保存在传入指针数组内
 * 
 * @param [in] network          创建完成的网络数组指针
 * @param [in] customop         数组指针，如果正常执行[0]为imagemake，[1]为detpost
 * @return 错误码 
 */
icraft_return
icraft_network_network_check(icraft_network_info_t* network, icraft_list_node_t** customop);


/**
 * @brief 裸机双网络流水前向接口
 * 
 * @param [in] network          创建完成的两个网络数组指针
 * @return 错误码 
 */
icraft_return
icraft_pipline_gic_forward(icraft_network_info_t **network);

icraft_return
icraft_single_network_set_irq(icraft_network_info_t *network);

icraft_return
icraft_pipeline_networks_set_irq(icraft_network_info_t **network);

#ifdef __cplusplus
}
#endif  // __cplusplus
#endif // _ICRAFT_GIC_H_