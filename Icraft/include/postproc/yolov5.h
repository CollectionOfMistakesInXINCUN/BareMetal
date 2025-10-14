#ifndef YOLOV5_H
#define YOLOV5_H
#include <stdint.h>
#include "icraft_network.h"

#define IPOSTPRINT     0  // print icorepost result
#define YOLOPRINT      1  // print yolo post process result

struct Box{
  float x;
  float y;
  float w;
  float h;
  float s;
  int cls;
};

void yolov5_BY(int net_img_w, int net_img_h, float* ofm_nr,  uint32_t* result_data, uint32_t* post_result, int8_t dtype);

//run yolo post
void runYoloPost(icraft_network_info_t* network);

#endif // ! YOLOV5_H