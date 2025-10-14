#include "gic/icraft_gic.h"

int32_t NET1 = 0;
int32_t NET2 = 1;

uint32_t net1_cur_op = 0;
uint32_t net2_cur_op = 0;
int32_t g_icraft_gic_imk_id = 0;
int32_t g_icraft_gic_icore_id = -1;
int32_t g_icraft_gic_imk_buffer_size = 2;
icraft_bool_t g_icraft_gic_imk_pause = ICRAFT_FALSE;
icraft_bool_t g_icraft_gic_icore_ing = ICRAFT_FALSE;
icraft_queue_t *g_icraft_gic_imk_queue = NULL;
icraft_queue_t *g_icraft_gic_detpost_queue = NULL;

icraft_list_node_t** customop1;
icraft_list_node_t** customop2;

uint64_t op_time_begin = 0;
uint64_t network_time_begin = 0;


uint64_t gic_reg_base = 0x80002C00;

int times = 0;


icraft_return
icraft_gic_handler_init(
    uint32_t interrupt_id, 
    FMSH_InterruptHandler handler, 
    uint8_t trigger_type, 
    uint8_t priority, 
    void *handler_argument
){
    int32_t status = 0;
    status = FGicPs_SetupInterruptSystem(&IntcInstance);
    if (status != FMSH_SUCCESS)
    {
        icraft_print("interrupt_id: %d Gic initialization failed.\r\n", interrupt_id);
        return ICRAFT_GIC_INIT_FAIL;
    }
    FMSH_ExceptionRegisterHandler(FMSH_EXCEPTION_ID_IRQ_INT,
            (FMSH_ExceptionHandler)FGicPs_InterruptHandler_IRQ, &IntcInstance);
    status = FGicPs_Connect(&IntcInstance, interrupt_id, handler, handler_argument);
    if (status != GIC_SUCCESS)
    {
        icraft_print("interrupt_id: %d, connect failed.\r\n", interrupt_id);
        return ICRAFT_GIC_INIT_FAIL;
    }    
    FGicPs_SetPriorityTriggerType(&IntcInstance, interrupt_id, priority, trigger_type);
    FGicPs_Enable(&IntcInstance, interrupt_id);
    icraft_print("[GIC] gic init hanlder(interrupt_id=%u) success!\r\n", interrupt_id);
    return ICRAFT_SUCCESS;
}

