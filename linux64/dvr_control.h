#ifndef INCLUDE_DVR_CONTROL_H
#define INCLUDE_DVR_CONTROL_H
#include <string>
#include <map>
#include <vector>
#include <mutex>
#include "HCNetSDK.h"
#include "play.h"
//#include "thread_pool.h"
struct VideInfo {
  long startTime;
  long endTime;
  int speed;
  int status;
  StreamType dwStreamType;
};

enum PlayTaskStatus {
  START,
  STOP
};


class PlayTask {
  public:
    PlayTask();
    PlayTask(int taskId, int userId, int channel);
    PlayTask(int taskId, int userId, int channel, long start, long end);

    int getTaskId()    const {
      return taskId;
    }

    int getChannel()    const {
      return channel;
    }

    int getUserId() const {
      return userId;
    }
    
    const VideInfo &getVide() const {
      return vide;
    }

    void addBytes(long n) {
      inputByte += n;
    }
    
    long getInputBytes() {
      return inputByte;
    }

    void setInputBytes(long nByte) {
      inputByte = nByte;
    }

    long getStartTime() const {
      return startTime;
    }

    void setPlayHandle(int handle) {
      this->playHandle = handle;
    }

    int getPlayHandle() {
      return playHandle;
    }

    void setPlayDePort(int port) {
      dePort = port;
    }

    int getDePort() const {
      return dePort;
    }

    void setStatus(PlayTaskStatus status) {
      this->status = status;
    }

    PlayTaskStatus getStatus() {
      return status;
    }

    void setUpdateTime(long updateTime) {
      this->updateTime = updateTime;
    }

    long getUpdateTime() {
      return updateTime;
    }

    PlayType getPlayType() {
      return playType;
    }

    void setPlayType(PlayType playType) {
      this->playType = playType;
    }

    void setStreamType(StreamType type) {
      vide.dwStreamType = type;
    }

    StreamType getStreamType() {
      return vide.dwStreamType;
    }
    
  private:
    int taskId;
    long startTime;
    long updateTime;
    int channel;
    int userId;
    volatile long inputByte;
    VideInfo vide;
    int playHandle;
    int dePort;
    volatile PlayTaskStatus status;
    PlayType playType;
};

class DVRControl {
  public:
    bool addTask(PlayTask *playTask);
    int play(int taskId);
    int stopPlayTask(int taskId);
    int playControl(int taskId, int flag, long param);
    std::string getTaskInfo();

    void addNbytes(int taskId, long nbyte);

    int getTaskStatus(int taskId);

    int getPlayDePort(int taskId);

    void setPlayHandle(int taskId, int handle);
     
    int heartBeat(int taskId);
    void taskHeartCheck();
 
  private:
    PlayTask* getPlayTask(int taskId);
    std::map<int, PlayTask> playTaskMap;
    std::map<int, long> userMap;
    std::mutex lock;
    
};

DVRControl & getDVRControl();

void heartBeatCheck();

#endif
