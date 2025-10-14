#include "icraft_network.h"
#include "utils/icraft_common.h"

#include "deserialize/serialize_reader.h"

#undef ns
#define ns(x) FLATBUFFERS_WRAP_NAMESPACE(flatbuffer, x)

uint32_t network_sid = 0;

typedef struct {
    uint32_t byte_size;
    uint32_t logic_addr;
    uint32_t phy_addr;
} temp_seg;

int cmp (const void *a, const void *b) {
    return ((temp_seg*)a)->logic_addr - ((temp_seg*)b)->logic_addr;
}


uint32_t ftmp_hash_func(void *raw_key, uint32_t key_size){
	uint32_t byte;
	uint32_t hash = 5381;
	char* key = raw_key;
	for (byte = 0; byte < key_size; ++byte) {
		// (hash << 5) + hash = hash * 33
		hash = ((hash << 5) + hash) ^ key[byte];
	}
	return hash;    
}


icraft_return 
icraft_network_create_from_file(const char *model_path, icraft_network_info_t **network){
#if ICRAFT_LOG_TIME
    global_timer_enable();
    uint64_t interface_time_begin = get_current_time();
#endif

    icraft_return ret;
    uint32_t model_fsize;

#if ICRAFT_LOG_TIME
    uint64_t load_time_begin = get_current_time();
#endif
    void *buffer = NULL;  
    ret = icraft_ff_load_file(model_path, &buffer, &model_fsize);
    if(ret){
      fmsh_print("[NETWORK] load model file failed, model_path:%s\r\n", model_path);
        return ICRAFT_NETWORK_LOAD_FILE_FAIL;
    }
#if ICRAFT_LOG_TIME
    uint64_t load_time_end = get_current_time();
    float load_time = icraft_timer_convert_to_ms(load_time_end, load_time_begin);
#endif

#if ICRAFT_DEBUG
    icraft_print("[NETWORK_UNPACK] load model success, model size: %u\r\n", model_fsize);
#endif

    uint32_t aligned_networt_t_size = 64 * (sizeof(icraft_network_info_t) / 64 + 1);
    *network = (icraft_network_info_t*)aligned_alloc(CACHE_ALIGN_SIZE, aligned_networt_t_size);
    if(*network == NULL){
        fmsh_print("[NETWORK_UNPACK] malloc network (size=%d) failed when unpacking model\r\n", aligned_networt_t_size);
        return ICRAFT_NETWORK_MALLOC_FAIL;
    }
    (*network)->buffer = buffer;
    icraft_list_node_t *ops_list = NULL;
    icraft_op_t *ops_type;

#if ICRAFT_LOG_TIME
    uint64_t unpack_time_begin = get_current_time();
#endif
    ns(Network_table_t) fb_network = ns(Network_as_root(buffer));
    if(fb_network == 0){
        return ICRAFT_NETWORK_UNPACK_BUFFER_UNACCESSIABLE; 
    }


    // basic network info
    (*network)->mpe_rows = ns(Network_mpe_rows(fb_network));
    (*network)->mpe_cols = ns(Network_mpe_cols(fb_network));
    (*network)->mmu_mode = ns(Network_mmu_mode(fb_network));
    (*network)->name = (char*)ns(Network_name(fb_network));
    (*network)->current_op = -1;
    (*network)->network_id = network_sid;
    (*network)->network_done = ICRAFT_FALSE;
    network_sid++;
    //fmsh_print("network_id:%d, network_sid:%d\r\n", (*network)->network_id, network_sid);


    // segment info
    uint32_t aligned_segment_t_size = 64 * (sizeof(icraft_segment_info_t) / 64 + 1);
    icraft_segment_info_t *segments = (icraft_segment_info_t*)aligned_alloc(CACHE_ALIGN_SIZE, aligned_segment_t_size);
    if(segments == NULL){
        fmsh_print("[NETWORK_UNPACK] malloc segments (size=%d) failed when unpacking model\r\n", aligned_segment_t_size);
        return ICRAFT_NETWORK_MALLOC_FAIL;
    }    
    ns(Segment_struct_t) fb_segments = ns(Network_segment(fb_network));
    segments->params_segment_bytesize = ns(Segment_params_segment_bytesize(fb_segments));
    segments->params_segment_logic_addr = ns(Segment_params_segment_logic_addr(fb_segments));
    segments->params_segment_phy_addr = ns(Segment_params_segment_phy_addr(fb_segments));
    segments->instr_segment_bytesize = ns(Segment_instr_segment_bytesize(fb_segments));
    segments->instr_segment_logic_addr = ns(Segment_instr_segment_logic_addr(fb_segments));
    segments->instr_segment_phy_addr = ns(Segment_instr_segment_phy_addr(fb_segments));
    segments->input_segment_bytesize = ns(Segment_input_segment_bytesize(fb_segments));
    segments->input_segment_logic_addr = ns(Segment_input_segment_logic_addr(fb_segments));
    segments->input_segment_phy_addr = ns(Segment_input_segment_phy_addr(fb_segments));
    segments->output_segment_bytesize = ns(Segment_output_segment_bytesize(fb_segments));
    segments->output_segment_logic_addr = ns(Segment_output_segment_logic_addr(fb_segments));
    segments->output_segment_phy_addr = ns(Segment_output_segment_phy_addr(fb_segments));
    segments->ftmp_segment_bytesize = ns(Segment_ftmp_segment_bytesize(fb_segments));
    segments->ftmp_segment_logic_addr = ns(Segment_ftmp_segment_logic_addr(fb_segments));
    segments->ftmp_segment_phy_addr = ns(Segment_ftmp_segment_phy_addr(fb_segments));
    (*network)->segments = segments;

    // ftmp
    flatbuffer_Ftmp_vec_t fb_ftmps = ns(Network_ftmps(fb_network));
    uint32_t ftmps_num = ns(Ftmp_vec_len(fb_ftmps));
    (*network)->ftmps_num = ftmps_num;

    icraft_hashtable_t *ftmps_ht;
    ret = icraft_hashtable_create(&ftmps_ht, sizeof(uint32_t), sizeof(icraft_ftmp_info_t), ftmps_num, ftmp_hash_func);
    if(ret){
        fmsh_print("[NETWORK_UNPACK] create hashtable for network ftmps failed, errno: %u\r\n", ret);
        return ret;
    }

    
    for(uint32_t i = 0; i < ftmps_num; ++i){
        flatbuffer_Ftmp_table_t fb_ftmp = ns(Ftmp_vec_at(fb_ftmps, i));
        icraft_ftmp_info_t *ftmp = (icraft_ftmp_info_t*)aligned_alloc(CACHE_ALIGN_SIZE, align64_sizeof(icraft_ftmp_info_t));
        ftmp->dtype = ns(Ftmp_dtype(fb_ftmp));
        ftmp->mtype = ns(Ftmp_mtype(fb_ftmp));
        ftmp->vid = ns(Ftmp_v_id(fb_ftmp));
        ftmp->addr = ns(Ftmp_addr(fb_ftmp));
        ftmp->size = ns(Ftmp_size(fb_ftmp));
        ftmp->shape_len = flatbuffers_int32_vec_len(flatbuffer_Ftmp_shape(fb_ftmp));
        ftmp->shape = (int32_t*)ns(Ftmp_shape(fb_ftmp));
        ftmp->normratio_len = flatbuffers_float_vec_len(ns(Normratio_data(ns(Ftmp_normratio(fb_ftmp)))));
        ftmp->normratio_data = (float*)(ns(Normratio_data(ns(Ftmp_normratio(fb_ftmp)))));
        ftmp->sync_idx = 0;
        ftmp->layout_len = flatbuffers_char_vec_len(flatbuffer_Ftmp_layout(fb_ftmp));
        ftmp->layout = (char*)ns(Ftmp_layout(fb_ftmp));
        ret = icraft_hashtable_insert(ftmps_ht, &ftmp->vid, ftmp);
        if(ret){
            fmsh_print("[NETWORK_UNPACK] insert ftmp (key=%u) failed, errno: %u\r\n", ftmp->vid, ret);
            return ret;
        }
        else{
#if ICRAFT_DEBUG
            icraft_print("[NETWORK_UNPACK] insert ftmp (key=%u) success\r\n", ftmp->vid);
            icraft_print("[NETWORK_UNPACK] vid: %u, mtype: %u, dtype: %u, addr: 0x%llX, size: %u(0x%llx), shape len: %u, layout: %s\r\n",
                ftmp->vid, ftmp->mtype, ftmp->dtype, ftmp->addr, ftmp->size, ftmp->size, ftmp->shape_len, ftmp->layout);
            uint32_t search_key = ftmp->vid;
            void *search_buffer;
            ret = icraft_hashtable_search(ftmps_ht, &search_key, &search_buffer);
            icraft_ftmp_info_t *search_ftmp = (icraft_ftmp_info_t*)search_buffer;
            icraft_print("[NETWORK_UNPACK_CHECK] vid: %u, mtype: %u, dtype: %u, addr: 0x%llX, size: %u(0x%llX), shape len: %u\r\n",
                search_ftmp->vid, search_ftmp->mtype, search_ftmp->dtype, 
                search_ftmp->addr, search_ftmp->size, search_ftmp->size, search_ftmp->shape_len);
#endif
        }
    }
    
    (*network)->ftmps = ftmps_ht;
#if ICRAFT_DEBUG
    icraft_print("[NETWORK_UNPACK] ftmp num(network): %u\r\n", ftmps_num);
    icraft_print("[NETWORK_UNPACK] ftmp num(hashtable): %u\r\n", ftmps_ht->len);
#endif
    // ops
    ns(Op_vec_t) fb_ops = ns(Network_ops(fb_network));
    uint32_t ops_num = ns(Op_vec_len(fb_ops));
    (*network)->ops_num = ops_num;


    for(uint32_t i = 0; i < ops_num; ++i){
#if ICRAFT_DEBUG
        icraft_print("-----------------------------------------\r\n");
#endif
        ns(Op_table_t) fb_op = ns(Op_vec_at(fb_ops, i));
        icraft_basic_op_info_t *basic_op_info = 
            (icraft_basic_op_info_t*)aligned_alloc(CACHE_ALIGN_SIZE, align64_sizeof(icraft_basic_op_info_t));

        basic_op_info->compile_target = ns(Op_compile_target(fb_op));
        basic_op_info->op_id = ns(Op_op_id(fb_op));
        basic_op_info->ifms = (uint32_t*)ns(Op_ifms(fb_op));
        basic_op_info->ofms = (uint32_t*)ns(Op_ofms(fb_op));
        basic_op_info->ifms_num = flatbuffers_uint32_vec_len(flatbuffer_Op_ifms(fb_op));
        basic_op_info->ofms_num = flatbuffers_uint32_vec_len(flatbuffer_Op_ofms(fb_op));
        basic_op_info->time_log = (icraft_time_log_t*)aligned_alloc(CACHE_ALIGN_SIZE, align64_sizeof(icraft_time_log_t));
        if(basic_op_info->time_log == NULL){
            fmsh_print("[NETWORK_UNPACK] malloc for time log fail, malloc size: %u\r\n", align64_sizeof(icraft_time_log_t));
            return ICRAFT_NETWORK_MALLOC_FAIL;
        }
        basic_op_info->time_log->interface_time = 0;
        basic_op_info->time_log->hw_time = 0;
        basic_op_info->time_log->memcpy_time = 0;
        basic_op_info->network_p = *network;
        basic_op_info->free = freeBasicOperation;

        flatbuffer_Attribute_union_type_t fb_op_type = ns(Op_attribute_type(fb_op));
        basic_op_info->op_type = fb_op_type;

        const char *op_type_str = ns(Attribute_type_name(fb_op_type));
#if ICRAFT_DEBUG
        icraft_print("[NETWORK_UNPACK] op id: %u, op type: %s, compile target: %u, ifms num: %u, ofms num: %u\r\n", 
                        basic_op_info->op_id, op_type_str, basic_op_info->compile_target, 
                        basic_op_info->ifms_num, basic_op_info->ofms_num);
        icraft_print("[NETWORK_UNPACK] ifms: \r\n");
        for(uint32_t i = 0; i < basic_op_info->ifms_num; ++i){
            uint32_t vid = basic_op_info->ifms[i];
            icraft_print("%u\r\n", vid);
        }
        icraft_print("[NETWORK_UNPACK] ofms: \r\n");
        for(uint32_t i = 0; i < basic_op_info->ofms_num; ++i){
            uint32_t vid = basic_op_info->ofms[i];
            icraft_print("%u\r\n", vid);
        }
#endif

        ret = _icraft_network_add_ifms_ofms_to_op(*network, &basic_op_info);
        if(ret){
            fmsh_print("[NETWORK UNPACK] add ifms and ofms for op(id=%u) failed, errno: %u\r\n", 
                basic_op_info->op_id, ret);
            return ICRAFT_NETWORK_PREPARE_IFMS_OFMS_FAIL; 
        }
        icraft_print("[NETWORK_UNPACK] add inputs and outputs for op(id=%u) success!\r\n", basic_op_info->op_id);

        switch (fb_op_type)
        {
        case ns(Attribute_HardOp):
            ns(HardOp_table_t) fb_hardop = ns(Op_attribute(fb_op));
            if (flatbuffers_uint8_vec_len(flatbuffer_HardOp_hdop_instr_data(fb_hardop)) == 0) {
                icraft_print("[warning] skip compile op:%d, instr_size is 0\r\n", basic_op_info->op_id);
                free(basic_op_info);
                continue;
            }
            icraft_hardop_t *hardop = 
                (icraft_hardop_t*)aligned_alloc(CACHE_ALIGN_SIZE, align64_sizeof(icraft_hardop_t));
            if(hardop == NULL){
                icraft_print("malloc memory for hardop with size %d failed\r\n", align64_sizeof(icraft_hardop_t));
                return ICRAFT_NETWORK_MALLOC_FAIL;
            }
            hardop->basic_info = basic_op_info;
            hardop->sync_index = ns(HardOp_sync_idx(fb_hardop));
            hardop->layer_count = ns(HardOp_layer_count(fb_hardop));
            hardop->instr_phy_addr = ns(HardOp_hdop_instr_physic_addr(fb_hardop));
            hardop->instr_logic_addr = ns(HardOp_hdop_instr_logic_addr(fb_hardop));
            hardop->param_addr = ns(HardOp_hdop_param_addr(fb_hardop));
            hardop->instr_size = flatbuffers_uint8_vec_len(flatbuffer_HardOp_hdop_instr_data(fb_hardop));
            hardop->param_size = flatbuffers_uint8_vec_len(flatbuffer_HardOp_hdop_param_data(fb_hardop));
            hardop->free = freeHardop;
            hardop->forward = icraft_hardop_forward;
            hardop->init = icraft_hardop_init;
#if ICRAFT_NETWORK_NO_MEMCPY
            hardop->instr_data = (uint8_t*)flatbuffer_HardOp_hdop_instr_data(fb_hardop);   // const uint8_t *
            if((uint64_t)(hardop->instr_data) % 64 != 0){
                icraft_print("[NETWORK UNPACK] ps ddr addr of hardop instr data(0x%llX) is not aligned to cacheline(=%u)\r\n", 
                            hardop->instr_data, CACHE_ALIGN_SIZE);
                return ICRAFT_NETWORK_UNPACK_ADDR_NOT_ALIGNED;
            }
#else // malloc and memcpy for instr data
            hardop->instr_data = (uint8_t*)aligned_alloc(CACHE_ALIGN_SIZE, align64(hardop->instr_size));
            if(hardop->instr_data == NULL){
                fmsh_print("[NETWORK_UNPACK] malloc memory for HARDOP(opid=%u) instr data failed, needed size=%u(aligned)\r\n", 
                    hardop->basic_info->op_id, align64(hardop->instr_size));
                return ICRAFT_NETWORK_MALLOC_FAIL;
            }
            memcpy( hardop->instr_data, flatbuffer_HardOp_hdop_instr_data(fb_hardop), hardop->instr_size);
            icraft_print("[NETWORK_UNPACK] memcpy instructions data(size=%u) from flatbuffer(0x%llX) file to ps addr 0x%llX\r\n", 
                hardop->instr_size, flatbuffer_HardOp_hdop_instr_data(fb_hardop), hardop->instr_data);            
#endif
#if ICRAFT_NETWORK_NO_MEMCPY
            hardop->param_data = (uint8_t*)flatbuffer_HardOp_hdop_param_data(fb_hardop);   // const uint8_t *
            if((uint64_t)(hardop->param_data) % 64 != 0){
                fmsh_print("[NETWORK UNPACK] ps ddr addr of hardop param data(0x%llX) is not aligned to cacheline(=%u)\r\n", 
                            hardop->param_data, CACHE_ALIGN_SIZE);
                return ICRAFT_NETWORK_UNPACK_ADDR_NOT_ALIGNED;
            }
#else // malloc and memcpy for param data
            hardop->param_data = (uint8_t*)aligned_alloc(CACHE_ALIGN_SIZE, align64(hardop->param_size));
            if(hardop->param_data == NULL){
                fmsh_print("[NETWORK_UNPACK] malloc memory for HARDOP(opid=%u) param data failed, needed size=%u(aligned)\r\n", 
                    hardop->basic_info->op_id, align64(hardop->param_size));
                return ICRAFT_NETWORK_MALLOC_FAIL;
            }
            memcpy(hardop->param_data, flatbuffer_HardOp_hdop_param_data(fb_hardop), hardop->param_size);
            icraft_print("[NETWORK_UNPACK] memcpy params data(size=%u) from flatbuffer(0x%llX) file to ps addr 0x%llX\r\n", 
                hardop->param_size, flatbuffer_HardOp_hdop_param_data(fb_hardop), hardop->param_data);
#endif
            icraft_print("[NETWORK_UNPACK] sync idx: %u, layer count: %u, instr logic addr: %u, instr phy addr: %u, param addr: %u, instr size: %u, param size: %u\r\n", 
                        hardop->sync_index, 
                        hardop->layer_count, 
                        hardop->instr_logic_addr, 
                        hardop->instr_phy_addr, 
                        hardop->param_addr,
                        hardop->instr_size,
                        hardop->param_size);
            //if (hardop->instr_size != 0) {
            
            icraft_return hardop_ret;
            hardop_ret = icraft_hardop_init(hardop);;
            if(hardop_ret){
                fmsh_print("[NETWORK] initialize hardop failed, hardop err code: %u\r\n", hardop_ret);
                return ICRAFT_NETWORK_INIT_OP_FAIL;
            }
            ret = icraft_list_append(&ops_list, hardop);
            if(ret){
                fmsh_print("[NETWORK_UNPACK] append %s, id=%u, to ops list failed, errno: %u\r\n", 
                    op_type_str, hardop->basic_info->op_id, ret);
                return ret;
            }
            
            //}
            break;
        case ns(Attribute_Cast):
         if (basic_op_info->compile_target == ICRAFT_COMPILE_TARGET_CPU) {
                //icraft_hashtable_insert((*network)->ftmps, &basic_op_info->ofms[0], basic_op_info->ifms_ptr[0]);
                icraft_print("[Warning] cast op:%d is hostop\r\n", basic_op_info->op_id);
                icraft_hostop_t *hostop = 
                (icraft_hostop_t*)aligned_alloc(CACHE_ALIGN_SIZE, align64_sizeof(icraft_hostop_t));
            if(hostop == NULL){
                fmsh_print("malloc memory for cast with size %d failed\r\n", align64_sizeof(icraft_hostop_t));
                return ICRAFT_NETWORK_MALLOC_FAIL;
            }
            hostop->basic_info = basic_op_info;
            hostop->free = freeHostop;
            hostop->forward = icraft_hostop_forward;
            hostop->init = icraft_hostop_init;

            icraft_list_append(&ops_list, hostop);
                break;
            }
            flatbuffer_Cast_table_t fb_cast = ns(Op_attribute(fb_op));
            icraft_cast_t *cast = 
                (icraft_cast_t*)aligned_alloc(CACHE_ALIGN_SIZE, align64_sizeof(icraft_cast_t));
            if(cast == NULL){
                fmsh_print("malloc memory for cast with size %d failed\r\n", align64_sizeof(icraft_cast_t));
                return ICRAFT_NETWORK_MALLOC_FAIL;
            }
            cast->basic_info = basic_op_info;
            cast->free = freeCast;
            cast->forward = icraft_cast_forward;
            cast->init = icraft_cast_init;
           
            icraft_return cast_ret;
            cast_ret = icraft_cast_init(cast);
            if(cast_ret){
                fmsh_print("[NETWORK] initialize cast failed, cast err code: %u\r\n", cast_ret);
                return ICRAFT_NETWORK_INIT_OP_FAIL;
            }
            ret = icraft_list_append(&ops_list, cast);
            if(ret){
                fmsh_print("[NETWORK_UNPACK] append %s, id=%u, to ops list failed, errno: %u\r\n", 
                    op_type_str, cast->basic_info->op_id, ret);
                return ret;
            }
            break;
        case ns(Attribute_ImageMake):
            flatbuffer_ImageMake_table_t fb_imagemake = ns(Op_attribute(fb_op));
            icraft_imagemake_t *imagemake = 
                (icraft_imagemake_t*)aligned_alloc(CACHE_ALIGN_SIZE, align64_sizeof(icraft_imagemake_t));
            if(imagemake == NULL){
                fmsh_print("malloc memory for imagemake with size %d failed\r\n", align64_sizeof(icraft_imagemake_t));
                return ICRAFT_NETWORK_MALLOC_FAIL;
            }        
            imagemake->basic_info = basic_op_info;
            imagemake->imk_reg_base = flatbuffer_ImageMake_imk_reg_base(fb_imagemake);
            imagemake->imk_input_port = flatbuffer_ImageMake_imk_input_port(fb_imagemake);
            imagemake->imk_one_fast = flatbuffer_ImageMake_imk_one_fast(fb_imagemake);
            imagemake->imk_input_bits = flatbuffer_ImageMake_imk_input_bits(fb_imagemake);

            flatbuffers_uint32_vec_t fb_pad_size = flatbuffer_ImageMake_imk_pad_size(fb_imagemake);
            imagemake->imk_pad_size = (uint32_t*)fb_pad_size;
            uint32_t pad_size_len = flatbuffers_uint32_vec_len(fb_pad_size);
            imagemake->imk_pad_size_len = pad_size_len;
            
            flatbuffers_float_vec_t fb_pre_mean = flatbuffer_ImageMake_imk_pre_mean(fb_imagemake);
            imagemake->imk_pre_mean = (float*)fb_pre_mean;        //< 直接指向内存数据
            uint32_t pre_mean_len = flatbuffers_float_vec_len(fb_pre_mean);
            imagemake->imk_pre_mean_len = pre_mean_len;
            
            flatbuffers_float_vec_t fb_pre_scale = flatbuffer_ImageMake_imk_pre_scale(fb_imagemake);
            imagemake->imk_pre_scale = (float*)fb_pre_scale;    //< 直接指向内存数据
            uint32_t pre_scale_len = flatbuffers_float_vec_len(fb_pre_scale);  
            imagemake->imk_pre_scale_len = pre_scale_len;

            imagemake->free = freeImagemake;
            imagemake->forward = icraft_imagemake_forward;
            imagemake->init = icraft_imagemake_init;

            icraft_print("reg base: %llX, input port: %d, one fast: %d, input_bis:%d\r\n", imagemake->imk_reg_base, imagemake->imk_input_port, imagemake->imk_one_fast, imagemake->imk_input_bits);
            icraft_print("pad size: \r\n"); 
            for(uint32_t i = 0; i < imagemake->imk_pad_size_len; ++i){
                icraft_print("%d\r\n", imagemake->imk_pad_size[i]);
            }
            icraft_print("pre mean: \r\n");
            for(uint32_t i = 0; i < imagemake->imk_pre_mean_len; ++i){
                icraft_print("%lf\r\n", imagemake->imk_pre_mean[i]);
            }            
            icraft_print("pre scale: \r\n");
            for(uint32_t i = 0; i < imagemake->imk_pre_scale_len; ++i){
                icraft_print("%lf\r\n", imagemake->imk_pre_scale[i]);
            }
            ret = icraft_list_append(&ops_list, imagemake);
            if(ret){
                fmsh_print("append %s, id=%u, to ops list failed, errno: %u\r\n", 
                    op_type_str, imagemake->basic_info->op_id, ret);
                return ret;
            }

            icraft_return imk_ret;
            imk_ret = icraft_imagemake_init(imagemake);
            if(imk_ret){
                fmsh_print("[NETWORK] initialize imagemake failed, imagemake err code: %u\r\n", imk_ret);
                return ICRAFT_NETWORK_INIT_OP_FAIL;
            }
            break;

        case ns(Attribute_Detpost):
            flatbuffer_Detpost_table_t fb_detpost = ns(Op_attribute(fb_op));
            icraft_detpost_t *detpost = 
                (icraft_detpost_t*)aligned_alloc(CACHE_ALIGN_SIZE, align64_sizeof(icraft_detpost_t));
            if(detpost == NULL){
                icraft_print("malloc memory for detpost with size %d failed\r\n", align64_sizeof(icraft_detpost_t));
                return ICRAFT_NETWORK_MALLOC_FAIL;
            }        
            detpost->basic_info = basic_op_info;
            detpost->detpost_layer_en = flatbuffer_Detpost_detpost_layer_en(fb_detpost);
            detpost->detpost_chn_1st = flatbuffer_Detpost_detpost_chn_1st(fb_detpost);        
            detpost->detpost_chn_2nd = flatbuffer_Detpost_detpost_chn_2nd(fb_detpost);     
            detpost->detpost_chn_3rd = flatbuffer_Detpost_detpost_chn_3rd(fb_detpost); 
            detpost->detpost_anchor_num = flatbuffer_Detpost_detpost_anchor_num(fb_detpost); 
            detpost->detpost_cmp_chn = flatbuffer_Detpost_detpost_cmp_chn(fb_detpost); 
            detpost->detpost_cmp_en = flatbuffer_Detpost_detpost_cmp_en(fb_detpost); 
            detpost->detpost_cmp_mask0 = flatbuffer_Detpost_detpost_cmp_mask0(fb_detpost);
            detpost->detpost_cmp_mask1 = flatbuffer_Detpost_detpost_cmp_mask1(fb_detpost);
            detpost->detpost_valid_chn = flatbuffer_Detpost_detpost_valid_chn(fb_detpost);
            detpost->detpost_reg_base = flatbuffer_Detpost_detpost_reg_base(fb_detpost);
            flatbuffers_int32_vec_t fb_data_thr = flatbuffer_Detpost_detpost_data_thr(fb_detpost);
            uint32_t data_thr_len = flatbuffers_int32_vec_len(fb_data_thr);
            detpost->detpost_data_thr_len = data_thr_len;
            detpost->detpost_data_thr = (int32_t*)fb_data_thr;
            detpost->gic_round = 0;  
            detpost->free = freeDetpost;
            detpost->forward = icraft_detpost_forward;
            detpost->init = icraft_detpost_init;

            icraft_print("detpost_layer_en: %d\r\n", detpost->detpost_layer_en);
            icraft_print("detpost_chn_1st: %d\r\n", detpost->detpost_chn_1st);
            icraft_print("detpost_chn_2nd: %d\r\n", detpost->detpost_chn_2nd);
            icraft_print("detpost_chn_3rd: %d\r\n", detpost->detpost_chn_3rd);
            icraft_print("detpost_anchor_num: %d\r\n", detpost->detpost_anchor_num);
            icraft_print("detpost_cmp_chn: %d\r\n", detpost->detpost_cmp_chn);
            icraft_print("detpost_cmp_en: %d\r\n", detpost->detpost_cmp_en);
            icraft_print("detpost_cmp_mask0: 0x%X\r\n", detpost->detpost_cmp_mask0);
            icraft_print("detpost_cmp_mask1: 0x%X\r\n", detpost->detpost_cmp_mask1);
            icraft_print("detpost_valid_chn: %d\r\n", detpost->detpost_valid_chn);
            icraft_print("detpost_reg_base: 0x%llX\r\n", detpost->detpost_reg_base);
            icraft_print("detpost_data_thr: \r\n");
            for(uint32_t i = 0;i<data_thr_len;++i){
                icraft_print("%u\r\n", detpost->detpost_data_thr[i]);
            } 
            ret = icraft_list_append(&ops_list, detpost);
            if(ret){
                icraft_print("append %s, id=%u, to ops list failed, errno: %u\r\n", 
                    op_type_str, detpost->basic_info->op_id, ret);
                return ret;
            }

            icraft_return detpost_ret;
            detpost_ret = icraft_detpost_init(detpost);;
            if(detpost_ret){
                fmsh_print("[NETWORK] initialize detpost failed, detpost err code: %u\r\n", detpost_ret);
                return ICRAFT_NETWORK_INIT_OP_FAIL;
            }
            break;
       case ns(Attribute_WarpAffine): 
        	
        	 flatbuffer_WarpAffine_table_t fb_warpaffine = ns(Op_attribute(fb_op));
        	 
        	 icraft_warpaffine_t *warpaffine = 
        	                (icraft_warpaffine_t*)aligned_alloc(CACHE_ALIGN_SIZE, align64_sizeof(icraft_warpaffine_t));
        	 flatbuffer_MInversedArray_vec_t  MInversedArray  = flatbuffer_WarpAffine_m_inversed(fb_warpaffine);
        	 warpaffine->basic_info = basic_op_info;
        	 uint32_t  MInversedArray_size = flatbuffer_MInversedArray_vec_len(MInversedArray);
        	 warpaffine->MInversedArray_size = MInversedArray_size;
        	 warpaffine->M_inversed = (float **)aligned_alloc(CACHE_ALIGN_SIZE, align64(MInversedArray_size * sizeof(float *)));
        	 
        	 for (int i = 0; i < MInversedArray_size; ++i) {
        		 flatbuffer_MInversedArray_table_t  m_ivers_table = flatbuffer_MInversedArray_vec_at(MInversedArray, i);
                         flatbuffers_float_vec_t nums_vec = flatbuffer_MInversedArray_nums(m_ivers_table);
        		 uint32_t size = flatbuffers_float_vec_len(nums_vec);
        		 
        		 warpaffine->M_inversed[i] = (float *)aligned_alloc(CACHE_ALIGN_SIZE, align64(size * sizeof(float)));
     
        		 warpaffine->M_inversed[i] = (float *)flatbuffer_MInversedArray_nums(m_ivers_table);;
        	 }

        	 
			if(warpaffine == NULL){
				icraft_print("malloc memory for warpaffine with size %d failed\r\n", align64_sizeof(icraft_warpaffine_t));
				return ICRAFT_NETWORK_MALLOC_FAIL;
			}  
			
			warpaffine->warpaffine_plddr_addr =  flatbuffer_WarpAffine_pl_chunk_addr(fb_warpaffine);
			warpaffine->flags = flatbuffer_WarpAffine_flags(fb_warpaffine);				
			warpaffine->borderMode   = flatbuffer_WarpAffine_border_mode(fb_warpaffine);		
			warpaffine->borderValue  = flatbuffer_WarpAffine_border_value(fb_warpaffine);		
			warpaffine->basic_info = basic_op_info;
			warpaffine->free = freeWarpaffine;    
			warpaffine->forward = icraft_warpaffine_forward;
			warpaffine->init = icraft_warpaffine_init;
			ret = icraft_list_append(&ops_list, warpaffine);
			
			if(ret){
				icraft_print("append %s, id=%u, to ops list failed, errno: %u\r\n", 
					op_type_str, warpaffine->basic_info->op_id, ret);
				return ret;
			}
			
			icraft_return init_ret;
			init_ret =  icraft_warpaffine_init(warpaffine);
			if(init_ret){
				icraft_print("[NETWORK] initialize warpaffine failed, warpaffine err code: %u\r\n", init_ret);
				return ICRAFT_NETWORK_INIT_OP_FAIL;
			}

			break;
                        
          case ns(Attribute_SegPost): {
        	
        	flatbuffer_SegPost_table_t fb_segpost = ns(Op_attribute(fb_op));
        	       	 
        	icraft_segpost_t *segpost = (icraft_segpost_t*)aligned_alloc(CACHE_ALIGN_SIZE, align64_sizeof(icraft_segpost_t));
        	
        	segpost->mode = flatbuffer_SegPost_mode(fb_segpost);
		segpost->src_ysize = flatbuffer_SegPost_src_ysize(fb_segpost);
		segpost->src_xsize = flatbuffer_SegPost_src_xsize(fb_segpost);
		segpost->dst_ysize = flatbuffer_SegPost_dst_ysize(fb_segpost);
		segpost->dst_xsize = flatbuffer_SegPost_dst_xsize(fb_segpost);
                segpost->reg_base = flatbuffer_SegPost_reg_base(fb_segpost);
		segpost->align_corner = flatbuffer_SegPost_align_corner(fb_segpost);
 		segpost->basic_info = basic_op_info;	
 		segpost->free = freeSegpost;    
 		segpost->forward = icraft_segpost_forward;
 		segpost->init = icraft_segpost_init;
                
                icraft_print("segpost_mode: %d\r\n", segpost->mode);
                icraft_print("segpost_src_ysize: %d\r\n", segpost->src_ysize);
                icraft_print("segpost_src_xsize: %d\r\n", segpost->src_xsize);
                icraft_print("segpost_dst_ysize: %d\r\n", segpost->dst_ysize);
                icraft_print("segpost_dst_xsize: %d\r\n", segpost->dst_xsize);
                icraft_print("segpost_reg_base: 0x%llX\r\n", segpost->reg_base);
                icraft_print("segpost_align_corner: %d\r\n", segpost->align_corner);
 		
 		ret = icraft_list_append(&ops_list, segpost);
 		if(ret){
 			icraft_print("append %s, id=%u, to ops list failed, errno: %u\r\n", 
 				op_type_str, segpost->basic_info->op_id, ret);
 			return ret;
 		}
 		
 		icraft_return init_ret;
		init_ret =  icraft_segpost_init(segpost);
		if(init_ret){
			icraft_print("[NETWORK] initialize segpost failed, segpost err code: %u\r\n", init_ret);
			return ICRAFT_NETWORK_INIT_OP_FAIL;
		}
   
        	break;
        }


		
        case ns(Attribute_Input):
            flatbuffer_Input_table_t fb_input = ns(Op_attribute(fb_op));
            icraft_input_t *input = 
                (icraft_input_t*)aligned_alloc(CACHE_ALIGN_SIZE, align64_sizeof(icraft_input_t));
            if(input == NULL){
                fmsh_print("malloc memory for input with size %d failed\r\n", align64_sizeof(icraft_input_t));
                return ICRAFT_NETWORK_MALLOC_FAIL;
            }        
            input->basic_info = basic_op_info;
            input->free = freeInput;
            input->forward = icraft_input_forward;
            input->init = icraft_input_init;
            ret = icraft_list_append(&ops_list, input);
            if(ret){
                fmsh_print("append %s, id=%u, to ops list failed, errno: %u\r\n", 
                    op_type_str, input->basic_info->op_id, ret);
                return ret;
            }
            (*network)->ifms_num = input->basic_info->ofms_num;
            (*network)->ifms = input->basic_info->ofms;
            break;

        case ns(Attribute_Output):
            flatbuffer_Output_table_t fb_output = ns(Op_attribute(fb_op));
            icraft_output_t *output = 
                (icraft_output_t*)aligned_alloc(CACHE_ALIGN_SIZE, align64_sizeof(icraft_output_t));
            if(output == NULL){
                fmsh_print("malloc memory for output with size %d failed\r\n", align64_sizeof(icraft_output_t));
                return ICRAFT_NETWORK_MALLOC_FAIL;
            }        
            output->basic_info = basic_op_info;
            output->free = freeOutput;    
            output->forward = icraft_output_forward;
            output->init = icraft_output_init;
            ret = icraft_list_append(&ops_list, output);
            if(ret){
                icraft_print("append %s, id=%u, to ops list failed, errno: %u\r\n", 
                    op_type_str, output->basic_info->op_id, ret);
                return ret;
            }
            (*network)->ofms_num = output->basic_info->ifms_num;
            (*network)->ofms = output->basic_info->ifms;
            break;

        default:
            
            icraft_hostop_t *hostop = 
                (icraft_hostop_t*)aligned_alloc(CACHE_ALIGN_SIZE, align64_sizeof(icraft_hostop_t));
            if(hostop == NULL){
                icraft_print("malloc memory for cast with size %d failed\r\n", align64_sizeof(icraft_hostop_t));
                return ICRAFT_NETWORK_MALLOC_FAIL;
            }
            hostop->basic_info = basic_op_info;
            hostop->free = freeHostop;
            hostop->forward = icraft_hostop_forward;
            hostop->init = icraft_hostop_init;
            icraft_list_append(&ops_list, hostop);
            icraft_print("[WARNING] current_op:%d is Hostop, op_type:%s\r\n", basic_op_info->op_id, op_type_str);
        }
    }

    (*network)->ops = ops_list;


#if ICRAFT_DEBUG
    icraft_print(">>>>>>>>> name %s\r\n", (*network)->name);
    icraft_print(">>>>>>>>> mmu mode %d\r\n", (*network)->mmu_mode);
    icraft_print(">>>>>>>>> mpe rows %d, mpe cols %d\r\n", (*network)->mpe_rows, (*network)->mpe_cols);
    icraft_print(">>>>>>>>> params_segment_logic_addr: %d\r\n", segments->params_segment_logic_addr);
    icraft_print(">>>>>>>>> params_segment_phy_addr: %d\r\n", segments->params_segment_phy_addr);
    icraft_print(">>>>>>>>> params_segment_bytesize: %d\r\n", segments->params_segment_bytesize);
    icraft_print(">>>>>>>>> instr_segment_logic_addr: %d\r\n", segments->instr_segment_logic_addr);
    icraft_print(">>>>>>>>> instr_segment_phy_addr: %d\r\n", segments->instr_segment_phy_addr);
    icraft_print(">>>>>>>>> instr_segment_bytesize: %d\r\n", segments->instr_segment_bytesize);    
    icraft_print(">>>>>>>>> input_segment_logic_addr: %d\r\n", segments->input_segment_logic_addr);
    icraft_print(">>>>>>>>> input_segment_phy_addr: %d\r\n", segments->input_segment_phy_addr);
    icraft_print(">>>>>>>>> input_segment_bytesize: %d\r\n", segments->input_segment_bytesize);
    icraft_print(">>>>>>>>> output_segment_logic_addr: %d\r\n", segments->output_segment_logic_addr);
    icraft_print(">>>>>>>>> output_segment_phy_addr: %d\r\n", segments->output_segment_phy_addr);
    icraft_print(">>>>>>>>> output_segment_bytesize: %d\r\n", segments->output_segment_bytesize);
    icraft_print(">>>>>>>>> ftmp_segment_logic_addr: %d\r\n", segments->ftmp_segment_logic_addr);
    icraft_print(">>>>>>>>> ftmp_segment_phy_addr: %d\r\n", segments->ftmp_segment_phy_addr);
    icraft_print(">>>>>>>>> ftmp_segment_bytesize: %d\r\n", segments->ftmp_segment_bytesize);
    icraft_print(">>>>>>>>> ops num: %d\r\n", ops_num);
    icraft_print(">>>>>>>>> ifms num: %u\r\n", (*network)->ifms_num);
    icraft_print(">>>>>>>>> ofms num: %u\r\n", (*network)->ofms_num);
    icraft_print(">>>>>>>>> ifms: \r\n");
    for(uint32_t i = 0; i < (*network)->ifms_num; ++i){
        icraft_print("> %u\r\n", (*network)->ifms[i]);
        void *ftmp_buf;
        icraft_hashtable_search((*network)->ftmps, &(*network)->ifms[i], &ftmp_buf);
        icraft_ftmp_info_t *ifm = (icraft_ftmp_info_t*)ftmp_buf;
    }
    icraft_print(">>>>>>>>> ofms: \r\n");
    for(uint32_t i = 0; i < (*network)->ofms_num; ++i){
        icraft_print("> %u\r\n", (*network)->ofms[i]);
        void *ftmp_buf;
        icraft_hashtable_search((*network)->ftmps, &(*network)->ofms[i], &ftmp_buf);
        icraft_ftmp_info_t *ofm = (icraft_ftmp_info_t*)ftmp_buf;
    }
#endif

    // open mmu
    if(((*network)->mmu_mode == ICRAFT_DEVICE_MMU_OPEN))
    {
        icraft_device_mmu_table_t* mmu_table = (icraft_device_mmu_table_t*)aligned_alloc(CACHE_ALIGN_SIZE, align64_sizeof(icraft_device_mmu_table_t));
        (*network)->mmu_table = mmu_table;
        _icraft_network_append_mmu_table(*network);
        if(g_icraft_device_mmu_isopen == ICRAFT_FALSE){
            icraft_print("[NETWORK] mmu has been closed, open now...\r\n");
            ret = icraft_device_mmu_open();
            if(ret){
                icraft_print("[NETWORK] open mmu failed, errno: %u\r\n", ret);
                return ICRAFT_NETWORK_OPEN_MMU_FAIL;
            }
        }
    }

#if ICRAFT_LOG_TIME
    uint64_t unpack_time_end = get_current_time();
    float unpack_time = icraft_timer_convert_to_ms(unpack_time_end, unpack_time_begin);
#endif

#if ICRAFT_LOG_TIME
    uint64_t deploy_time_begin = get_current_time();
#endif
    // upload instr and param to plddr
    ret = _icraft_network_deploy(*network);
    if(ret){
        icraft_print("upload network(name=%s) failed, errno: %u\r\n", 
            (*network)->name, ret);
        return ICRAFT_NETWORK_UPLOAD_FAIL;
    }
#if ICRAFT_LOG_TIME
    uint64_t deploy_time_end = get_current_time();
    float deploy_time = icraft_timer_convert_to_ms(deploy_time_end, deploy_time_begin);
#endif

#if ICRAFT_LOG_TIME
    (*network)->time_log = (icraft_network_time_log_t*)aligned_alloc(CACHE_ALIGN_SIZE, align64_sizeof(icraft_network_time_log_t));
    if((*network)->time_log == NULL){
        icraft_print("[NETWORK] malloc for time log failed, malloc size: %u\r\n", align64_sizeof(icraft_network_time_log_t));
        return ICRAFT_NETWORK_MALLOC_FAIL;
    }
    (*network)->time_log->load_time = load_time;
    (*network)->time_log->unpack_time = unpack_time;
    (*network)->time_log->deploy_time = deploy_time;
    (*network)->time_log->hw_time = 0.0;

    uint64_t interface_time_end = get_current_time();
    float interface_time = icraft_timer_convert_to_ms(interface_time_end, interface_time_begin);
    (*network)->time_log->interface_create_network_time = interface_time;
#endif
    return ICRAFT_SUCCESS;
}

