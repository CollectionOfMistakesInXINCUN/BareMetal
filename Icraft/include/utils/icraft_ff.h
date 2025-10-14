/**
 * @file icraft_unpack.h
 * @author Junkun Wang (wangjunkun@fmsh.com.cn)
 * @brief Icraft3.x deserialize/unpack with flatbuffer (flatcc v0.6.1)
 * @date 2024-05-23
 * 
 * @copyright Copyright (c) 2023 Shanghai Fudan Microelectronics Group Co., Ltd. All rights reserved.
 * 
 */

#ifndef _ICRAFT_UNPACK_H_
#define _ICRAFT_UNPACK_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>

#include "ff.h"

#include "icraft_print.h"
#include "icraft_errno.h"
#include "icraft_type.h"

/**
* @brief  加载文件到内存
* 
* @param  [in]   file_path          被加载文件的路径   
* @param  [out]  buffer             文件加载到内存后的数据指针
* @param  [out]  file_size          文件自身所占的字节大小
* 
* @return 错误码
* @retval 成功时返回ICRAFT_SUCCESS，失败时返回其它错误状态，详见icraft_errno.h
* 
* @note   [1] file_size由于内存对齐的因素，与实际在内存中分配的空间大小可能不一致。
* @note   例如：文件大小为133字节，而buffer空间的大小受制于内存分配接口的限制，分配大小需要64对齐，即192字节。
* @note   [2] 接口内部会分配buffer的内存空间，因此请不要在接口外部申请buffer的使用空间，以免造成内存泄漏 
*/
icraft_return
icraft_ff_load_file(const char *file_path, void **buffer, uint32_t *file_size);


/**
* @brief  将内存数据写到文件
* 
* @param  [in]   file_path           要写数据的文件路径   
* @param  [in]   buffer              要写的内存数据指针
* @param  [in]   file_size           要写的数据字节数
* @param  [out]  written_size        实际写到文件的字节数
* 
* @return 错误码
* @retval 成功时返回ICRAFT_SUCCESS，失败时返回其它错误状态，详见icraft_errno.h
* 
* @note   [1] 若文件已存在，该接口会覆盖原有文件内容
*/
icraft_return
icraft_ff_write_buffer_to_file(const char *file_path, void *buffer, uint32_t const buffer_size, uint32_t *written_size);


/**
* @brief  创建目录
* 
* @param  [in]   file_path           待创建的目录路径   
* 
* @return 错误码
* @retval 成功时返回ICRAFT_SUCCESS，失败时返回其它错误状态，详见icraft_errno.h
* 
* @note   [1] 不支持前级目录不存在的多级目录，例如/dirA/dirB，但/dirA不存在
* @note   [2] 不支持目录最后加斜杠，例如/dirA/
*/
icraft_return 
icraft_ff_mkdir(const char *dir);


/**
 * @brief 打印指定路径的文件列表
 * 
 * @param [in]   dir   查看的路径
 * 
 * @return icraft_return 
 * @retval 成功时返回ICRAFT_SUCCESS，失败时返回其它错误状态，详见icraft_errno.h
 */
icraft_return
icraft_ff_listdir(const char *dir);


#ifdef __cplusplus
}
#endif

#endif  // _ICRAFT_UNPACK_H_
