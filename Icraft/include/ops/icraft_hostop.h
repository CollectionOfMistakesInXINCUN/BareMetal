
#ifndef _ICRAFT_HOSTOP_H_
#define _ICRAFT_HOSTOP_H_

#include "icraft_type.h"
#include "icraft_errno.h"
#include "ops/icraft_basic_op.h"


/**
* @brief Hostop执行前向推理所需的信息
*/
typedef struct icraft_hostop_t{
    icraft_basic_op_info_t *basic_info;
    icraft_return (*free) (struct icraft_hostop_t* self);
    icraft_return (*forward) (struct icraft_hostop_t* self);
    icraft_return (*init) (struct icraft_hostop_t* self);
} icraft_hostop_t;


icraft_return
icraft_hostop_forward(struct icraft_hostop_t* cast);

icraft_return
icraft_hostop_init(struct icraft_hostop_t* cast);

icraft_return freeHostop(struct icraft_hostop_t* self);

#endif // ! _ICRAFT_HOSTOP_H_