#ifndef INCLUDE_PLAY_H
#define INCLUDE_PLAY_H
#include "HCNetSDK.h"
#include "threadpool/thread_pool.h"
#include "sdk_common.h"
#include <glog/logging.h>
class PlayTask;
class SdkApi;

class DecodeTask : public Runnable {
 public:
  DecodeTask(const TaskParam &param);
  void Run();
  virtual ~DecodeTask();
  virtual void ErrorMsg(int id, const std::string &msg);
  
 private:
  TaskParam param_;
  char *pbuf_;
  static float jpgQuality_;
  static float step_;
};

class MessageTask : public Runnable {
 public:
  MessageTask(const std::string &topic, const      std::string &mess);
  void Run();
  virtual void ErrorMsg(int code, const std::string message);
  virtual ~MessageTask();
  
 private:
  std::string topic_;
  std::string mess_;
};

int playGetPos(int handle, int *pos);
void exeServiceInit();
bool hasFilePlay(int lUserId, int channel, long startTime, long endTime);
ExecutorService *getPlayService(int i);
ExecutorService *getStoreService(int i);

#endif
