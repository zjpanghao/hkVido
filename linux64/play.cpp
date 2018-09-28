#include "play.h"
#include <stdio.h>
#include "public.h"
#include "string.h"
#include <stdlib.h>
#include <unistd.h>

#include"opencv2/core/core.hpp"
#include"opencv2/highgui/highgui.hpp"
#include"opencv2/imgproc/imgproc.hpp"

#include "base64.h"
#include <memory>
#include <string>
#include <vector>
#include <glog/logging.h>
#include "json/json.h"
#include "store.h"
#include "store_factory.h"
#include "PlayM4.h"
#include "dvr_control.h"
#include "sdk_common.h"
#include "sdk_api.h"
using namespace cv;
#define MAX_THREAD_NUM 16
static ExecutorService *executorService[MAX_THREAD_NUM];
void exeServiceInit() {
  for(int i = 0; i < MAX_THREAD_NUM; i++) {
    executorService[i] = Executors::NewFixPool(1);
  }
}

ExecutorService *getPlayService(int i) {
  if (i < 0 || i >= MAX_THREAD_NUM) {
    return NULL;
  }
  return executorService[i];
}

bool YV12ToBGR24_OpenCV(unsigned char* pYUV,std::vector<unsigned char> *inImage,int width,int height) {
    if (width < 1 || height < 1 || pYUV == NULL)
        return false;
    Mat dst(height, width, CV_8UC3);
    Mat src(height + height/2, width, CV_8UC1, pYUV);
    cvtColor(src, dst, CV_YUV2BGR_YV12);
    imencode(".jpg", dst, *inImage);
    return true;
}

DecodeTask::DecodeTask(
 const TaskParam &param)
    :  pbuf_(NULL), param_(param) {
  
  pbuf_ = (char*)malloc(param.size);
  memcpy(pbuf_, param.buf, param.size);
}

DecodeTask::~DecodeTask() {
  free(pbuf_);
}

void DecodeTask::Run() {
  std::vector<unsigned char> image;
  YV12ToBGR24_OpenCV((unsigned char*)pbuf_, &image, param_.width, param_.height);
  std::string imageBase64;
  if (image.empty()) {
    return;
  }
  imageBase64 = Base64::getBase64().encode(image);
  Json::Value root;
  root["bufferedImageStr"] = imageBase64;
  root["taskId"] = param_.taskId;
  //root["type"] = (int)playType;
  // root["stamp"] = (unsigned int)(param_.api)->getTimeStamp(param_.port);
  root["stamp"] = (unsigned int)(param_.timestamp);
  root["cameraId"] = param_.cameraId;
  root["cameraName"] = param_.cameraName;
  root["area"] = param_.areaName;
  String res = root.toStyledString();
  if (!param_.topic.empty()) {
    getGuardStore(0).Send(param_.topic, res, 0);
  }
}