void icraft_gic_handler_single(void *data) {
    icraft_return ret;
    icraft_return ftmp_ret;
    icraft_network_info_t *cur_net = (icraft_network_info_t*)data;
   
    icraft_list_node_t *cur_op = icraft_network_getOp(cur_net, cur_net->current_op);

    icraft_basic_op_t* cur_op_ptr = (icraft_basic_op_t*)(cur_op->data);
#if DUMP_AFTER_EACH_OP
    if (cur_op_ptr->basic_info->op_type != ICRAFT_OP_DETPOST) {
    icraft_print("[DUMP] for ofms(opid=%u) start!\r\n", cur_op_ptr->basic_info->op_id);
    for(uint32_t i = 0; i < cur_op_ptr->basic_info->ofms_num; ++i){
        ftmp_ret = icraft_ftmp_dump(cur_op_ptr->basic_info->ofms_ptr[i], cur_net->name);
        if(ftmp_ret){
            fmsh_print("[DUMP] (opid=%u) ofm(vid=%u) failed, ftmp err code: %u\r\n", 
                cur_op_ptr->basic_info->op_id, cur_op_ptr->basic_info->ofms_ptr[i]->vid, ftmp_ret);
            return;
        }
    }
    icraft_print("[DUMP] for ofms(opid=%u) success!\r\n", cur_op_ptr->basic_info->op_id);
    }
#endif
    
    if (cur_op->next == NULL) {
        icraft_print("[WARNING] network has no output op...\r\n");
        cur_net->network_done = ICRAFT_TRUE;
#if ICRAFT_LOG_TIME
    uint64_t interface_time_end = get_current_time();
    float interface_time = icraft_timer_convert_to_ms(interface_time_end, network_time_begin);
    cur_net->time_log->inferface_forward_time = interface_time;
#endif
        icraft_device_reset(1);
        return;
    }

    if(cur_op_ptr->basic_info->compile_target == ICRAFT_COMPILE_TARGET_FPGA) {
        uint32_t gic_id = 0;
        icraft_device_read_reg_relative(0x100002C00, &gic_id);
        times += 1;
        icraft_print("gic_id:%d, times:%d\r\n", gic_id, times);
    }
    
#if ICRAFT_LOG_TIME
    uint64_t op_time_end = get_current_time();
    float op_interface_time = icraft_timer_convert_to_ms(op_time_end, op_time_begin);
    cur_op_ptr->basic_info->time_log->interface_time = op_interface_time;
    
    switch(cur_op_ptr->basic_info->op_type) {
        case ICRAFT_OP_HARDOP:
            if (cur_op_ptr->basic_info->compile_target == ICRAFT_COMPILE_TARGET_BUYI) {
                uint32_t icore_time = 0;
                icraft_device_read_reg_relative(0xD0, &icore_time);
                cur_op_ptr->basic_info->time_log->hw_time = (float)icore_time * 5.0 / 1000000.0;
                cur_net->time_log->hw_time += (float)icore_time * 5.0 / 1000000.0;
            }
            else {
                uint32_t icore_time = 0;
                icraft_device_read_reg_relative(0x100001400 + 0x10, &icore_time);
                cur_op_ptr->basic_info->time_log->hw_time = (float)icore_time  / 100000.0;
                cur_net->time_log->hw_time += (float)icore_time  / 100000.0;
            }
            break;
        case ICRAFT_OP_CAST:
            uint32_t hard_time;
            icraft_device_read_reg_relative(0x100001000 + 0x38, &hard_time);
            cur_op_ptr->basic_info->time_log->hw_time = (float)hard_time * 10.0 / 1000000.0; 
            cur_net->time_log->hw_time += (float)hard_time * 10.0 / 1000000.0;
            break;
        case ICRAFT_OP_IMAGEMAKE:
            //uint32_t hard_time;
            icraft_device_read_reg_relative(0x100000400 + 0x238, &hard_time);
            cur_op_ptr->basic_info->time_log->hw_time = (float)hard_time * 10.0 / 1000000.0; 
            cur_net->time_log->hw_time += (float)hard_time * 10.0 / 1000000.0;
            break;
        case ICRAFT_OP_WARPAFFINE:
            icraft_device_read_reg_relative(WARPAFFINE_REG_BASE + 0x024, &hard_time);
            cur_op_ptr->basic_info->time_log->hw_time = (float)hard_time * 10.0 / 1000000.0;
            cur_net->time_log->hw_time += (float)hard_time * 10.0 / 1000000.0;
            break;
        case ICRAFT_OP_SEGPOST:
            icraft_device_read_reg_relative(SEGPOST_REG_BASE + 0x10, &hard_time);
            icraft_device_write_reg_relative(SEGPOST_REG_BASE + 0x10, 0);
            cur_op_ptr->basic_info->time_log->hw_time = (float)hard_time * 10.0 / 1000000.0;
            cur_net->time_log->hw_time += (float)hard_time * 10.0 / 1000000.0;
            

        default:
            break;
    }
#endif

    icraft_basic_op_t *next_op = (icraft_basic_op_t*)(cur_op->next->data);

    while(next_op->basic_info->op_type != ICRAFT_OP_OUTPUT && next_op->basic_info->compile_target == ICRAFT_COMPILE_TARGET_CPU) {
        next_op->forward(next_op);
        cur_op = cur_op->next;
        if (cur_op == NULL) {
            icraft_device_reset(1);
            return;
        }
        next_op = (icraft_basic_op_t*)(cur_op->next->data);
    }

    icraft_basic_op_t *op = (icraft_basic_op_t*)(cur_op->data); 
    if (op->basic_info->op_type == ICRAFT_OP_DETPOST) {
        icraft_detpost_t * detpost = (icraft_detpost_t *)(op);
        if(detpost->gic_round >= detpost->group_num){
            ret = icraft_gic_detpost_update_base(detpost);
#if ICRAFT_LOG_TIME
            cur_net->time_log->hw_time += op->basic_info->time_log->hw_time;
#endif
            if (ret) {
                fmsh_print("[ERROR] gic forward error in detpost update_base, error code:%u\r\n", ret);
                return;
            }
            detpost->gic_round = 0;
#if DUMP_AFTER_EACH_OP
    
    for(uint32_t i = 0; i < cur_op_ptr->basic_info->ofms_num; ++i){
        ftmp_ret = icraft_ftmp_dump(cur_op_ptr->basic_info->ofms_ptr[i], cur_net->name);
        if(ftmp_ret){
            icraft_print("[DUMP] (opid=%u) ofm(vid=%u) failed, ftmp err code: %u\r\n", 
                cur_op_ptr->basic_info->op_id, cur_op_ptr->basic_info->ofms_ptr[i]->vid, ftmp_ret);
            return;
        }
        icraft_print("[DUMP] for ofms(opid=%u) success!\r\n", cur_op_ptr->basic_info->op_id);
    }
    
#endif
        }
        else{
            ret = icraft_gic_detpost_update_base(detpost);
            ret = icraft_detpost_forward(detpost);
            if (ret) {
                fmsh_print("[ERROR] gic forward error in detpost forward, error code:%u\r\n", ret);
                return;
            }
            return;
        }
    }
    else if (op->basic_info->op_type == ICRAFT_OP_CAST) {
        icraft_device_write_reg_relative(0x100001000 + 0x018, 1);
    }
    else if (op->basic_info->op_type == ICRAFT_OP_WARPAFFINE) {
        icraft_device_write_reg_relative(WARPAFFINE_REG_BASE + 0x020, 1);
    }
    
    switch(next_op->basic_info->op_type) {
        case ICRAFT_OP_HARDOP:
            icraft_hardop_t * hardop = (icraft_hardop_t *)(next_op);
            if (hardop->basic_info->compile_target == ICRAFT_COMPILE_TARGET_BUYI) {
                uint32_t sync_index = hardop->sync_index + hardop->layer_count;
                //icraft_print("gic_sync_index: %d\r\n", sync_index);
                icraft_device_write_reg(0x400000D4, sync_index);
            }
#if ICRAFT_LOG_TIME
            op_time_begin = get_current_time();
#endif 
            ret = icraft_hardop_forward(hardop);
            if (ret) {
                fmsh_print("[ERROR] gic forward error in hardop, error code:%u\r\n", ret);
                return;
            }
            break;
        case ICRAFT_OP_OUTPUT:
            cur_net->network_done = ICRAFT_TRUE;
#if ICRAFT_LOG_TIME
    uint64_t interface_time_end = get_current_time();
    float interface_time = icraft_timer_convert_to_ms(interface_time_end, network_time_begin);
    cur_net->time_log->inferface_forward_time = interface_time;
#endif
            icraft_device_reset(1);
            break;

        default:
#if ICRAFT_LOG_TIME
            op_time_begin = get_current_time();
#endif 
            ret = next_op->forward(next_op);
            if (ret) {
                fmsh_print("[ERROR] gic forward error in opid:%d forward, error code:%u\r\n", next_op->basic_info->op_id,ret);
                return;
            }
            break;
    }
    return;
}



