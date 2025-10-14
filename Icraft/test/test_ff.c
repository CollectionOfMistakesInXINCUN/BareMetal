#include "utils/icraft_ff.h"
#include "utils/icraft_cache.h"

void test_ff_load_file(){
    fmsh_print("=================TEST UNPACK LOAD FILE===============\r\n");
    const char *file_path = "/test_flatbuffer/YoloV5s_customop.snapshot";
    icraft_return ret;
    uint32_t fsize;
    void *buffer = NULL;
    ret = icraft_ff_load_file(file_path, &buffer, &fsize);
    if(ret){
        fmsh_print("[RESULT] test load file failed, err code: %d\r\n", ret);
    }
    else if(buffer == NULL){
        fmsh_print("[RESULT] buffer is NULL\r\n");
    }
    else{
        fmsh_print("[RESULT] test load file success\r\n");
    }

    if(buffer != NULL){
        free(buffer);
    }
}


void test_ff_write_file(){
    fmsh_print("=================TEST WRITE BUFFER TO FILE===============\r\n");
    uint32_t data_size = 1024 * 100;
    uint8_t fill_data[3] = {0xCC, 0xFE, 0x11}; 
    uint8_t *data = aligned_alloc(CACHE_ALIGN_SIZE, data_size);
    icraft_return ret;
    const char *file = "/test_flatbuffer/test.bin";
    uint32_t written_size;
    for(uint32_t k = 0;  k < 3; ++k){
        for(uint32_t i = 0; i < data_size ; ++i){
            data[i] = fill_data[k];
        }
        ret = icraft_ff_write_buffer_to_file(file, data, data_size, &written_size);
        if(ret){
            fmsh_print("[RESULT] test write buffer to file failed, icraft ff err code:%u\r\n", ret);
            free(data);
            return;
        }
        fmsh_print("[RESULT] test write buffer to file success\r\n");
    }
    free(data);
    return;
}


void test_ff_mkdir(){
    fmsh_print("=================TEST MKDIR===============\r\n");
    icraft_return ff_ret;
    char dir1[20] = "/wjk/wjk1/wjk2";
    ff_ret = icraft_ff_mkdir(dir1);
    if(ff_ret){
        fmsh_print("[RESULT] create dir1(%s) fail\r\n", dir1);
        //return;
    }
    else{
        fmsh_print("[RESULT] create dir1(%s) success\r\n", dir1);
    }

    char dir2[20] = "/wjk/";
    ff_ret = icraft_ff_mkdir(dir2);
    if(ff_ret){
        fmsh_print("[RESULT] create dir2(%s) fail\r\n", dir2);
        //return;
    }  
    else{ 
        fmsh_print("[RESULT] create dir2(%s) success\r\n", dir2); 
    }

    char dir3[20] = "/wjk";
    ff_ret = icraft_ff_mkdir(dir3);
    if(ff_ret){
        fmsh_print("[RESULT] create dir3(%s) fail\r\n", dir3);
        //return;
    }   
    else{
        fmsh_print("[RESULT] create dir3(%s) success\r\n", dir3); 
    }
}

void test_ff_listdir(){
    fmsh_print("=================TEST LISTDIR===============\r\n");
    char dir[100] = "/input";
    // char dir1[100] = "/test_flatbuffer/models/good";
    // icraft_ff_mkdir(dir1);
    icraft_return ff_ret;
    ff_ret = icraft_ff_listdir(dir);
    if(ff_ret){
        fmsh_print("[RESULT] list dir (%s) failed, icraft ff err code: %u\r\n", dir, ff_ret);
        return;
    }
    fmsh_print("[RESULT] list dir (%s) success!\r\n", dir);
    return;
}


void test_ff_bad_case(){
    fmsh_print("=================TEST FF BAD CASE===============\r\n");
    const char *file_path = "/snapshots/two/YoloV5s.snapshot";
    icraft_return ret;
    uint32_t fsize;
    void *buffer = NULL;

    buffer = aligned_alloc(CACHE_ALIGN_SIZE, 1024 * 1024 * 10);
    fmsh_print("addr outside: 0x%llX\r\n", buffer);

    ret = icraft_ff_load_file(file_path, &buffer, &fsize);
    if(ret){
        fmsh_print("[RESULT] test load file failed, err code: %d\r\n", ret);
    }
    else if(buffer == NULL){
        fmsh_print("[RESULT] buffer is NULL\r\n");
    }
    else{
        fmsh_print("[RESULT] test load file success\r\n");
    }

    if(buffer != NULL){
        free(buffer);
    }    
}