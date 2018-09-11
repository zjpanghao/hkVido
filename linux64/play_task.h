#ifndef INCLUDE_PLAY_TASK_H
#define INCLUDE_PLAY_TASK_H
#include <stdio.h>
#include <string>

#include "sdk_common.h"
struct VideInfo {
  long startTime;
  long endTime;
  int speed;
  int status;
  int pos;
  StreamType dwStreamType;
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

    void setPos(int pos) {
      vide.pos = pos;
    }

    int getPos() {
      return vide.pos;
    }

    std::string &getTopic() {
      return topic;
    }

    void setTopic(const std::string &topic) {
      this->topic = topic;
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
    std::string topic;
};
#endif
