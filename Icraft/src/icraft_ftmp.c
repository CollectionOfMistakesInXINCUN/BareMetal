#include "icraft_ftmp.h"

icraft_return
icraft_ftmp_sync(const uint32_t sync_idx)
{
    icraft_return ret;
    uint32_t layer_count;
    global_timer_enable();
    double counts = TIMEOUT_THRES * 1000 * GTC_CLK_FREQ;
    uint64_t begin = get_current_time();
    while((get_current_time() - begin) < counts){
        ret = icraft_device_get_layercount(&layer_count);
        if(ret){
            fmsh_print("[FTMP_SYNC] get layer count failed, err code: %u\r\n", ret);
            return ICRAFT_FTMP_GET_LAYERCOUNT_FAIL;
        }
        if(layer_count >= sync_idx){
            icraft_print("[FTMP_SYNC] sync success, layercount: %u, sync idx: %u\r\n", layer_count, sync_idx);
            return ICRAFT_SUCCESS;
        }
    }
    fmsh_print("[FTMP_SYNC] sync failed, layer count=%u, sync index=%u\r\n", layer_count, sync_idx);
    return ICRAFT_FTMP_SYNC_FAIL;
}

icraft_return
icraft_ftmp_dump(icraft_ftmp_info_t const * const ftmp, const char *folder_name)
{
    if(ftmp == NULL){
        fmsh_print("[FTMP_DUMP] ftmp to dump is NULL\r\n");
        return ICRAFT_FTMP_NULL;
    }
    
    icraft_return dev_ret;
    icraft_return ff_ret;

    char dump_path[512] = "/dump";
    char suffix[] = ".ftmp";

    ff_ret = icraft_ff_mkdir(dump_path);
    if(ff_ret){
        fmsh_print("[FTMP_DUMP] create dump dir(%s) fail, icraft ff err code:%u\r\n", dump_path, ff_ret);
        return ICRAFT_FTMP_CREATE_DUMP_DIR_FAIL;
    }

    icraft_mtype_t mtype = ftmp->mtype;
    int ret;
    uint32_t cur_len;
    uint32_t remain_len;
    uint32_t written_size;

    cur_len = strlen(dump_path);
    remain_len = sizeof(dump_path) - cur_len;
    ret = snprintf(dump_path + cur_len, remain_len, "/%s", folder_name);
    if(ret < 0){
        fmsh_print("[FTMP_DUMP] encode error when concat file path for dump ftmp\r\n");
        return ICRAFT_FTMP_GENERATE_PATH_FAIL;
    }
    if(ret >= sizeof(dump_path)){
        fmsh_print("[FTMP_DUMP] dump path has been truncated, needed buffer size is %u\r\n", ret+1);
        return ICRAFT_FTMP_GENERATE_PATH_FAIL;
    }

    ff_ret = icraft_ff_mkdir(dump_path);
    if(ff_ret){
        fmsh_print("[FTMP_DUMP] create dump dir(%s) fail, icraft ff err code:%u\r\n", dump_path, ff_ret);
        return ICRAFT_FTMP_CREATE_DUMP_DIR_FAIL;
    }

    switch (mtype)
    {
    case ICRAFT_MTYPE_ETM:
        icraft_print("[FTMP_DUMP] dumping ftmp(vid=%u, ETM)...\r\n", ftmp->vid);
        uint8_t *buffer = (uint8_t*)aligned_alloc(CACHE_ALIGN_SIZE, align64(ftmp->size));
        dev_ret = icraft_device_read_plmem(buffer, ftmp->addr, ftmp->size);
        if(dev_ret){
            fmsh_print("[FTMP_DUMP] read ftmp data from pl ddr failed, device err code: %u\r\n", dev_ret);
            return ICRAFT_FTMP_GET_DATA_FAIL;
        }

        cur_len = strlen(dump_path);
        remain_len = sizeof(dump_path) - cur_len;
        ret = snprintf(dump_path + cur_len, remain_len, "/%u.ftmp", ftmp->vid);
        if(ret < 0){
            fmsh_print("[FTMP_DUMP] encode error when concat file path for dump ftmp\r\n");
            return ICRAFT_FTMP_GENERATE_PATH_FAIL;
        }
        if(ret >= sizeof(dump_path)){
            fmsh_print("[FTMP_DUMP] dump path has been truncated, needed buffer size is %u\r\n", ret+1);
            return ICRAFT_FTMP_GENERATE_PATH_FAIL;
        }

        // cur_len = strlen(dump_path);
        // remain_len = sizeof(dump_path) - cur_len;
        // ret = snprintf(dump_path + cur_len, remain_len, "%s", suffix);
        // if(ret < 0){
        //     icraft_print("[FTMP_DUMP] encode error when concat file path for dump ftmp\r\n");
        //     return ICRAFT_FTMP_GENERATE_PATH_FAIL;
        // }
        // if(ret >= sizeof(dump_path)){
        //     icraft_print("[FTMP_DUMP] dump path has been truncated, needed buffer size is %u\r\n", ret+1);
        //     return ICRAFT_FTMP_GENERATE_PATH_FAIL;
        // }

        icraft_print("[FTMP_DUMP] dump path: %s\r\n", dump_path);
        //icraft_print_mem(buffer, 256);
        ff_ret = icraft_ff_write_buffer_to_file(dump_path, buffer, ftmp->size, &written_size);
        free(buffer);
        if(ff_ret || (written_size != ftmp->size)){
            fmsh_print("[FTMP_DUMP] write buffer to file fail, written size: %u, icraft ff err code: %u\r\n", 
                        written_size, ff_ret);
            return ICRAFT_FTMP_DUMP_FAIL;
        }
        icraft_print("[FTMP_DUMP] dump ftmp(vid=%u, ETM) to file(path=%s) success!\r\n", 
            ftmp->vid, dump_path);
        break;
    
    case ICRAFT_MTYPE_HOST:
        icraft_print("[FTMP_DUMP] dumping ftmp(vid=%u, HOST)...\r\n", ftmp->vid);
        if(ftmp->data == NULL){
            fmsh_print("[FTMP_DUMP] ftmp(vid=%u, HOST) data is NULL\r\n", ftmp->vid);
            return ICRAFT_FTMP_NO_DATA;
        }
        cur_len = strlen(dump_path);
        remain_len = sizeof(dump_path) - cur_len;
        ret = snprintf(dump_path + cur_len, remain_len, "/%u.ftmp", ftmp->vid);
        if(ret < 0){
            fmsh_print("[FTMP_DUMP] encode error when concat file path for dump ftmp\r\n");
            return ICRAFT_FTMP_GENERATE_PATH_FAIL;
        }
        if(ret >= sizeof(dump_path)){
            fmsh_print("[FTMP_DUMP] dump path has been truncated, needed buffer size is %u\r\n", ret+1);
            return ICRAFT_FTMP_GENERATE_PATH_FAIL;
        }

        // cur_len = strlen(dump_path);
        // remain_len = sizeof(dump_path) - cur_len;
        // ret = snprintf(dump_path + cur_len, remain_len, "%s", suffix);
        // if(ret < 0){
        //     icraft_print("[FTMP_DUMP] encode error when concat file path for dump ftmp\r\n");
        //     return ICRAFT_FTMP_GENERATE_PATH_FAIL;
        // }
        // if(ret >= sizeof(file_name)){
        //     icraft_print("[FTMP_DUMP] dump path has been truncated, needed buffer size is %u\r\n", ret+1);
        //     return ICRAFT_FTMP_GENERATE_PATH_FAIL;
        // }

        icraft_print("[FTMP_DUMP] dump path: %s\r\n", dump_path);
        ff_ret = icraft_ff_write_buffer_to_file(dump_path, ftmp->data, ftmp->size, &written_size);
        if(ff_ret){
            fmsh_print("[FTMP_DUMP] write buffer to file fail, icraft ff err code: %u\r\n", ff_ret);
            return ICRAFT_FTMP_DUMP_FAIL;
        }
        icraft_print("[FTMP_DUMP] dump ftmp(vid=%u, HOST) to file(path=%s) success!\r\n", 
            ftmp->vid, dump_path);
        break;

    case ICRAFT_MTYPE_OCM:
        icraft_print("[FTMP_DUMP] skip dumping ftmp(vid=%u, OCM)\r\n");
        break;

    default:
        icraft_print("[FTMP_DUMP] unknown mtype of ftmp, get mtype=%u\r\n", mtype);
        return ICRAFT_FTMP_INVALID_MTYPE;
    }

    return ICRAFT_SUCCESS;
}

icraft_return _freeFtmp(icraft_ftmp_info_t* self) {

    /*
    if (self->data != NULL && self->mtype == ICRAFT_MTYPE_HOST) {
        free(self->data);
    }
    */

    return ICRAFT_SUCCESS;
}