icraft_return
_icraft_network_add_ifms_ofms_to_op(icraft_network_info_t *network , icraft_basic_op_info_t **op)
{
    if(network == NULL){
        fmsh_print("network is null\r\n");
        return ICRAFT_NETWORK_NULL;
    }
    if((*op) == NULL){
        fmsh_print("op is null\r\n");
        return ICRAFT_NETWORK_OP_NULL;
    }
    icraft_return hash_ret;
    uint32_t ifms_num = (*op)->ifms_num;
    uint32_t ofms_num = (*op)->ofms_num;
    (*op)->ifms_ptr = (icraft_ftmp_info_t**)aligned_alloc(CACHE_ALIGN_SIZE, align64(sizeof(icraft_ftmp_info_t*) * ifms_num));
    if((*op)->ifms_ptr == NULL){
        fmsh_print("malloc memory for op(id=%u) ifms ptr failed\r\n", (*op)->op_id);
        return ICRAFT_NETWORK_MALLOC_FAIL;
    }
    (*op)->ofms_ptr = (icraft_ftmp_info_t**)aligned_alloc(CACHE_ALIGN_SIZE, align64(sizeof(icraft_ftmp_info_t*) * ofms_num));
    if((*op)->ofms_ptr == NULL){
        fmsh_print("malloc memory for op(id=%u) ofms ptr failed\r\n", (*op)->op_id);
        return ICRAFT_NETWORK_MALLOC_FAIL;
    }


    for(uint32_t i = 0; i < ifms_num; ++i){
        void *ftmp_buf;
        hash_ret = icraft_hashtable_search(network->ftmps, &((*op)->ifms[i]), &ftmp_buf);
        if(hash_ret){
            fmsh_print("search ifm(vid=%u) from ftmps failed, hash err code:%u\r\n", 
                (*op)->ifms[i], hash_ret);
            return ICRAFT_NETWORK_GET_FTMP_FAIL;
        }
        icraft_ftmp_info_t *ifm = (icraft_ftmp_info_t*)ftmp_buf;
        (*op)->ifms_ptr[i] = ifm;
    }
    for(uint32_t i = 0; i < ofms_num; ++i){
        void *ftmp_buf;
        hash_ret = icraft_hashtable_search(network->ftmps, &((*op)->ofms[i]), &ftmp_buf);
        if(hash_ret){
            fmsh_print("search ofm(vid=%u) from ftmps failed, hash err code:%u\r\n", 
                (*op)->ofms[i], hash_ret);
            return ICRAFT_NETWORK_GET_FTMP_FAIL;
        }
        icraft_ftmp_info_t *ofm = (icraft_ftmp_info_t*)ftmp_buf;
        (*op)->ofms_ptr[i] = ofm;
    }    
    return ICRAFT_SUCCESS;
}


