#include "ops/icraft_basic_op.h"

#define icraft_timer_convert_to_ms(a, b) (float)((a) - (b)) / 1000.0 / GTC_CLK_FREQ

char const *g_icraft_op_type_names[] = {
    "INPUT",
    "OUTPUT",
    "HARDOP",
    "CAST",
    "IMAGEMAKE",
    "DETPOST",
    "WARPAFFINE",
    "SEGPOST"
};

icraft_return icraft_get_op_type(icraft_basic_op_t const *op, char *name)
{
    if(op == NULL){
        icraft_print("op is NULL, unable to get op type\r\n");
        return ICRAFT_OP_NULL;
    }
    if(name == NULL){
        icraft_print("buffer for type name is NULL when get op type\r\n");
        return ICRAFT_OP_GET_NAME_BUFFER_NULL;
    }

    if(op->basic_info->op_type == ICRAFT_OP_INPUT){
        strcpy(name, g_icraft_op_type_names[0]);
    }
    else if(op->basic_info->op_type == ICRAFT_OP_OUTPUT){
        strcpy(name, g_icraft_op_type_names[1]);
    }
    else if(op->basic_info->op_type == ICRAFT_OP_HARDOP){
        strcpy(name, g_icraft_op_type_names[2]);
    }
    else if(op->basic_info->op_type == ICRAFT_OP_CAST){
        strcpy(name, g_icraft_op_type_names[3]);
    }
    else if(op->basic_info->op_type == ICRAFT_OP_IMAGEMAKE){
        strcpy(name, g_icraft_op_type_names[4]);
    }
    else if(op->basic_info->op_type == ICRAFT_OP_DETPOST){
        strcpy(name, g_icraft_op_type_names[5]);
    }
    else if(op->basic_info->op_type == ICRAFT_OP_WARPAFFINE){
        strcpy(name, g_icraft_op_type_names[6]);
    }
    else if(op->basic_info->op_type == ICRAFT_OP_SEGPOST) {
        strcpy(name, g_icraft_op_type_names[7]);
    }
    else{
        icraft_print("unknown op type(%u)\r\n", op->basic_info->op_type);
        return ICRAFT_OP_UNKNOWN_TYPE;
    }
    return ICRAFT_SUCCESS;
}

icraft_return freeBasicOperation(struct icraft_basic_op_info_t* op) {
    
    free(op->time_log);

    free(op->ifms_ptr);
    free(op->ofms_ptr);

    free(op->network_p);

    return ICRAFT_SUCCESS;
}