icraft_return 
icraft_network_gic_forward(icraft_network_info_t *network) {
    icraft_return ret;
    network->network_done = ICRAFT_FALSE;
    icraft_list_node_t* op_first = network->ops;
    if (op_first == NULL) {
        fmsh_print("[ERROR] network gic forward ops error, op_first is NULL\r\n");
        return ICRAFT_GIC_FORWARD_FAIL;
    }
    icraft_basic_op_t* input = (icraft_basic_op_t*)(op_first->data);
#if ICRAFT_LOG_TIME
    global_timer_enable();
    network_time_begin = get_current_time();
#endif

    while(input->basic_info->compile_target == ICRAFT_COMPILE_TARGET_CPU) {
        input->forward(input);
        op_first = op_first->next;
        input = (icraft_basic_op_t*)(op_first->data);
    }
    if (input->basic_info->compile_target == ICRAFT_COMPILE_TARGET_BUYI) {
        icraft_hardop_t * hardop = (icraft_hardop_t *)(input);
        uint32_t sync_index = hardop->sync_index + hardop->layer_count;
        icraft_device_write_reg(0x400000D4, sync_index);
    }
#if ICRAFT_LOG_TIME
    op_time_begin = get_current_time();
#endif
    ret = input->forward(input);
    if (ret) {
        fmsh_print("[ERROR] network gic forward error\r\n");
        return ret;
    }

}