icraft_return 
_icraft_network_append_mmu_table(icraft_network_info_t *network){

    temp_seg segs[5];

    segs[0].byte_size = network->segments->params_segment_bytesize;
    segs[1].byte_size = network->segments->instr_segment_bytesize;
    segs[2].byte_size = network->segments->input_segment_bytesize;
    segs[3].byte_size = network->segments->output_segment_bytesize;
    segs[4].byte_size = network->segments->ftmp_segment_bytesize;

    segs[0].logic_addr = network->segments->params_segment_logic_addr;
    segs[1].logic_addr = network->segments->instr_segment_logic_addr;
    segs[2].logic_addr = network->segments->input_segment_logic_addr;
    segs[3].logic_addr = network->segments->output_segment_logic_addr;
    segs[4].logic_addr = network->segments->ftmp_segment_logic_addr;

    segs[0].phy_addr = network->segments->params_segment_phy_addr;
    segs[1].phy_addr = network->segments->instr_segment_phy_addr;
    segs[2].phy_addr = network->segments->input_segment_phy_addr;
    segs[3].phy_addr = network->segments->output_segment_phy_addr;
    segs[4].phy_addr = network->segments->ftmp_segment_phy_addr;
    
    
    qsort(segs, 5, sizeof(temp_seg), cmp);

    for (int i = 0; i < MMU_SIZE; i++) {
        if (i == 0) {
            network->mmu_table->logic_bases[i] = 0;
            network->mmu_table->phy_bases[i] = 0;
        }
        else if (i < 6) {
            if (segs[i-1].byte_size == 0) {
                continue;
            }
            network->mmu_table->logic_bases[i] = segs[i-1].logic_addr;
            network->mmu_table->phy_bases[i] = segs[i-1].phy_addr;
        }
        else {
            network->mmu_table->logic_bases[i] = UINT32_MAX;
            network->mmu_table->phy_bases[i] = UINT32_MAX;
        }
    }

#if ICRAFT_DEBUG
    icraft_print("-------logic bases    |    phy bases-------\r\n");
    for(uint32_t i = 0; i < MMU_SIZE; ++i){
        icraft_print("       0x%lX    |    0x%lX       \r\n", 
            network->mmu_table->logic_bases[i],
            network->mmu_table->phy_bases[i]
        );
    }
    icraft_print("--------------------------------------------\r\n");
#endif
    return ICRAFT_SUCCESS;
}

