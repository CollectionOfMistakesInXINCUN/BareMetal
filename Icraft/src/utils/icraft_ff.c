#include "utils/icraft_ff.h"
#include "utils/icraft_cache.h"

icraft_return
icraft_ff_load_file(const char *file_path, void **buffer, uint32_t *file_size){
    if((*buffer) != NULL){
        fmsh_print("'buffer' must point to NULL\r\n");
        return ICRAFT_FF_LOAD_FILE_BUFFER_NOT_NULL;
    }
    icraft_print("start load file: %s\r\n", file_path);
    FIL fp;
    FILINFO fileinfo;
    uint32_t rc;  // ff的错误码
    icraft_return ret;  // icraft错误码
    rc = f_open(&fp, file_path, FA_READ);
    if(rc){
        fmsh_print("open file failed, err code: %d, see <ff.h>\r\n", rc);
        f_close(&fp);
        return ICRAFT_FF_LOAD_FILE_FAIL;
    }
    rc = f_stat(file_path, &fileinfo);
    if(rc){
        fmsh_print("stat file failed, err code: %d, see <ff.h>\r\n", rc);
        f_close(&fp);
        return ICRAFT_FF_LOAD_FILE_FAIL;        
    }
    rc = f_rewind(&fp);
    if(rc){
        fmsh_print("rewind file failed, err code: %d, see <ff.h>\r\n", rc);
        f_close(&fp);
        return ICRAFT_FF_LOAD_FILE_FAIL;           
    }
    (*file_size) = fileinfo.fsize;
    icraft_print("file size ---%d\r\n", fileinfo.fsize);
    uint32_t aligned_fsize = (fileinfo.fsize / 64 + 1) * 64;
    *buffer = aligned_alloc(CACHE_ALIGN_SIZE, aligned_fsize);
    if(*buffer == NULL){
        fmsh_print("malloc memory for file failed, size %d\r\n", fileinfo.fsize);
        return ICRAFT_FF_MALLOC_FAIL;
    }
    else{
        icraft_print("malloc memory ---addr 0x%X, ---size %d for file\r\n", *buffer, aligned_fsize);
    }
    rc = f_read(&fp, *buffer, fileinfo.fsize, &rc);
    if(rc){
        fmsh_print("read file to memory failed, err code %d, see <ff.h>\r\n", rc);
        return ICRAFT_FF_LOAD_FILE_FAIL; 
    }
    rc = f_close(&fp);
    if(rc){
        fmsh_print("close file failed, err code %d, see <ff.h>\r\n", rc);
        return ICRAFT_FF_CLOSE_FILE_FAIL; 
    }
    icraft_print("load file success\r\n");
    return ICRAFT_SUCCESS;
}


icraft_return
icraft_ff_write_buffer_to_file(const char *file_path, void * const buffer, uint32_t const buffer_size, uint32_t *written_size)
{
    icraft_print("start writing file: %s, with buffer size=%u\r\n", file_path, buffer_size);
    FIL fp;
    FILINFO fileinfo; 
    icraft_return ret;  // icraft错误码
    ret = f_open(&fp, file_path, FA_CREATE_ALWAYS|FA_READ|FA_WRITE);    // 会创建
    if(ret){
        fmsh_print("open file %s failed, ff err code: %u, see <ff.h>\r\n",file_path, ret);
        f_close(&fp);
        return ICRAFT_FF_CREATE_FILE_FAIL;
    }
    ret = f_write(&fp, buffer, buffer_size, written_size);
    if(ret || (*written_size != buffer_size)){
        fmsh_print("write buffer(size=%u) to file(path=%s) failed, written size=%u\r\n",
            buffer_size, file_path, *written_size);
        f_close(&fp);
        return ICRAFT_FF_WRITE_FILE_FAIL;
    }
    ret = f_close(&fp);
    if(ret){
        fmsh_print("close file failed(path=%s), ff err code %d, see <ff.h>\r\n", file_path, ret);
        return ICRAFT_FF_CLOSE_FILE_FAIL; 
    }
/*
#if ICRAFT_DEBUG
    icraft_print("start checking written data...\r\n");
    void *check_buffer = aligned_alloc(CACHE_ALIGN_SIZE, buffer_size);
    uint32_t load_size;
    ret = icraft_ff_load_file(file_path, &check_buffer, &load_size);
    if(ret){
        icraft_print("load file(path=%s) failed, icraft ff err code: %u\r\n", file_path, ret);
    }
    if(memcmp(check_buffer, buffer, buffer_size) != 0){
        icraft_print("written data to file is uncompatible with original buffer\r\n");
        free(check_buffer);
        return ICRAFT_FF_WRITE_FILE_CHECK_FAIL;
    }
    icraft_print("check write file success\r\n");
    free(check_buffer);
#endif
*/
    return ICRAFT_SUCCESS;
}


