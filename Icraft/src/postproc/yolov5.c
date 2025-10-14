#include <stdint.h>
#include <stdlib.h>
#include <math.h>
#include "postproc/yolov5.h"
#include "platform/fmsh_print.h"
#include "gic/icraft_gic.h"

/*************************************************************************************/
/*                                                                                   */
/*                               PostProcess: Yolo                                   */
/*                                                                                   */
/*************************************************************************************/

struct Box sigmoid8bit(int8_t* tensor_data, float scale, int obj_ptr_start){
  struct Box res;
  res.x = 1.0 / (1.0 + expf(-tensor_data[obj_ptr_start + 0] * scale));
  res.y = 1.0 / (1.0 + expf(-tensor_data[obj_ptr_start + 1] * scale));
  res.w = 1.0 / (1.0 + expf(-tensor_data[obj_ptr_start + 2] * scale));
  res.h = 1.0 / (1.0 + expf(-tensor_data[obj_ptr_start + 3] * scale));
  res.s = 1.0 / (1.0 + expf(-tensor_data[obj_ptr_start + 4] * scale));  
  return res;
}

struct Box sigmoid16bit(int16_t* tensor_data, float scale, int obj_ptr_start){
  struct Box res;
  res.x = 1.0 / (1.0 + expf(-tensor_data[obj_ptr_start + 0] * scale));
  res.y = 1.0 / (1.0 + expf(-tensor_data[obj_ptr_start + 1] * scale));
  res.w = 1.0 / (1.0 + expf(-tensor_data[obj_ptr_start + 2] * scale));
  res.h = 1.0 / (1.0 + expf(-tensor_data[obj_ptr_start + 3] * scale));
  res.s = 1.0 / (1.0 + expf(-tensor_data[obj_ptr_start + 4] * scale));
  
  return res;
}

float yolov5_overlap(float x1, float w1, float x2, float w2) {

    float l1 = x1 - w1 / 2;
    float l2 = x2 - w2 / 2;
    float left = l1 > l2 ? l1 : l2;
    float r1 = x1 + w1 / 2;
    float r2 = x2 + w2 / 2;
    float right = r1 < r2 ? r1 : r2;

    return right - left;
}


float yolov5_inter_section(struct Box a, struct Box b) {

    float w = yolov5_overlap(a.x, a.w, b.x, b.w);
    float h = yolov5_overlap(a.y, a.h, b.y, b.h);

    if (w < 0 || h < 0) return 0;

    return w * h;
}


float yolov5_union_section(struct Box a, struct Box b) {

    float i = yolov5_inter_section(a, b);

    return a.w * a.h + b.w * b.h - i;
}


float yolov5_iou(struct Box a, struct Box b) {

    float I = yolov5_inter_section(a, b);
    float U = yolov5_union_section(a, b);

    if (U == 0) return 0;

    return I / U;
}

int yolov5_cmpfunc(const void* a, const void* b) {
  float a_s = ((struct Box*)a)->s;
  float b_s = ((struct Box*)b)->s;
  return a_s < b_s ? 1 : -1;
}