icraft_return
_icraft_network_get_ifms_ofms_for_op(icraft_network_info_t *network,
	icraft_basic_op_info_t *op,
	icraft_ftmp_info_t **ifms,
	icraft_ftmp_info_t **ofms)
{
    icraft_return hash_ret;
    uint32_t ifms_num = op->ifms_num;
    uint32_t ofms_num = op->ofms_num;
    for(uint32_t i = 0; i<ifms_num;++i){
        void *ftmp_buf;
        hash_ret = icraft_hashtable_search(network->ftmps, &(op->ifms[i]), &ftmp_buf);
        if(hash_ret){
            fmsh_print("search ifm(vid=%u) from ftmps failed, hash err code:%u\r\n", 
                op->ifms[i], hash_ret);
            return ICRAFT_NETWORK_GET_FTMP_FAIL;
        }
        icraft_ftmp_info_t *ifm = (icraft_ftmp_info_t*)ftmp_buf;
        ifms[i] = ifm;
    }
    for(uint32_t i = 0; i<ofms_num;++i){
        void *ftmp_buf;
        hash_ret = icraft_hashtable_search(network->ftmps, &(op->ofms[i]), &ftmp_buf);
        if(hash_ret){
            fmsh_print("search ofm(vid=%u) from ftmps failed, hash err code:%u\r\n", 
                op->ifms[i], hash_ret);
            return ICRAFT_NETWORK_GET_FTMP_FAIL;
        }
        icraft_ftmp_info_t *ofm = (icraft_ftmp_info_t*)ftmp_buf;
        ofms[i] = ofm;
    }    
    return ICRAFT_SUCCESS;

}