icraft_return
icraft_network_network_check(icraft_network_info_t* network, icraft_list_node_t** customop) {
    if(network == NULL) {
        fmsh_print("[ERROR] netowrk is NULL\r\n");
        return ICRAFT_NETWORK_NULL;
    }

    if(customop == NULL) {
        fmsh_print("[ERROR] custom_id is NULL\r\n");
        return ICRAFT_NETWORK_NULL;
    }

    icraft_return dev_ret;
    uint32_t ops_num = network->ops_num;
    icraft_list_node_t *current_op = network->ops;
    uint32_t op_idx = 0;
    BOOL exist_imk = FALSE;
    BOOL exist_detpost = FALSE;
    while(current_op != NULL){
        icraft_basic_op_t *operation = (icraft_basic_op_t*)(current_op->data);
        if (operation->basic_info->op_type == ICRAFT_OP_IMAGEMAKE) {
            customop[0] = current_op;
            exist_imk = TRUE;
        }
        if (operation->basic_info->op_type == ICRAFT_OP_DETPOST) {
            customop[1] = current_op;
            exist_detpost = TRUE;
        }
        current_op = current_op->next;
    }

    if ((exist_imk && exist_detpost) == 0) {
        fmsh_print("[ERROR] this network not exist imk or detpost\r\n");
        return ICRAFT_NETWORK_NULL;
    }

    return ICRAFT_SUCCESS;
}

void icraft_gic_handler_pipeline_icore(void *data) {
    //fmsh_print("[ICORE_PIP_GIC] start\r\n");
    icraft_network_info_t **net_list = (icraft_network_info_t**)data;
    icraft_network_info_t *cur_net = net_list[g_icraft_gic_icore_id];
    //fmsh_print("g_icraft_gic_icore_id:%d\r\n", g_icraft_gic_icore_id);
    uint32_t imk_qsize = 0;
    icraft_queue_size(g_icraft_gic_imk_queue, &imk_qsize);
    if((g_icraft_gic_imk_pause == ICRAFT_TRUE) && (imk_qsize < g_icraft_gic_imk_buffer_size)){
        icraft_list_node_t* temp = g_icraft_gic_imk_id == 0 ? customop1[0] : customop2[0];
        icraft_imagemake_t *cur_imk = (icraft_imagemake_t *)(temp->data);
        icraft_imagemake_init(cur_imk);
        icraft_imagemake_forward(cur_imk);
        g_icraft_gic_imk_pause = ICRAFT_FALSE;
    }
    uint32_t cur_op_id = g_icraft_gic_icore_id == 0 ? net1_cur_op : net2_cur_op;
    icraft_list_node_t *cur_op = icraft_network_getOp(cur_net, cur_op_id);
    if (((icraft_basic_op_t*)(cur_op->data))->basic_info->op_type != ICRAFT_OP_HARDOP) {
      fmsh_print("[ERROR] pipeline_icore netowrk: %d handler current op is not hardop, is %d\r\n", 
      g_icraft_gic_icore_id, ((icraft_basic_op_t*)(cur_op->data))->basic_info->op_type);
        return;
    }
    icraft_basic_op_t *next_op = (icraft_basic_op_t*)(cur_op->next->data);
    if (next_op->basic_info->op_type == ICRAFT_OP_DETPOST) {
        icraft_device_reset(1);
    }
    if (g_icraft_gic_icore_id == 0) {
        net1_cur_op = next_op->basic_info->op_id;
    }
    else {
        net2_cur_op = next_op->basic_info->op_id;
    }
    next_op->forward(next_op);
    //fmsh_print("[ICORE_PIP_GIC] end\r\n");
}