void yolov5_BY(int net_img_w, int net_img_h, float* ofm_nr,  uint32_t* result_data, uint32_t* post_result, int8_t dtype){
  //-------------------------------------//
  //            Data Type
  //-------------------------------------//
  int BIT;
  int MAXC;
  int MINC;
  if (dtype == 0 || dtype == 1) {
    BIT = 8;
    MAXC = 64;
    MINC = 8;
  }
  else {
    BIT = 16;
    MAXC = 32;
    MINC = 4;
  }
  //-------------------------------------//
  //             Params
  //-------------------------------------//
  float THR_F = 0.40;
  float IOU   = 0.45;
  int NET_W   = net_img_w;
  int NET_H   = net_img_h;
  float* SCALE = ofm_nr;  
  int N_CLASS = 80;
  int GROUPS  = 3;
  int ANCHOR_LENGTH = ceil((5 + N_CLASS) / (float)MINC) * MINC + MINC; // 每个条条的长度：(85->88) + 4 = 92 注：16比特
  float STRIDE[3] = { 8, 16, 32 };
  float ANCHORS[3][3][2] = { { { 10, 13}, { 16,  30}, { 33,  23} } ,
                              { { 30, 61}, { 62,  45}, { 59, 119} } ,
                              { {116, 90}, {156, 198}, {373, 326} } };

  int img_w = 640;
  int img_h = 640;
  if (PLIN) {
    img_w = CAM_W;
    img_h = CAM_H;
  }
  const float w_ratio = (float)NET_W / img_w;
  const float h_ratio = (float)NET_H / img_h;
  const float ratio = w_ratio <= h_ratio ? w_ratio : h_ratio;
  int w_unpad = (int)round(img_w * ratio);
  int h_unpad = (int)round(img_h * ratio);

  icraft_print("anchor length: %u, w_ratio: %f, h_ratio: %f\r\n", ANCHOR_LENGTH, w_ratio, h_ratio);
  
  struct Box* box_array = (struct Box*) malloc(sizeof(struct Box)*(result_data[0]+result_data[1]+result_data[2]));
  int8_t* tensor_data_8 = (int8_t*)post_result;
  int16_t* tensor_data_16 = (int16_t*)post_result;
  int box_id = 0;
  for(int yolo = 0; yolo < GROUPS; yolo++){
    int objnum = result_data[yolo];
    int output_size = objnum * ANCHOR_LENGTH;        //输出了多少个16比特
    if (output_size != 0) {
      for (int i = 0; i < objnum; i++) {
        int obj_ptr_start = i * ANCHOR_LENGTH;
        int obj_ptr_next = obj_ptr_start + ANCHOR_LENGTH;
        int16_t anchor_index, location_y, location_x;
        struct Box box_;
        switch (BIT){
          case 8: {
            int8_t anchor_index1 = tensor_data_8[i * ANCHOR_LENGTH + ANCHOR_LENGTH - 1];
            int8_t anchor_index2 = tensor_data_8[i * ANCHOR_LENGTH + ANCHOR_LENGTH - 2];
            int8_t location_y1 = tensor_data_8[i * ANCHOR_LENGTH + ANCHOR_LENGTH - 3];
            int8_t location_y2 = tensor_data_8[i * ANCHOR_LENGTH + ANCHOR_LENGTH - 4];
            int8_t location_x1 = tensor_data_8[i * ANCHOR_LENGTH + ANCHOR_LENGTH - 5];
            int8_t location_x2 = tensor_data_8[i * ANCHOR_LENGTH + ANCHOR_LENGTH - 6];
            anchor_index = ((((int16_t)anchor_index1) << 8) + ((int16_t)anchor_index2));
            location_y = ((((int16_t)location_y1) << 8) + ((int16_t)location_y2));
            location_x = ((((int16_t)location_x1) << 8) + ((int16_t)location_x2));
            box_ = sigmoid8bit(tensor_data_8, SCALE[yolo], obj_ptr_start);
            int8_t* class_ptr_start = tensor_data_8 + obj_ptr_start + 5;
            int8_t max_prob = -128;
            int max_prob_index = -1;
            for(int j = 0; j<N_CLASS; j++){
              if(class_ptr_start[j] > max_prob) {
                max_prob = class_ptr_start[j];
                max_prob_index = j;
              }
            }
            box_.s = box_.s / (1 + expf(-max_prob * SCALE[yolo]));
            box_.cls = max_prob_index;
            
            break;          
          }
          case 16:{
            anchor_index = tensor_data_16[obj_ptr_next - 1];
            location_y = tensor_data_16[obj_ptr_next - 2];
            location_x = tensor_data_16[obj_ptr_next - 3];
            box_ = sigmoid16bit(tensor_data_16, SCALE[yolo], obj_ptr_start);
            int16_t* class_ptr_start = tensor_data_16 + obj_ptr_start + 5;
            int16_t max_prob = -32768;
            int max_prob_index = -1;
            for(int j = 0; j<N_CLASS; j++){
              if(class_ptr_start[j] > max_prob) {
                max_prob = class_ptr_start[j];
                max_prob_index = j;
              }
            }
            box_.s = box_.s / (1 + expf(-max_prob * SCALE[yolo]));
            box_.cls = max_prob_index;
            break;
          }
        }
        box_.x = (2 * box_.x + location_x - 0.5) * STRIDE[yolo] / ratio;
        box_.y = (2 * box_.y + location_y - 0.5) * STRIDE[yolo] / ratio;
        box_.w = 4 *powf(box_.w,2) * ANCHORS[yolo][anchor_index][0] / ratio;
        box_.h = 4 * powf(box_.h,2) * ANCHORS[yolo][anchor_index][1] / ratio;
        box_.x = box_.x - box_.w / 2;
        box_.y = box_.y - box_.h / 2;
        if(box_.s > THR_F) {
          box_array[box_id++] = box_;
        }
      }
      tensor_data_8 += output_size;    
      tensor_data_16 += output_size; 
    }
  }
  
  //-------------------------------------//
  //       后处�?�?NMS
  //-------------------------------------//
  //排序
  qsort(box_array, box_id, sizeof(struct Box), yolov5_cmpfunc);

  //全部置为1
  int* valid_id = (int*) malloc(sizeof(int)*(box_id));
  for (int i = 0; i < box_id; i++) {
      valid_id[i] = 1;
  }

  //从大到小遍历
  for (int i = 0; i < box_id; i++) {
      if (valid_id[i] == 1) {                                                     // 如果�?
          for (int j = i + 1; j < box_id; j++) {                                  // 遍历它之后的�?
              if (valid_id[j] == 1 && box_array[i].cls == box_array[j].cls) {     // 如果后续框为1且类别相�?
                  float qq = yolov5_iou(box_array[i], box_array[j]);
                  if (qq > IOU) {                                                 // 且重�?
                      valid_id[j] = 0;                                            // 把后续框置零
                  }
              }
          }
      }
  }

/*************************************************************************************/
/*                                                                                   */
/*                       Debug Part (activated if DEBUGMODE==1)                      */
/*                                                                                   */
  for (int i = 0; i < box_id; i++) {
      if (valid_id[i] == 1) {
          float x = box_array[i].x;
          float y = box_array[i].y;
          float w = box_array[i].w;
          float h = box_array[i].h;
          float s = box_array[i].s;
          int c = box_array[i].cls;
          fmsh_print("c:%d s:%f x:%f y:%f w:%f h:%f\r\n", c, s, x, y, w, h);      
      }
  }
  if (box_id == 0) {
  fmsh_print("no dection\r\n");
  }

/*************************************************************************************/
  free(box_array);
  free(valid_id); 
};