/*
icraft_return 
icraft_network_forward(icraft_network_info_t *network)
{
#if ICRAFT_LOG_TIME
    global_timer_enable();
    uint64_t interface_time_begin = get_current_time();
#endif

    if(network == NULL){
        fmsh_print("network is null\r\n");
        return ICRAFT_NETWORK_NOT_INITIAL;
    }
    icraft_return op_ret;
    icraft_return net_ret;
    uint32_t ops_num = network->ops_num;
    icraft_list_node_t *current_op = network->ops;
    if(current_op == NULL){
        fmsh_print("network has no ops\r\n");
        return ICRAFT_NETWORK_NO_OPS;
    }
    uint32_t op_idx = 0;
    icraft_return ftmp_ret;
    network->current_op = -1;

    while(current_op != NULL){
        //icraft_basic_op_info_t *basic_op = (icraft_basic_op_info_t*)(current_op->data);
        uint32_t op_type = network->ops_type[op_idx];

        icraft_basic_op_t* basic_op = (icraft_basic_op_t*)current_op->data;
        op_ret = basic_op->forward(basic_op);
        if(op_ret){
            fmsh_print("[FORWARD] network forward failed at op(opid=%u), err code: %u\r\n", 
                basic_op->basic_info->op_id, op_ret);
            return ICRAFT_NETWORK_FORWARD_FAIL;
        }
#if ICRAFT_DEBUG
        icraft_print("[FORWARD] run op(opid=%u) success\r\n", basic_op->basic_info->op_id);
#endif

#if DUMP_AFTER_EACH_OP
        for(uint32_t i = 0; i < basic_op->basic_info->ofms_num; ++i){
            ftmp_ret = icraft_ftmp_dump(basic_op->basic_info->ofms_ptr[i], network->name);
            if(ftmp_ret){
                icraft_print("[FORWARD] dump op(opid=%u) ofm(vid=%u) failed, ftmp err code: %u\r\n", 
                    basic_op->basic_info->op_id, basic_op->basic_info->ofms_ptr[i]->vid, ftmp_ret);
                return ICRAFT_NETWORK_DUMP_FAIL;
            }
            fmsh_print("[FORWARD] dump ofms for op(opid=%u) success!\r\n", basic_op->basic_info->op_id);
        }
#endif

        current_op = current_op->next;
        op_idx++;
    }
    if(op_idx != network->ops_num){
        fmsh_print("[NETWORK] forward ops num (%u) dismatch network ops num (%u)\r\n", op_idx, network->ops_num);
        return ICRAFT_NETWORK_FORWARD_OPS_NUM_DISMATCH;
    }

#if ICRAFT_LOG_TIME
    uint64_t interface_time_end = get_current_time();
    float interface_time = icraft_timer_convert_to_ms(interface_time_end, interface_time_begin);
    network->time_log->inferface_forward_time = interface_time;
#endif

    return ICRAFT_SUCCESS;
}
*/


