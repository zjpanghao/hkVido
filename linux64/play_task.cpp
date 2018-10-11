#include "play_task.h"
PlayTask::PlayTask()    {
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

PlayTask::PlayTask(int taskId, int userId, int channel, PlayType type, long start, long end) :PlayTask(taskId, userId, channel, type) {
  this->vide.startTime = start;
  this->vide.endTime = end;
}