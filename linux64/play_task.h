#ifndef INCLUDE_PLAY_TASK_H
#define INCLUDE_PLAY_TASK_H
#include <stdio.h>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>
#include "sdk_common.h"
#include <glog/logging.h>
struct VideInfo {
  long startTime;
  long endTime;
  int speed;
  int status;
  int pos;
  StreamType dwStreamType;
};

class SdkApi;
class PlayTask {
  public:
    PlayTask();

    PlayTask(int taskId, int userId, int channel, PlayType type);

    PlayTask(int taskId, int userId, int channel, PlayType type, long start, long end);

	~PlayTask() {
		delete sendThdId;
    }
	
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

    void setSdkApi(SdkApi *api) {
      this->api = api;
    }
    void setCameraId(int id) {
      this->cameraId = id;
    }

    void setCameraName(std::string cameraName) {
      this->cameraName = cameraName;
    }

    void setAreaName(std::string areaName) {
      this->areaName = areaName;
    }

    int getCameraId() {
      return cameraId;
    }

    std::string getCameraName() {
      return cameraName;
    }

    std::string getAreaName() {
      return areaName;
    }

    SdkApi *getSdkApi() {
      return api;
    }

	std::shared_ptr<TaskParam> getPack();
	void addPack(std::shared_ptr<TaskParam> pack);
	
	int getWritePackIndex() {
	  return writePackIndex;
    }

	void setWritePackIndex(int inx) {
	  writePackIndex = inx;
    }

	void  setSendThdId(std::thread* id) {
	  sendThdId = id;
	}

	std::thread* getSendThdId() {
	  return sendThdId;
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
    int cameraId;
    std::string cameraName;
    std::string areaName;
	std::map<int, std::shared_ptr<TaskParam>> packMap_;
	std::thread *sendThdId{NULL};
	int readPackIndex{0};
	int writePackIndex{0};
	int timeout{0};
	std::mutex lock_;
    SdkApi *api;
};
#endif