icraft_return
_icraft_network_deploy(icraft_network_info_t const *network)
{
    icraft_return dev_ret;
    uint32_t ops_num = network->ops_num;
    icraft_list_node_t *current_op = network->ops;
    while(current_op != NULL){
        icraft_basic_op_t* basic_op = (icraft_basic_op_t*)current_op->data;
        icraft_op_t op_type = basic_op->basic_info->op_type;
        switch (op_type)
        {
        case ICRAFT_OP_HARDOP:
            icraft_hardop_t *hardop = (icraft_hardop_t*)(current_op->data);
            icraft_print("[DEPLOY STAGE] uploading instruction(0x%llX) of HARDOP(opid=%u), to plddr addr:%u(0x%llX), size:%u\r\n", 
                hardop->instr_data, hardop->basic_info->op_id, hardop->instr_phy_addr, hardop->instr_phy_addr, hardop->instr_size);
            if(hardop->instr_size != 0){
                //dev_ret = icraft_device_write_plmem(4096, hardop->instr_data, 256);
                //dev_ret = icraft_device_write_plmem(hardop->instr_phy_addr, hardop->instr_data, 128);
                dev_ret = icraft_device_write_plmem(hardop->instr_phy_addr, hardop->instr_data, hardop->instr_size);
                if(dev_ret){
                    fmsh_print("[DEPLOY STAGE] upload op(id=%u) instr, addr=%u, size=%u failed, device err code=%u\r\n", 
                        hardop->basic_info->op_id, hardop->instr_phy_addr, hardop->instr_size, dev_ret);
                    return ICRAFT_NETWORK_UPLOAD_FAIL;
                }
#if ICRAFT_DEBUG
                icraft_print("[DEPLOY STAGE] checking whether uploaded instructions correct...\r\n");

                void *check_instr_buf = (void*)aligned_alloc(CACHE_ALIGN_SIZE, align64(hardop->instr_size));
                if(check_instr_buf == NULL){
                    icraft_print("[DEPLOY STAGE] malloc check buffer for instructions failed\r\n");
                    return ICRAFT_NETWORK_DEPLOY_CHECK_FAIL;
                }
                //icraft_print_mem(check_instr_buf, 64);
                //dev_ret = icraft_device_read_plmem(check_instr_buf, 4096, 256);                
                //dev_ret = icraft_device_read_plmem(check_instr_buf, hardop->instr_phy_addr, 128);
                dev_ret = icraft_device_read_plmem(check_instr_buf, hardop->instr_phy_addr, hardop->instr_size);
                if(dev_ret){
                    icraft_print("[DEPLOY STAGE] read instructions from plddr failed, addr=%u, size=%u\r\n", 
                        hardop->instr_phy_addr, hardop->instr_size);
                    free(check_instr_buf);
                    return ICRAFT_NETWORK_DEPLOY_CHECK_FAIL;
                }
                // icraft_print_mem(hardop->instr_data, 256);
                // icraft_print_mem(check_instr_buf, 256);
                if(memcmp(check_instr_buf, hardop->instr_data, hardop->instr_size) != 0){
                    icraft_print("[DEPLOY STAGE] check instructions(0x%llX) and dma data(0x%llX) fail, data inconsistent\r\n", 
                        hardop->instr_data, check_instr_buf);
                    free(check_instr_buf);
                    return ICRAFT_NETWORK_DEPLOY_CHECK_FAIL;
                }
                free(check_instr_buf);
                icraft_print("[DEPLOY STAGE] check instructions success!\r\n");
#endif
                icraft_print("[DEPLOY STAGE] upload instructions of HARDOP(opid=%u) success\r\n", hardop->basic_info->op_id);
            }

            icraft_print("[DEPLOY STAGE] uploading params(0x%llX) of HARDOP(opid=%u), to plddr addr:%u(0x%llX), size:%u\r\n", 
                hardop->param_data, hardop->basic_info->op_id, hardop->param_addr, hardop->param_addr, hardop->param_size);
            if(hardop->param_size != 0){
                dev_ret = icraft_device_write_plmem(hardop->param_addr, hardop->param_data, hardop->param_size);
                if(dev_ret){
                    icraft_print("[DEPLOY STAGE] upload op(id=%u) param, addr=%u, size=%u failed, device err code=%u\r\n", 
                        hardop->basic_info->op_id, hardop->param_addr, hardop->param_size, dev_ret);
                    return ICRAFT_NETWORK_UPLOAD_FAIL;
                }
#if ICRAFT_DEBUG
                icraft_print("[DEPLOY STAGE] checking whether uploaded params correct...\r\n");
                void *check_params_buf = aligned_alloc(CACHE_ALIGN_SIZE, align64(hardop->param_size));
                if(check_params_buf == NULL){
                    icraft_print("[DEPLOY STAGE] malloc check buffer for params failed\r\n");
                    return ICRAFT_NETWORK_DEPLOY_CHECK_FAIL;
                }
                dev_ret = icraft_device_read_plmem(check_params_buf, hardop->param_addr, hardop->param_size);
                if(dev_ret){
                    icraft_print("[DEPLOY STAGE] read params from plddr failed, addr=%u, size=%u\r\n", hardop->param_addr, hardop->param_size);
                    free(check_params_buf);
                    return ICRAFT_NETWORK_DEPLOY_CHECK_FAIL;
                }
                if(memcmp(check_params_buf, hardop->param_data, hardop->param_size) != 0){
                    icraft_print("[DEPLOY STAGE] check params(0x%llX) and dma data(0x%llX) fail, data inconsistent\r\n", 
                        hardop->param_data, check_params_buf);
                    free(check_params_buf);
                    return ICRAFT_NETWORK_DEPLOY_CHECK_FAIL;
                }
                free(check_params_buf);
                icraft_print("[DEPLOY STAGE] check params success!\r\n");
#endif
                icraft_print("[DEPLOY STAGE] upload params of HARDOP(opid=%u) success\r\n", hardop->basic_info->op_id);
            }
            break;
        default:
            break;
        }
        current_op = current_op->next;
    }

    return ICRAFT_SUCCESS;
}


