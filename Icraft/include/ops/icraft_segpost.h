

#ifndef _ICRAFT_SEGPOST_H_
#define _ICRAFT_SEGPOST_H_

#include "icraft_type.h"
#include "icraft_errno.h"

#include "icraft_ftmp.h"
#include "icraft_basic_op.h"


typedef struct icraft_segpost_t{
   
    icraft_basic_op_info_t *basic_info;
    icraft_return (*free) (struct icraft_segpost_t* self);
    icraft_return (*forward) (struct icraft_segpost_t* self);
    icraft_return (*init) (struct icraft_segpost_t* self);
    int mode;
    int src_ysize;
    int src_xsize;
    int dst_ysize;
    int dst_xsize;
    BOOL align_corner;
    int sleep_time;
    int reg_base;
    uint32_t * ps_addr;
	
} icraft_segpost_t;


icraft_return 
icraft_segpost_forward(struct icraft_segpost_t* segpost);

icraft_return 
icraft_segpost_init(struct icraft_segpost_t* segpost);

icraft_return freeSegpost(struct icraft_segpost_t* self);

#endif // _ICRAFT_SEGPOST_H_
