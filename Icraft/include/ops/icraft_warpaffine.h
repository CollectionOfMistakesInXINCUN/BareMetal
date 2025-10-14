
#ifndef _ICRAFT_WARPAFFINE_H_
#define _ICRAFT_WARPAFFINE_H_

#include "icraft_type.h"
#include "icraft_errno.h"
#include "ops/icraft_basic_op.h"


/**
* @brief Hostop鎵ц鍓嶅悜鎺ㄧ悊鎵�渶鐨勪俊鎭�*/

typedef struct icraft_warpaffine_t {
    icraft_basic_op_info_t *basic_info;
    icraft_return (*free) (struct icraft_warpaffine_t* self);
    icraft_return (*forward) (struct icraft_warpaffine_t* self);
    icraft_return (*init) (struct icraft_warpaffine_t* self);
    
    uint64_t   warpaffine_reg_base;
    uint64_t   warpaffine_plddr_addr;
    uint64_t   MInversedArray_size;

    void*    grid_data;
    int      grid_byte_size;

    float**  M_inversed;
    int flags;					///< 设置插值方式，默认为线性插值 1-INTER_LINEAR， or 0-INTER_NEAREST
    int borderMode;				///< 边界pad方式，默认为 0-BORDER_CONSTANT:指定常量 or 1-BORDER_REPLICATE：复制最边缘像素
    int borderValue;
	

} icraft_warpaffine_t;


icraft_return
icraft_warpaffine_forward(struct icraft_warpaffine_t* self);

icraft_return
icraft_warpaffine_init(struct icraft_warpaffine_t* self);

icraft_return freeWarpaffine(struct icraft_warpaffine_t* self);

#endif // ! _ICRAFT_WARPAFFINE_H_