icraft_return
icraft_network_fill_host_inputs_with_buffers(
    icraft_network_info_t *const network, 
    void **buffers, 
    uint32_t const buffers_num, 
    uint32_t const * const ifm_idx_list)
{
    icraft_return ht_ret;
    if(buffers_num > network->ifms_num){
        icraft_print("buffers number (%u) is out of bounds, while network inputs number=%u\r\n", 
            buffers_num, network->ifms_num);
        return ICRAFT_NETWORK_BUFFER_NUM_OUT_OF_BOUNDS;
    }
    for(uint32_t i = 0; i < buffers_num; ++i){
        if(ifm_idx_list[i] > network->ifms_num){
            icraft_print("idx=%u is out of bounds, where network inputs number=%u\r\n", 
                ifm_idx_list[i], network->ifms_num);
            return ICRAFT_NETWORK_INPUT_IDX_OUT_OF_BOUNDS;
        }
    }
    for(uint32_t i = 0; i < buffers_num; ++i){
        uint32_t ifm_idx = ifm_idx_list[i];
    
        icraft_print("creating input (idx=%u) from buffer...\r\n", ifm_idx);
        if(buffers[i] == NULL){
            icraft_print("buffers[%u] is NULL\r\n", i);
            return ICRAFT_NETWORK_BUFFER_NULL;
        }
        void *ifm_buf;
        ht_ret = icraft_hashtable_search(network->ftmps, &(network->ifms[ifm_idx]), &ifm_buf);
        if(ht_ret){
            icraft_print("fetch input ftmp from ftmp list failed, hash table err code: %u\r\n", ht_ret);
            return ICRAFT_NETWORK_GET_FTMP_FAIL;
        }
        icraft_ftmp_info_t* ifm = (icraft_ftmp_info_t*)ifm_buf;
        if(ifm->mtype == ICRAFT_MTYPE_ETM){
            icraft_print("input(idx=%u, vid=%u)'s mtype is not HOST, but mtype=%u, use 'icraft_network_create_inputs_from_plddr' instead\r\n",
                ifm_idx, ifm->vid, ifm->mtype);
            return ICRAFT_NETWORK_INVALID_MYTPE;
        }
        if(ifm->mtype == ICRAFT_MTYPE_OCM){
            icraft_print("input(idx=%u, vid=%u)'s mtype is not HOST, but OCM\r\n",
                ifm_idx, ifm->vid);
            return ICRAFT_NETWORK_INVALID_MYTPE;        
        }        
        ifm->data = buffers[i];
        icraft_print("create input (idx=%u, vid=%u) from buffer success.\r\n", ifm_idx, ifm->vid);
    }
    return ICRAFT_SUCCESS;
}

