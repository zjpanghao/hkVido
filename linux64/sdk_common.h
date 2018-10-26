#ifndef INCLUDE_SDK_COMMON_H
#define INCLUDE_SDK_COMMON_H
#include <map>
#include <string>
#include <thread>
#include <vector>
#include "sdk_common.h"
class SdkApi;
class PlayTask;
enum PlayType {
  PLAYREAL,
  PLAYBACK
};

enum StreamType {
  MAIN_STREAM,
  CHILD_STREAM
};

enum FactoryType {
  HAIKANG,
  DAHUA
};

struct DeviceInfo {
  unsigned char type;
  unsigned char byChanNum;                    //模拟通道个数
  unsigned char byStartChan;
  unsigned char byIPChanNum;
  unsigned char byStartDChan;
};

enum PlayTaskStatus {
  START,
  STOP
};

struct TaskParam{
  PlayTask *task_;
  long timestamp;
  const char *buf;
  int size;
  int width;
  int height;
  int inx;
  std::vector<unsigned char> image;
};

  
#endif