icraft_return 
icraft_ff_mkdir(const char* dir)
{
    FILINFO fileinfo;
    FRESULT f_ret;
    f_ret = f_stat(dir, &fileinfo);
    if(f_ret){
        icraft_print("[FF] dir (%s) does not exit, create now...\r\n", dir);
        f_ret = f_mkdir(dir);
        if(f_ret){
            icraft_print("[FF] create dir (%s) fail, bsp ff err code: %u\r\n", dir, f_ret);
            return ICRAFT_FF_CREATE_DIR_FAIL;
        }
        icraft_print("[FF] create dir (%s) success!\r\n", dir);
    }
    else{
        icraft_print("[FF] dir (%s) already exist, no need to create\r\n", dir);
    }

    return ICRAFT_SUCCESS;
}


icraft_return
icraft_ff_listdir(const char *dir)
{
    FRESULT ff_ret;
    DIR dir_obj;
    FILINFO fileinfo;
    uint32_t i = 0;
    uint32_t j = 0;
    
    ff_ret = f_opendir(&dir_obj, dir);
    if(ff_ret){
        icraft_print("[FF] open dir (%s) failed, ff err code: %u\r\n", dir, ff_ret);
        return ICRAFT_FF_OPEN_DIR_FAIL;
    }
    icraft_print("[FF] list files of dir: %s\r\n", dir);
    for(;;){
        ff_ret = f_readdir(&dir_obj, &fileinfo);
        // ascii (0):NULL
        if((ff_ret != FR_OK) || (fileinfo.fname[0] == 0)){
            break;
        }
        if(fileinfo.fattrib & AM_DIR){
            icraft_print("subdir[%u]: %s\r\n", j, fileinfo.fname);
            j++;
        }
        else{
            icraft_print("file[%u]: %s, file size: %lu, time stamp: %u/%02u/%02u, %02u:%02u, attributes: %c%c%c%c%c\r\n",
                i, fileinfo.fname, fileinfo.fsize, (fileinfo.fdate >> 9) + 1980, fileinfo.fdate >> 5 & 15, 
                fileinfo.fdate & 31, fileinfo.ftime >> 11, fileinfo.ftime >> 5 & 63,
                (fileinfo.fattrib & AM_DIR) ? 'D' : '-',
                (fileinfo.fattrib & AM_RDO) ? 'R' : '-',
                (fileinfo.fattrib & AM_HID) ? 'H' : '-',
                (fileinfo.fattrib & AM_SYS) ? 'S' : '-',
                (fileinfo.fattrib & AM_ARC) ? 'A' : '-');
            i++;
        }
    }
    ff_ret = f_closedir(&dir_obj);
    if(ff_ret){
        icraft_print("[FF] close dir (%s) failed, ff err code: %u\r\n", dir, ff_ret);
        return ICRAFT_FF_CLOSE_DIR_FAIL;
    }
    return ICRAFT_SUCCESS;
}

icraft_return
icraft_ff_getfilename(char *filename) {
    FRESULT ff_ret;
    char *dot = strrchr(filename, '.');
    if (dot != NULL) {
        *dot = '\0';
    }
    return 0;
}