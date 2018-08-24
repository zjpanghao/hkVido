#include "dvr_control.h"
#include "unistd.h"
#include "login.h"
#include "play.h"
#include <glog/logging.h>
#include "channel.h"
DVRControl & getDVRControl() {
  static DVRControl dvrControl;
  return dvrControl;
}

PlayTask::PlayTask() {
}

PlayTask::PlayTask(int taskId, int userId, int channel) {
  this->channel = channel;
  this->taskId = taskId;
  this->startTime = time(NULL);
  this->updateTime = time(NULL);
  this->vide.startTime = 0;
  this->vide.endTime = 0;
  this->userId = userId;
  this->inputByte = 0;
  this->playHandle = -1;
  this->playType = PlayType::PLAYREAL;
  this->vide.dwStreamType = StreamType::CHILD_STREAM;
  dePort = -1;
  status = PlayTaskStatus::STOP;
}

PlayTask::PlayTask(int taskId, int userId, int channel, long start, long end) {
  this->channel = channel;
  this->taskId = taskId;
  this->startTime = time(NULL);
  this->updateTime = time(NULL);
  this->vide.startTime = start;
  this->vide.endTime = end;
  this->userId = userId;
  this->inputByte = 0;
  this->playHandle = -1;
  dePort = -1;
  this->playType = PlayType::PLAYBACK;
  status = PlayTaskStatus::STOP;
}

  bool DVRControl::addTask(PlayTask *playTask) {
   ChannelInfo info = {0};
   getChannelControl().getChannelInfo(playTask->getChannel(), &info);
   if (info.state != 0 || info.recordState == false) {
     return false;
   }
   std::lock_guard<std::mutex> guard(lock);
   auto it = playTaskMap.find(playTask->getTaskId());
   if (it != playTaskMap.end()) {
     return false;
   }
   playTaskMap[playTask->getTaskId()] = *playTask;
   return true;
  }

int DVRControl::play(int taskId) {
  std::lock_guard<std::mutex> guard(lock);
  auto it = playTaskMap.find(taskId);
  if (it == playTaskMap.end()) {
    return -1;
  }
  PlayTask &playTask = it->second;
  if (playTask.getPlayType() == PlayType::PLAYREAL) {
    return playReal(&playTask);
  }
  struct tm start;
  struct tm end;
  localtime_r(&playTask.getVide().startTime, &start);
  localtime_r(&playTask.getVide().endTime, &end);
  NET_DVR_TIME   dvrStartTime = {start.tm_year + 1900, 
      start.tm_mon + 1, start.tm_mday, 
      start.tm_hour, start.tm_min, start.tm_sec};
  NET_DVR_TIME   dvrEndTime = {end.tm_year + 1900, end.tm_mon + 1, end.tm_mday, 
      end.tm_hour, end.tm_min, start.tm_sec};
  int rc = 0;
  rc = playByTime(playTask.getUserId(), playTask.getChannel(), &dvrStartTime, &dvrEndTime, &playTask);
  return rc;
}

int DVRControl::getTaskStatus(int taskId) {
  std::lock_guard<std::mutex> guard(lock);
  PlayTask *playTask = getPlayTask(taskId);
  if (playTask) {
    return playTask->getStatus();
  }
  return PlayTaskStatus::STOP;
}

int DVRControl::stopPlayTask(int taskId) {
  {
    std::lock_guard<std::mutex> guard(lock);
    PlayTask *playTask = getPlayTask(taskId);
    if (playTask) {
      playTask->setStatus(PlayTaskStatus::STOP);
      sleep(1);
    } else {
      return -1;
    }
  }
  std::lock_guard<std::mutex> guard(lock);
  PlayTask *playTask = getPlayTask(taskId);
  if (playTask) {
    stopPlay(playTask->getPlayHandle(), playTask->getDePort(), playTask->getPlayType());
    playTaskMap.erase(taskId);
  } else {
    return -1;
  }
  return 0;
}