void runYoloPost(icraft_network_info_t* network){    
  //post process: yolov5 yolo
  // only test for gic
  //uint64_t yolo_begin = get_current_time(); //time
  icraft_return ret;
  icraft_list_node_t** customop = (icraft_list_node_t**)aligned_alloc(CACHE_ALIGN_SIZE, 2 * sizeof(icraft_list_node_t*));
  ret = icraft_network_network_check(network, customop);
    if (ret) {
        fmsh_print("[ERROR] error in check net for imk&detpost check, err code:%u", ret);
        return;
    }
  icraft_imagemake_t *imk = (icraft_imagemake_t *)(customop[0]->data);
  icraft_detpost_t *detpost = (icraft_detpost_t *)(customop[1]->data);
  uint32_t net_img_w = imk->width;
  uint32_t net_img_h = imk->height;

  //TODO
  uint32_t* result_data = detpost->result_data;
  uint32_t* post_result = detpost->post_result;
  
  uint32_t ofm_num = detpost->basic_info->ofms_num;
  float* ofm_nr = (float*)aligned_alloc(CACHE_ALIGN_SIZE, align64(ofm_num * sizeof(float)));

  for(int i = 0; i < ofm_num; ++i){
    if(detpost->basic_info->ofms_ptr[i]->normratio_data == NULL){

    }
    ofm_nr[i] = detpost->basic_info->ofms_ptr[i]->normratio_data[0];
  } 
  yolov5_BY(net_img_w, net_img_h, ofm_nr, result_data, post_result, detpost->basic_info->ifms_ptr[0]->dtype);  
  free(customop);
  free(ofm_nr);
  //uint64_t yolorun_cost_ = get_current_time() - yolo_begin;
}