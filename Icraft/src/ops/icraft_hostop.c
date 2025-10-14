#include "ops/icraft_hostop.h"
#include "icraft_network.h"
#include <math.h>



icraft_return
icraft_hostop_forward(struct icraft_hostop_t* hostop) {
#if PLIN
    return ICRAFT_SUCCESS;
#endif
    hostop->basic_info->ofms_ptr[0]->data = hostop->basic_info->ifms_ptr[0]->data;
    return ICRAFT_SUCCESS;
}


icraft_return freeHostop(struct icraft_hostop_t* self) {
    
    icraft_return ret = self->basic_info->free(self->basic_info);
    free(self->basic_info);
    if (ret) {
        icraft_print("[Hostop FREE ERROR] error code %u\r\n", ret);
        return ICRAFT_HOSTOP_DECONSTRUCT_FAIL;
    }
    return ICRAFT_SUCCESS;
     
}

icraft_return
icraft_hostop_init(struct icraft_hostop_t* hostop) {
    return ICRAFT_SUCCESS;
}