int DVRControl::playControl(int taskId, int flag, long param) {
  std::lock_guard<std::mutex> guard(lock);
  PlayTask *playTask = getPlayTask(taskId);
  if (playTask) {
    return playBackControl(playTask->getPlayHandle(), flag, param);
  }
  return -1;
}

  void DVRControl::addNbytes(int taskId, long nbyte) {
      std::lock_guard<std::mutex> guard(lock);
      PlayTask* task = getPlayTask(taskId);
      if (task) {
        task->addBytes(nbyte);
      }
  }

std::string DVRControl::getTaskInfo() {
  std::lock_guard<std::mutex> guard(lock);
  auto it = playTaskMap.begin();
  std::string result = "taskId, channel, userId, handle, deport, status, inputbytes(kb)  speed(kb/s) <br>";
  while (it != playTaskMap.end()) {
    PlayTask playTask = it->second;
    char buf[64];
    sprintf(buf ,"%d", playTask.getTaskId());
    result += buf;
    result += ",";
    sprintf(buf ,"%d", playTask.getChannel());
    result += buf;
    result += ",";
    sprintf(buf ,"%d", playTask.getUserId());
    result += buf;
    result += ",";
    sprintf(buf ,"%d", playTask.getPlayHandle());
    result += buf;
    result += ",";
    sprintf(buf ,"%d", playTask.getDePort());
    result += buf;
    result += ",";
    sprintf(buf ,"%d", playTask.getStatus());
    result += buf;
    result += ",";
    sprintf(buf ,"%lldkb", playTask.getInputBytes());
    result += buf;
    result += ",";
    sprintf(buf ,"%lldkb/s", playTask.getInputBytes() / (time(NULL) - playTask.getStartTime()));
    result += buf;
    result += "<br>";
    it++;
  }
  return result;
}

  void DVRControl::taskHeartCheck() {
    std::lock_guard<std::mutex> guard(lock);
    long current = time(NULL);
    auto it = playTaskMap.begin();
    while (it != playTaskMap.end()) {
      PlayTask* task = &it->second;
      long time = task->getUpdateTime();
      if (current - time < TASK_TIMEOUT_SECONDS) {
        it++;
        continue;
      }
      
      if (task->getStatus() == PlayTaskStatus::START) {
        task->setStatus(PlayTaskStatus::STOP);
        LOG(INFO) << "task " <<  it->first << "timeout";
        it++;
      } else {
        LOG(INFO) << "task " <<  it->first << "stop";
        stopPlay(task->getPlayHandle(), task->getDePort(), task->getPlayType());
        playTaskMap.erase(it++);
      } 
   }
 }

 int DVRControl::getPlayDePort(int taskId) {
    std::lock_guard<std::mutex> guard(lock);
    auto it = playTaskMap.find(taskId);
    if (it != playTaskMap.end()) {
      return it->second.getDePort();
    }
    return -1;
 }

 PlayTask* DVRControl::getPlayTask(int taskId) {
    auto it = playTaskMap.find(taskId);
    if (it != playTaskMap.end()) {
      return &it->second;
    } else {
      return NULL;
    }
 }

  void DVRControl::setPlayHandle(int taskId, int handle) {
    std::lock_guard<std::mutex> guard(lock);
    auto it = playTaskMap.find(taskId);
    if (it != playTaskMap.end()) {
      it->second.setPlayHandle(handle);
    }
  }

 int DVRControl::heartBeat(int taskId) {
    std::lock_guard<std::mutex> guard(lock);
    PlayTask* task = getPlayTask(taskId);
    if (task && task->getStatus() == PlayTaskStatus::START) {
        long current = time(NULL);
        task->setUpdateTime(current);
        getLoginControl().heartBeat(task->getUserId());
        return 0;
    } else {
      return -1;
    }
  }

void heartBeatCheck() {
  while (true) {
    sleep(30);
    getDVRControl().taskHeartCheck();
    getLoginControl().userHeartCheck();
   
  }
}