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
#define MAX_DECODE_TASK 250
static ExecutorService *executorService[MAX_THREAD_NUM];
static ExecutorService *executorServiceMessage[MAX_THREAD_NUM];

void exeServiceInit() {
  for(int i = 0; i < MAX_THREAD_NUM; i++) {
    executorService[i] = Executors::NewFixPool(1, MAX_DECODE_TASK);
  }

  for(int i = 0; i < MAX_THREAD_NUM; i++) {
    executorServiceMessage[i] = Executors::NewFixPool(1, MAX_DECODE_TASK);
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
	std::vector<int> param (2, 0); 
    param[0]=CV_IMWRITE_JPEG_QUALITY; 
    param[1]=95;//default(95) 0-100 
    imencode(".jpg", dst, *inImage, param);
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

void DecodeTask::ErrorMsg(int id, const std::string &msg) {
  LOG(ERROR) << "error " << msg;
}


void DecodeTask::Run() {
  
  std::vector<unsigned char> image;
  YV12ToBGR24_OpenCV((unsigned char*)pbuf_, &image, param_.width, param_.height);
  return;
  std::string imageBase64;
  if (image.empty()) {
    return;
  }
  Base64::getBase64().encode(image, imageBase64);
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
  	std::unique_ptr<MessageTask> messageTask(new MessageTask(param_.topic, res));
    executorServiceMessage[param_.taskId & 0xf]->Execute(std::move(messageTask));
  }
}

MessageTask::MessageTask(const std::string &topic, const std::string &mess) 
	:topic_(topic),
	 mess_(mess) {
}

MessageTask::~MessageTask(){}

void MessageTask::Run() {
  if (!topic_.empty()) {
  	// LOG(INFO) << "send to kafka size: " << mess_.size();
    getGuardStore(0).Send(topic_, mess_, 0);
  }
}

void MessageTask::ErrorMsg(int code, const std::string message) {
  LOG(INFO) <<"topic:" << topic_ << "msg:" << message;
}


