#include "play_task.h"
PlayTask::PlayTask() {
}

PlayTask::PlayTask(int taskId, int userId, int channel, PlayType type) {
  this->channel = channel;
  this->taskId = taskId;
  this->startTime = time(NULL);
  this->updateTime = time(NULL);
  this->vide.startTime = 0;
  this->vide.endTime = 0;
  this->userId = userId;
  this->inputByte = 0;
  this->playHandle = -1;
  this->playType = type;
  this->vide.dwStreamType = StreamType::CHILD_STREAM;
  dePort = -1;
  status = PlayTaskStatus::STOP;
  cameraName = " ";
  areaName = " ";
  topic="";
}

std::shared_ptr<TaskParam> PlayTask:: getPack() {
  if (timeout > 10) {
    LOG(INFO) << "time for " << readPackIndex;
    readPackIndex++;
  }
  std::lock_guard<std::mutex> lock(lock_);
  auto it = packMap_.find(readPackIndex);
  if (it != packMap_.end()) {
    auto pack = it->second;
    packMap_.erase(it);
    readPackIndex++;
    timeout = 0;
    return pack;
  }
  timeout++;
  if (readPackIndex % 1000 == 0) {
    auto it = packMap_.begin();
    while (it != packMap_.end()) {
      auto pack = it->second;
      if (pack->inx + 100 < readPackIndex) {
        packMap_.erase(it++);
      } else {
        it++;
      }
    }
  }
  return NULL;
}

void PlayTask::addPack(std::shared_ptr<TaskParam> pack) {
  std::lock_guard<std::mutex> lock(lock_);
  packMap_[pack->inx] = pack;
}


PlayTask::PlayTask(int taskId, int userId, int channel, PlayType type, long start, long end) :PlayTask(taskId, userId, channel, type) {
  this->vide.startTime = start;
  this->vide.endTime = end;
}
