#include <stdint.h>
#include <stdlib.h>
#include <math.h>
#include "postproc/segpost.h"
#include "platform/fmsh_print.h"
#include "gic/icraft_gic.h"

/*************************************************************************************/
/*                                                                                   */
/*                               PostProcess: Segpost                                */
/*                                                                                   */
/*************************************************************************************/

icraft_segpost_t*
icraft_network_segpost_check(icraft_network_info_t* network) {
    if(network == NULL) {
        fmsh_print("[ERROR] netowrk is NULL\r\n");
        return NULL;
    }

    icraft_list_node_t *current_op = network->ops;

    while(current_op != NULL){
        icraft_basic_op_t *operation = (icraft_basic_op_t*)(current_op->data);
        if (operation->basic_info->op_type == ICRAFT_OP_SEGPOST) {
            return (icraft_segpost_t*)(current_op->data);
        }
        current_op = current_op->next;
    }

        fmsh_print("[ERROR] this network not exist segpost\r\n");
        return NULL;
    
}

void process_index_and_conf_array(
    uint8_t* data,
    uint8_t* index_array,
    float* conf_array,
    uint32_t height,
    uint32_t width,
    float norm,
    int mode
) {

    for (int i = 0; i < height; ++i) {
        for (int j = 0; j < width; ++j) {
            int base = (i * width + j) * 4;
            // indexΪ��0ͨ��
            index_array[i * width + j] = data[base + 0];
            // confΪ������ͨ��
            uint32_t conf_3rd = data[base + 1];
            uint32_t conf_2en = data[base + 2];
            uint32_t conf_1st = data[base + 3];
            int32_t combine_value = 0;
            float combine_float = 0.0f;
            if (mode == 1) {
                combine_value = (conf_1st << 16) | (conf_2en << 8) | conf_3rd;
                
                if (conf_1st & 0x80) {
                  combine_value |= 0xFF000000;
                }
                combine_float = (combine_value / 16.0f) * norm;
            }
            else {
                combine_value = (conf_2en << 8) | conf_3rd;
                combine_float = (float)combine_value * norm;
            }
            conf_array[i * width + j] = combine_float;
        }
    }
}

void segpostPost(icraft_network_info_t* network){    
  //post process: segpost

  icraft_segpost_t* segpost_op = icraft_network_segpost_check(network);

  int mode = segpost_op->mode;

  icraft_ftmp_info_t* out_ftmp = segpost_op->basic_info->ofms_ptr[0];
  uint32_t height = out_ftmp->shape[1];
  uint32_t width = out_ftmp->shape[2];
  uint8_t* data = out_ftmp->data; 
  uint8_t* index_array = (uint8_t*)aligned_alloc(64, height * width);
  float norm = out_ftmp->normratio_data[0];
  icraft_print("[segpostPost] height: %d\r\n", height);
  icraft_print("[segpostPost] width: %d\r\n", width);
  icraft_print("[segpostPost] norm: %f\r\n", norm);
  icraft_print("[segpostPost] mode: %d\r\n", mode);
  float* conf_array = (float*)aligned_alloc(64, height * width * sizeof(float));
  process_index_and_conf_array((char*)data, index_array, conf_array, height, width, norm, mode);
  
  /* dump to file
  uint32_t written_size = 0;
  icraft_ff_write_buffer_to_file("/index_array.ftmp", (char*)index_array, height * width, &written_size);
  icraft_ff_write_buffer_to_file("/conf_array.ftmp", (char*)conf_array, height * width * sizeof(float), &written_size);
  */
  free(index_array);
  free(conf_array);

}