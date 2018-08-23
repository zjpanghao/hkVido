#ifndef INCLUDE_PLAY_H
#define INCLUDE_PLAY_H
#include "HCNetSDK.h"
#include "threadpool/thread_pool.h"
class PlayTask;

enum PlayType {
  PLAYREAL,
  PLAYBACK
};

enum StreamType {
  MAIN_STREAM,
  CHILD_STREAM
};
  
class DecodeTask : public Runnable {
 public:
  // DecodeTask(int port, int channel, PlayType playType, const std::vector<unsigned char> &image);
  DecodeTask(int port, int channel, PlayType playType, const char *buf, int size, int width, int height);
  void Run();
  virtual ~DecodeTask();
  
 private:
  int nPort;
  int      channel;
  PlayType type;
  char *pbuf_;
  int size_;
  int width_;
  int height_;
};

int playByTime(int lUserID, int channelId, NET_DVR_TIME *dvrStartTime, NET_DVR_TIME *dvrEndTime, PlayTask *playTask);
int playReal(PlayTask *playTask);
int stopPlay(int handle, int port, PlayType type);
int playBackControl(int handle, int flag, long param);
int playGetPos(int handle, int *pos);
int playBackSetStartTime(int handle);
void exeServiceInit();
bool hasFilePlay(int lUserId, int channel, long startTime, long endTime);
#endif
