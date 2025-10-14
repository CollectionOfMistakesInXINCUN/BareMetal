/**
 * @file network.h
 * @author Junkun Wang (wangjunkun@fmsh.com.cn)
 * @brief Icraft3.x 中Network的定义、创建和删除
 * @date 2024-04-11
 * 
 * @copyright Copyright (c) 2024 Shanghai Fudan Microelectronics Group Co., Ltd. All rights reserved.
 * 
 */

#ifndef _ICRAFT_NETWORK_H_
#define _ICRAFT_NETWORK_H_

#ifdef __cplusplus
extern "C" {
#endif

  
#include "icraft_ops.h"
#include "icraft_ftmp.h"
#include "icraft_device.h"
#include "utils/icraft_list.h"
#include "utils/icraft_hash_table.h"
#include "utils/icraft_ff.h"
#include "icraft_cfg.h"


extern uint32_t network_sid;

typedef struct{
	float  interface_create_network_time;
	float  inferface_forward_time;
    float  memcpy_time;
    float  hw_time; 
	float  load_time;
	float  unpack_time;
	float  deploy_time;
}icraft_network_time_log_t;

/**
* @brief 网络段信息
*/
typedef struct{
	uint32_t ftmp_segment_logic_addr;        ///< 网络中间层段的逻辑地址，开启mmu时有效
	uint32_t ftmp_segment_phy_addr;          ///< 网络中间层段的物理地址，关闭mmu时有效
	uint32_t ftmp_segment_bytesize;          ///< 网络中间层段的字节大小
	uint32_t input_segment_logic_addr;       ///< 网络输入段的逻辑地址，开启mmu时有效
	uint32_t input_segment_phy_addr;         ///< 网络输入段的物理地址，关闭mmu时有效                 
	uint32_t input_segment_bytesize;         ///< 网络输入段的字节大小   
	uint32_t instr_segment_logic_addr;       ///< 指令段的逻辑地址，开启mmu时有效
	uint32_t instr_segment_phy_addr;         ///< 指令段的物理地址，关闭mmu时有效  
	uint32_t instr_segment_bytesize;         ///< 指令段的字节大小 
	uint32_t output_segment_logic_addr;      ///< 网络输出段的逻辑地址，开启mmu时有效
	uint32_t output_segment_phy_addr;        ///< 网络输出段的物理地址，关闭mmu时有效  
	uint32_t output_segment_bytesize;        ///< 网络输出段的字节大小   
	uint32_t params_segment_logic_addr;      ///< 权重段的逻辑地址，开启mmu时有效
	uint32_t params_segment_phy_addr;        ///< 权重段的物理地址，关闭mmu时有效  
	uint32_t params_segment_bytesize;        ///< 权重段的字节大小 
}icraft_segment_info_t;


/**
 * @brief 描述前向推理的网络结构和计算信息
 * 
 */
typedef struct icraft_network_info_t{
    uint8_t                 mmu_mode;       ///< 是否使用mmu
    uint8_t                 mpe_rows;       ///< MPE的行数
    uint8_t                 mpe_cols;       ///< MPE的列数
    char                    *name;          ///< 网络名称
    icraft_segment_info_t   *segments;      ///< 网络各段信息，参考icraft_segment_info_t
    icraft_list_node_t      *ops;           ///< 网络前向推理所遍历的算子列表，按照执行序排列
    icraft_hashtable_t      *ftmps;         ///< 网络所有的ftmp，哈希表元素为[vid, icraft_ftmp_info_t]
	uint32_t                ops_num;        ///< 网络所有op的个数
	uint32_t                ftmps_num;      ///< 网络所有ftmp的个数
	//icraft_op_t             *ops_type;      ///< 存放每个op的类型的数组
	uint32_t                ifms_num;       ///< 网络输入ftmps的个数
	uint32_t                ofms_num;       ///< 网络输出ftmps的个数 
	uint32_t                *ifms;          ///< 存放网络输入ftmps的id列表
	uint32_t                *ofms;          ///< 存放网络输出ftmps的id列表
	icraft_network_time_log_t    *time_log; ///< 网络各类时间log
	icraft_device_mmu_table_t *mmu_table;   ///< mmu_mode 下不为NULL
	void *buffer;							///< 反序列化flatbuffers的缓存指针
	uint32_t				network_id;
	int32_t 				current_op;
	icraft_bool_t			network_done;
} icraft_network_info_t;


/**
* @brief  从反序列化内存信息生成Network
* 
* @param  [in]   model_path         模型文件的路径  
* @param  [out]  network            创建完成的Network
* 
* @return 错误码
* @retval 成功时返回ICRAFT_SUCCESS，失败时返回其它错误状态 @see icraft_errno.h
*/
icraft_return 
icraft_network_create_from_file(const char *model_path, icraft_network_info_t **network);


icraft_return 
icraft_network_forward(icraft_network_info_t *network);


icraft_return
_icraft_network_deploy(icraft_network_info_t const *network);


/**
* @brief  将buffers的数据作为network的Host端输入
* 
* @param  [in]   network         	创建完成的Network  
* @param  [in]   buffers            作为输入的数据指针
* @param  [in]   buffers_num		输入指针数组的大小
* @param  [in]   ifm_idx_list       按顺序匹配network输入的idx，比如network有两个输入，{1，0}表示buffers[0]匹配network第二个输入，buffers[1]匹配network第一个输入
* 
* @return 错误码
* @retval 成功时返回ICRAFT_SUCCESS，失败时返回其它错误状态 @see icraft_errno.h
*/
icraft_return
icraft_network_fill_host_inputs_with_buffers(
    icraft_network_info_t *const network, 
    void **buffers, 
    uint32_t const buffers_num, 
    uint32_t const * const ifm_idx_list
);


icraft_return
_icraft_network_get_ifms_ofms_for_op(icraft_network_info_t *network,
	icraft_basic_op_info_t *op,
	icraft_ftmp_info_t **ifms,
	icraft_ftmp_info_t **ofms
);

icraft_return 
_icraft_network_append_mmu_table(icraft_network_info_t *network);


/**
* @brief  将单个Host上buffer的数据上传etm并作为network ifm_idx输入
* 
* @param  [in]   network         	创建完成的Network  
* @param  [in]   buffer             作为输入的数据指针
* @param  [in]   size				输入数据的字节大小
* @param  [in]   ifm_idx      		按顺序匹配network输入的idx，1表示buffer匹配network第二个输入，0表示匹配network第一个输入
* 
* @return 错误码
* @retval 成功时返回ICRAFT_SUCCESS，失败时返回其它错误状态 @see icraft_errno.h
*/
icraft_return
 icraft_network_fill_etm_input_with_host_buffer(
	icraft_network_info_t *network, 
    void *buffer,
	uint32_t const size,
	uint32_t const ifm_idx
);

icraft_return
_icraft_network_add_ifms_ofms_to_op(icraft_network_info_t *network , icraft_basic_op_info_t **op);

/**
* @brief  如果打开计时模式，可以在前向完成后log输出时间信息
* 
* @param  [in]   network         	创建完成的Network  
* 
* @return 错误码
* @retval 成功时返回ICRAFT_SUCCESS，失败时返回其它错误状态 @see icraft_errno.h
*/
icraft_return
icraft_network_log_time(icraft_network_info_t *network);


/**
* @brief  获取op_id的算子基类指针
* 
* @param  [in]   network         	创建完成的Network  
* @param  [in]   op_id            	需要获取的op_id
* 
* @return icraft_basic_op_t* op基类指针
*/
icraft_list_node_t* 
icraft_network_getOp(icraft_network_info_t* network, uint32_t op_id);

/**
* @brief  释放network所占的内存资源
* 
* @param  [in]   network         	创建完成的Network  
* 
* @return 错误码
* @retval 成功时返回ICRAFT_SUCCESS，失败时返回其它错误状态 @see icraft_errno.h
*/
icraft_return icraft_network_free(icraft_network_info_t *self);


#ifdef __cplusplus
}
#endif  // __cplusplus
#endif // _ICRAFT_NETWORK_H_