void icraft_gic_handler_pipeline_others(void *data) {
    icraft_network_info_t **net_list = (icraft_network_info_t**)data;
    uint32_t gic_id = 0;
    icraft_return ret;
    icraft_device_read_reg_relative(0x100002C00, &gic_id);
    if (gic_id == 1) {
        ret = icraft_gic_pipeline_imagemake_handler(net_list);
    }
    else if(gic_id == 5) {
        ret = icraft_gic_pipeline_detpost_handler(net_list);
    }
    else {
        if (gic_id == 6) {
            icraft_device_write_reg_relative(CAST_REG_BASE + 0x018, 1);
        }
        else if (gic_id == 11) {
            icraft_device_write_reg_relative(WARPAFFINE_REG_BASE + 0x020, 1);
        }
        ret = icraft_gic_pipeline_others_handler(net_list);
    }
    if (ret) {
        fmsh_print("[ERROR] error in icraft_gic_handler_pipeline_others, err code: %u\r\n", ret);
    }
}

icraft_return
icraft_pipline_gic_forward(icraft_network_info_t **network) {
    icraft_return ret;
    customop1 = (icraft_list_node_t**)aligned_alloc(CACHE_ALIGN_SIZE, 2 * sizeof(icraft_list_node_t*));
    customop2 = (icraft_list_node_t**)aligned_alloc(CACHE_ALIGN_SIZE, 2 * sizeof(icraft_list_node_t*));

    ret = icraft_network_network_check(network[0], customop1);
    if (ret) {
        fmsh_print("[ERROR] error in check net1 for pipline forward, err code:%u", ret);
        return ICRAFT_GIC_INIT_FAIL;
    }

    ret = icraft_network_network_check(network[1], customop2);

    if (ret) {
        fmsh_print("[ERROR] error in check net2 for pipline forward, err code:%u", ret);
        return ICRAFT_GIC_INIT_FAIL;
    }

   

    icraft_imagemake_t *imk = (icraft_imagemake_t *)(customop1[0]->data);
    icraft_imagemake_init(imk);
    icraft_imagemake_forward(imk);
    return ICRAFT_SUCCESS;
}

