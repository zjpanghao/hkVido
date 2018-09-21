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
static ExecutorService *executorService;
void exeServiceInit() {
  executorService = Executors::NewFixPool(10);
}

ExecutorService *getPlayService() {
  return executorService;
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

static std::string getSerial(int taskId, 
  const PLAYM4_SYSTEM_TIME &playm4SystemTime, 
  const std::string &data) {
  Json::Value root;
  //root["channel"] = channel;
  root["data"] = data;
  root["taskId"] = taskId;
  //root["type"] = (int)playType;
  // root["time"] = getPlayTimeStr(playm4SystemTime);
  // LOG(INFO) << getPlayTimeStr(playm4SystemTime);
  return root.toStyledString();
}

DecodeTask::DecodeTask(
  int port,  
  int taskId, 
  const std::string topic,
  const char *buf, 
  int size, 
  int width, 
  int height)
    : nPort(port),taskId_(taskId), topic_(topic), size_(size), pbuf_(NULL), width_(width), height_(height) {
  pbuf_ = (char*)malloc(size);
  memcpy(pbuf_, buf, size);
}

DecodeTask::~DecodeTask() {
  free(pbuf_);
}

void DecodeTask::Run() {
  std::vector<unsigned char> image;
  YV12ToBGR24_OpenCV((unsigned char*)pbuf_, &image, width_, height_);
  std::string imageBase64;
  if (image.empty()) {
    return;
  }
  imageBase64 = Base64::getBase64().encode(image);
  // base64Encry((char*)&image[0], image.size(), &imageBase64);
  PLAYM4_SYSTEM_TIME playm4SystemTime;
  PlayM4_GetSystemTime(nPort, &playm4SystemTime);
  // LOG(INFO) << channel;
  String res = getSerial(taskId_, playm4SystemTime, imageBase64);
  if (!topic_.empty()) {
    getGuardStore(0).Send(topic_, res, 0);
  }
}



