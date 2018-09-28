#ifndef INCLUDE_PLAY_H
#define INCLUDE_PLAY_H
#include "HCNetSDK.h"
#include "threadpool/thread_pool.h"
#include "sdk_common.h"
class PlayTask;
class SdkApi;
struct TaskParam{
  SdkApi *api;
  int port;
  int taskId;
  std::string topic;
  const char *buf;
  int size;
  int width;
  int height;
  long timestamp;
  int cameraId;
  std::string cameraName;
  std::string areaName;
};
class DecodeTask : public Runnable {
 public:
  DecodeTask(const TaskParam &param);
  void Run();
  virtual ~DecodeTask();
  
 private:
  TaskParam param_;
  char *pbuf_;
};

int playGetPos(int handle, int *pos);
void exeServiceInit();
bool hasFilePlay(int lUserId, int channel, long startTime, long endTime);
ExecutorService *getPlayService(int i);
#endif