icraft_return icraft_gic_pipeline_imagemake_handler(icraft_network_info_t **net_list){
    //fmsh_print("[IMK_PIP_GIC] start\r\n");
    icraft_return ret;
    icraft_network_info_t *cur_net = net_list[g_icraft_gic_imk_id];
   
    uint32_t imk_qsize;
    icraft_queue_size(g_icraft_gic_imk_queue, &imk_qsize);
    if(imk_qsize < g_icraft_gic_imk_buffer_size)  {
        if (g_icraft_gic_imk_id == NET1) {
            icraft_queue_enqueue(g_icraft_gic_imk_queue, &NET1);
        }
        else {
            icraft_queue_enqueue(g_icraft_gic_imk_queue, &NET2);
        }
        int32_t next_net_idx = g_icraft_gic_imk_id ^ 1;
        icraft_network_info_t *next_net = net_list[next_net_idx];
        icraft_list_node_t* temp = next_net_idx == 0 ? customop1[0] : customop2[0];
        icraft_imagemake_t *next_imk = (icraft_imagemake_t *)(temp->data);
        icraft_imagemake_init(next_imk);
        icraft_imagemake_forward(next_imk);
        g_icraft_gic_imk_id = next_net_idx;
    }
    else  {
        g_icraft_gic_imk_pause = ICRAFT_TRUE;
    }

    if((g_icraft_gic_icore_ing == ICRAFT_FALSE) && (imk_qsize > 0)) {
        void *net_idx;
        icraft_queue_dequeue(g_icraft_gic_imk_queue, &net_idx);
        //fmsh_print("imk dequeue net_idx:%d\r\n", *(int32_t*)net_idx);
        icraft_network_info_t *queue_net = net_list[*(int32_t*)net_idx];
        icraft_list_node_t* temp = (*(int32_t*)net_idx) == 0 ? customop1[0] : customop2[0];
        icraft_basic_op_t* imk_next_op = (icraft_basic_op_t*)(temp->next->data);
        if (imk_next_op->basic_info->op_type == ICRAFT_OP_HARDOP) {
            icraft_hardop_t *next_hardop = (icraft_hardop_t *)(imk_next_op);
            uint32_t sync_idx = next_hardop->sync_index + next_hardop->layer_count;
            icraft_device_write_reg(0x400000D4, sync_idx);
        }
        g_icraft_gic_icore_ing = ICRAFT_TRUE;
        g_icraft_gic_icore_id = *(int32_t*)net_idx;

        if (g_icraft_gic_icore_id == 0) {
            net1_cur_op = imk_next_op->basic_info->op_id;
        }
    else {
        net2_cur_op = imk_next_op->basic_info->op_id;
    }
        imk_next_op->forward(imk_next_op);
        if(g_icraft_gic_imk_pause == ICRAFT_TRUE)   {
            icraft_list_node_t* temp = g_icraft_gic_imk_id == 0 ? customop1[0] : customop2[0];
            icraft_imagemake_t *cur_imk = (icraft_imagemake_t *)(temp->data);
            icraft_imagemake_init(cur_imk);
            icraft_imagemake_forward(cur_imk);
            g_icraft_gic_imk_pause = ICRAFT_FALSE;
        }
    }
    //fmsh_print("[IMK_PIP_GIC] end\r\n");
    return ICRAFT_SUCCESS;
}

