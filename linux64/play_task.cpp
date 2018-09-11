#include "play_task.h"
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