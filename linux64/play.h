#ifndef INCLUDE_PLAY_H
#define INCLUDE_PLAY_H
#include "HCNetSDK.h"
#include "threadpool/thread_pool.h"
#include "sdk_common.h"
class PlayTask;
  
class DecodeTask : public Runnable {
 public:
  // DecodeTask(int port, int channel, PlayType playType, const std::vector<unsigned char> &image);
  DecodeTask(int port, int taskId, const std::string topic, const char *buf, int size, int width, int height);
  void Run();
  virtual ~DecodeTask();
  
 private:
  int nPort;
  int taskId_;
  std::string topic_;
  char *pbuf_;
  int size_;
  int width_;
  int height_;
};

int playReal(PlayTask *playTask);
int stopPlay(int handle, int port, PlayType type);
int playBackControl(int handle, int flag, long param);
int playGetPos(int handle, int *pos);
void exeServiceInit();
bool hasFilePlay(int lUserId, int channel, long startTime, long endTime);
ExecutorService *getPlayService();
#endif