icraft_return icraft_gic_pipeline_detpost_handler(icraft_network_info_t **net_list){
    
    //icraft_network_info_t *cur_net = net_list[g_icraft_gic_icore_id];
    icraft_list_node_t* temp = g_icraft_gic_icore_id == 0 ? customop1[1] : customop2[1];
    icraft_detpost_t *detpost = (icraft_detpost_t *)(temp->data);
    //fmsh_print("[DETPOST_PIP_GIC] start round:%d\r\n", detpost->gic_round);
    //fmsh_print("g_icraft_gic_icore_id:%d, network_id:%d\r\n", g_icraft_gic_icore_id, detpost->basic_info->network_p->network_id);
    
    if(detpost->gic_round >= detpost->group_num){
        icraft_gic_detpost_update_base(detpost);
        detpost->gic_round = 0;
        g_icraft_gic_icore_ing = ICRAFT_FALSE;
        if (g_icraft_gic_icore_id == NET1) {
            icraft_queue_enqueue(g_icraft_gic_detpost_queue, &NET1);
        }
        else {
            icraft_queue_enqueue(g_icraft_gic_detpost_queue, &NET2);
        }

        uint32_t imk_qsize = 0;
        icraft_queue_size(g_icraft_gic_imk_queue, &imk_qsize);
        if(imk_qsize != 0){
            void *net_idx;
            icraft_queue_dequeue(g_icraft_gic_imk_queue, &net_idx);
            //fmsh_print("detpost currnet next_id:%d deque next_net_id:%d\r\n",detpost->basic_info->network_p->network_id, *(int32_t*)net_idx);
            //icraft_network_info_t *net = net_list[*(int32_t*)net_idx];
            g_icraft_gic_icore_id = *(int32_t*)net_idx;
            temp = g_icraft_gic_icore_id == 0 ? customop1[0] : customop2[0];
            icraft_basic_op_t *imk_next_op = (icraft_basic_op_t *)(temp->next->data);
            
            g_icraft_gic_icore_ing = ICRAFT_TRUE;
            if (imk_next_op->basic_info->op_type == ICRAFT_OP_HARDOP) {
                icraft_hardop_t *next_hardop = (icraft_hardop_t *)(imk_next_op);
                uint32_t sync_idx = next_hardop->sync_index + next_hardop->layer_count;
                icraft_device_write_reg(0x400000D4, sync_idx);
            }
            if (g_icraft_gic_icore_id == 0) {
                net1_cur_op = imk_next_op->basic_info->op_id;
            }
            else {
                net2_cur_op = imk_next_op->basic_info->op_id;
            }
            imk_next_op->forward(imk_next_op);
        }
    }
    else{
        icraft_gic_detpost_update_base(detpost);
        icraft_detpost_forward(detpost);        
    }
    //fmsh_print("[DETPOST_PIP_GIC] end round:%d\r\n", detpost->gic_round);
    return ICRAFT_SUCCESS;
}

icraft_return
icraft_gic_pipeline_others_handler(icraft_network_info_t **net_list) {
    icraft_return ret;
    icraft_network_info_t *cur_net = net_list[g_icraft_gic_icore_id];
    uint32_t cur_op_id = g_icraft_gic_icore_id == 0 ? net1_cur_op : net2_cur_op;
    icraft_list_node_t *cur_op = icraft_network_getOp(cur_net, cur_op_id);
    
    icraft_basic_op_t *next_op = (icraft_basic_op_t*)(cur_op->next->data);
    if (next_op->basic_info->op_type == ICRAFT_OP_HARDOP) {
        icraft_hardop_t *next_hardop = (icraft_hardop_t *)(next_op);
        uint32_t sync_idx = next_hardop->sync_index + next_hardop->layer_count;
        icraft_device_write_reg(0x400000D4, sync_idx);
    } 
    else if(next_op->basic_info->op_type == ICRAFT_OP_DETPOST) {
        icraft_device_reset(1);
    }
    if (g_icraft_gic_icore_id == 0) {
        net1_cur_op = next_op->basic_info->op_id;
    }
    else {
        net2_cur_op = next_op->basic_info->op_id;
    }
    next_op->forward(next_op);
    return ICRAFT_SUCCESS;
}

icraft_return
icraft_single_network_set_irq(icraft_network_info_t *network) {
    icraft_return ret;
    ret = icraft_gic_handler_init(58, icraft_gic_handler_single, 3, 16, network);
    ret = icraft_gic_handler_init(59, icraft_gic_handler_single, 3, 16, network);

    if (ret) {
        fmsh_print("[ERROR] network gic set irq error\r\n");
        return ret;
    }

    return ICRAFT_SUCCESS;
}

icraft_return
icraft_pipeline_networks_set_irq(icraft_network_info_t **network) {
    icraft_return ret;
    ret = icraft_gic_handler_init(59, icraft_gic_handler_pipeline_icore, 3, 8, network);
    ret = icraft_gic_handler_init(58, icraft_gic_handler_pipeline_others, 3, 16, network);

    if (ret) {
        fmsh_print("[ERROR] networks gic pipeline set irq error\r\n");
        return ret;
    }

    return ICRAFT_SUCCESS;
}