icraft_return
icraft_network_fill_etm_input_with_host_buffer(
	icraft_network_info_t *network, 
    void *buffer,
	uint32_t const size,
	uint32_t const ifm_idx)
{
    if(network == NULL){
        icraft_print("network is null\r\n");
        return ICRAFT_NETWORK_NOT_INITIAL;
    }
    if(buffer == NULL){
        icraft_print("buffer is null\r\n");
        return ICRAFT_NETWORK_BUFFER_NULL;
    }
    // if((addr < MIN_PLDDR_ADDR) || (addr >= MAX_PLDDR_ADDR)){
    //     icraft_print("plddr addr(%u) is out of bound, which should range from [%u, %u]\r\n", 
    //         addr, MIN_PLDDR_ADDR, MAX_PLDDR_ADDR);
    //     return ICRAFT_NETWORK_ADDR_OUT_OF_BOUND;
    // }
    if(size >= MAX_PLDDR_DATA_SIZE){
        icraft_print("plddr data size(%u) is out of bound, which should be less than %u\r\n",
            size, MAX_PLDDR_DATA_SIZE);
        return ICRAFT_NETWORK_DATA_SIZE_OUT_OF_BOUND;
    }
    if(ifm_idx > network->ifms_num){
        icraft_print("idx=%u is out of bounds, where network inputs number=%u\r\n", 
            ifm_idx, network->ifms_num);
        return ICRAFT_NETWORK_INPUT_IDX_OUT_OF_BOUNDS;
    }
    icraft_return ht_ret;
    icraft_return dev_ret;
    void *ifm_buf;
    ht_ret = icraft_hashtable_search(network->ftmps, &(network->ifms[ifm_idx]), &ifm_buf);
    if(ht_ret){
        icraft_print("fetch input ftmp from ftmp list failed, hash table err code: %u\r\n", ht_ret);
        return ICRAFT_NETWORK_GET_FTMP_FAIL;
    }
    icraft_ftmp_info_t* ifm = (icraft_ftmp_info_t*)ifm_buf;    
    if(ifm->mtype == ICRAFT_MTYPE_HOST){
        icraft_print("input(idx=%u, vid=%u)'s mtype is not ETM, but mtype=%u, use 'icraft_network_fill_host_input_with_buffer' instead\r\n",
            ifm_idx, ifm->vid, ifm->mtype);
        return ICRAFT_NETWORK_INVALID_MYTPE;
    }
    if(ifm->mtype == ICRAFT_MTYPE_OCM){
        icraft_print("input(idx=%u, vid=%u)'s mtype is not ETM, but OCM\r\n",
            ifm_idx, ifm->vid);
        return ICRAFT_NETWORK_INVALID_MYTPE;        
    }
    if(size != ifm->size){
        icraft_print("buffer size(%u) for input(idx=%u, vid=%u) is inconsistent with ifm size(%u)\r\n", 
            size, ifm_idx, ifm->vid, ifm->size);
        return ICRAFT_NETWORK_BUFFER_SIZE_INCONSISTENT;
    }
    dev_ret = icraft_device_write_plmem(ifm->addr, buffer, size);
    if(dev_ret){
        icraft_print("write plddr data(addr=%u, size=%u) for input(idx=%u, vid=%u) failed, device err code: %u\r\n",
            ifm->addr, ifm->size, ifm_idx, ifm->vid, dev_ret);
        return ICRAFT_NETWORK_FILL_PLDDR_INPUT_FAIL;
    }
    return ICRAFT_SUCCESS;
}

icraft_return
icraft_network_log_time(icraft_network_info_t *network){
#if ICRAFT_LOG_TIME
    icraft_list_node_t *current_op = network->ops;
    fmsh_print("===========Operations Time Log============\r\n");
    while(current_op != NULL){
        icraft_basic_op_t *op = (icraft_basic_op_t*)(current_op->data);
        char type[10];
        icraft_get_op_type(op, type);
        fmsh_print("interface time(forward): %.4fms, memcpy time: %.4fms, hard time: %.4fms, opid: %u, type: %s\r\n",
                op->basic_info->time_log->interface_time, 
                op->basic_info->time_log->memcpy_time, 
                op->basic_info->time_log->hw_time,
                op->basic_info->op_id, 
                type
                );
        current_op = current_op->next;
    }
    fmsh_print("===========Network Time Log============\r\n");
    fmsh_print("load time: %.4fms, unpack time: %.4fms, interface time(create network): %.4fms, interface time(forward): %.4fms, hardware_time: %.4f.ms\r\n",
            network->time_log->load_time, 
            network->time_log->unpack_time, 
            network->time_log->interface_create_network_time, 
            network->time_log->inferface_forward_time,
            network->time_log->hw_time);
#endif
    return ICRAFT_SUCCESS;
}

icraft_list_node_t* 
icraft_network_getOp(icraft_network_info_t* network, uint32_t op_id) {
    icraft_return dev_ret;
    uint32_t ops_num = network->ops_num;
    icraft_list_node_t *current_op = network->ops;
    uint32_t op_idx = 0;
    while(current_op != NULL){
        icraft_basic_op_t *operation = (icraft_basic_op_t*)(current_op->data);
        if (operation->basic_info->op_id == op_id) {
            return current_op;
        }
        current_op = current_op->next;
    }
    fmsh_print("[NETWORK_GETOP ERROR] can not find op_id:%u \r\n", op_id);
}

icraft_return icraft_network_free(icraft_network_info_t *self) {
    
    
    icraft_list_node_t* current_op = self->ops;
    while (current_op != NULL) {
        icraft_basic_op_t *operation = (icraft_basic_op_t*)(current_op->data);
        operation->free(operation);
        free(operation);
        current_op = current_op->next;
    }
    icraft_list_destroy(self->ops);
    free(self->ops);
    
    
    icraft_hashtable_t* table = self->ftmps;
    
    for (uint32_t i = 0; i < table->capacity; ++i) {
        icraft_hashnode_t *node = table->buckets[i];
        while (node) {
            icraft_hashnode_t *temp = node;
            node = node->next;
            icraft_ftmp_info_t* ftmp = (icraft_ftmp_info_t*)(temp->value);
            _freeFtmp(ftmp);
            free(ftmp);
        }
    }
    
    icraft_hashtable_destroy(self->ftmps);
    free(self->ftmps);
    
    
    free(self->segments);

#if ICRAFT_LOG_TIME
    free(self->time_log);
#endif

    if (self->mmu_mode == ICRAFT_DEVICE_MMU_OPEN) {
        free(self->mmu_table->logic_bases);
        free(self->mmu_table->phy_bases);
        free(self->mmu_table);
    }

    free(self->buffer);
}