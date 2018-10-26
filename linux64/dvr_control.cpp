#include "dvr_control.h"
#include "unistd.h"
#include "login.h"
#include "play.h"
#include <glog/logging.h>
#include "channel.h"
#include "play_task.h"
#include "sdk_api.h"
#include "base64.h"
#include "json/json.h"
#include "store.h"
#include "store_factory.h"
DVRControl & getDVRControl() {
  static DVRControl dvrControl;
  return dvrControl;
}

  bool DVRControl::addTask(std::shared_ptr<PlayTask> playTask) {
   ChannelInfo info = {0};
   // getChannelControl().getChannelInfo(playTask->getChannel(), &info);
   if (info.channelStat.stat != 0) {
     return false;
   }
   //if (playTask->getPlayType() == PlayType::PLAYBACK && !info.channelStat.recordStat) {
   //  return false;
   //}
   std::lock_guard<std::mutex> guard(lock);
   auto it = playTaskMap.find(playTask->getTaskId());
   if (it != playTaskMap.end()) {
     return false;
   }
   playTaskMap[playTask->getTaskId()] = playTask;
   return true;
  }

int DVRControl::play(int taskId) {
  std::lock_guard<std::mutex> guard(lock);
  auto it = playTaskMap.find(taskId);
  if (it == playTaskMap.end()) {
    return -1;
  }
  std::shared_ptr<PlayTask> playTask = it->second;
  int rc = 0;
  SdkApi *api = getSdkApi(getLoginControl().getIp(playTask->getUserId()));
  playTask->setSdkApi(api);
  if (!api) {
    return -1;
  }
  if (playTask->getPlayType() == PlayType::PLAYREAL) {
    rc = api->playReal(playTask.get());
  } else {
    rc = api->playByTime(playTask);
  }
  std::thread *tid = new std::thread(sendThd, playTask.get());
  tid->detach();
  playTask->setSendThdId(tid);
  return rc;
}

int DVRControl::getTaskStatus(int taskId) {
  std::lock_guard<std::mutex> guard(lock);
  std::shared_ptr<PlayTask> playTask = getPlayTask(taskId);
  if (playTask) {
    return playTask->getStatus();
  }
  return PlayTaskStatus::STOP;
}

int DVRControl::stopPlayTask(int taskId) {
  {
    std::lock_guard<std::mutex> guard(lock);
    auto playTask = getPlayTask(taskId);
    if (playTask) {
      if (playTask->getStatus() == PlayTaskStatus::STOP) {
          LOG(ERROR) << taskId << "alreay stop";
          return -1;
      }
      playTask->setStatus(PlayTaskStatus::STOP);
      sleep(1);
    } else {
      return -1;
    }
  }
  std::lock_guard<std::mutex> guard(lock);
  auto playTask = getPlayTask(taskId);
  if (playTask) {
    SdkApi *api = getSdkApi(getLoginControl().getIp(playTask->getUserId()));
    if (!api) {
      return -1;
    }
    LOG(INFO) << "stop task:"<< taskId;
    api->stopPlay(playTask->getPlayHandle(), playTask->getDePort(), playTask->getPlayType());
	#if 0
	std::thread *tid = playTask->getSendThdId();
	if (tid) {
		tid->join();
		delete tid;
		tid = NULL;
	}
	#endif
    // playTaskMap.erase(taskId);
  } else {
    return -1;
  }
  return 0;
}

int DVRControl::playControl(int taskId, int flag, long param) {
  std::lock_guard<std::mutex> guard(lock);
  auto playTask = getPlayTask(taskId);
  if (playTask) {
    SdkApi *api = getSdkApi(getLoginControl().getIp(playTask->getUserId()));
    if (!api) {
      return -1;
    }
    api->playBackControl(playTask->getPlayHandle(), flag, param);
  }
  return -1;
}

  int DVRControl::getPos(int taskId) {
    int pos = -1;
    std::lock_guard<std::mutex> guard(lock);
    auto  task = getPlayTask(taskId);
    if (task) {
      return task->getPos();
    }
    return pos;
  }

std::string DVRControl::getTaskInfo() {
  long now = time(NULL);
  std::lock_guard<std::mutex> guard(lock);
  auto it = playTaskMap.begin();
  std::string result = "taskId, channel, userId, handle, deport, status, pos,inputbytes(kb)  speed(kb/s) <br>";
  while (it != playTaskMap.end()) {
    PlayTask &playTask = *(it->second);
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
    sprintf(buf ,"%d", playTask.getPos());
    result += buf;
    result += ",";
    sprintf(buf ,"%lldkb", playTask.getInputBytes());
    result += buf;
    result += ",";
    sprintf(buf ,"%lldkb/s", playTask.getInputBytes() / (now - playTask.getStartTime()));
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
      std::shared_ptr<PlayTask> task = it->second;
      long time = task->getUpdateTime();
      if (current - time < TASK_TIMEOUT_SECONDS) {
        it++;
        continue;
      }
      
      if (task->getStatus() == PlayTaskStatus::START) {
        it++;
        continue;
      }   
      LOG(INFO) << "task " <<  it->first << "clear";
      // it++;
    
      playTaskMap.erase(it++);
    } 
 }

 std::shared_ptr<PlayTask> DVRControl::getPlayTask(int taskId) {
    auto it = playTaskMap.find(taskId);
    if (it != playTaskMap.end()) {
      return it->second;
    } else {
      return NULL;
    }
 }

 std::shared_ptr<PlayTask> DVRControl::getByTaskId(int taskId) {
	std::lock_guard<std::mutex> guard(lock);
	return getPlayTask(taskId);
 }

  void DVRControl::setPlayHandle(int taskId, int handle) {
    std::lock_guard<std::mutex> guard(lock);
    auto it = playTaskMap.find(taskId);
    if (it != playTaskMap.end()) {
      it->second->setPlayHandle(handle);
    }
  }

bool DVRControl::hasFilePlay(int lUserId, int channel, long startTime, long endTime) {
  std::string ip = getLoginControl().getIp(lUserId);
  SdkApi *api = getSdkApi(ip);
  if (!api) {
    return false;
  }
  return api->hasFilePlay(lUserId, channel, startTime, endTime);
}

 int DVRControl::heartBeat(int taskId) {
    std::lock_guard<std::mutex> guard(lock);
    std::shared_ptr<PlayTask> task = getPlayTask(taskId);
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

void DVRControl::sendThd(void *param) {
    sleep(2);
	PlayTask *task = (PlayTask *)param;
	std::shared_ptr<TaskParam> taskParam;
	LOG(INFO) << "task start" << task->getTaskId();
	int timeout = 0;
	while (task->getStatus() == PlayTaskStatus::START) {
		
		while (true) {
		  if (taskParam = task->getPack()) {
		  } else {
		  	timeout++;
			break;
		  }
		  std::string imageBase64;
		  if (taskParam->image.empty()) {
		    break;
		  }
		  LOG(INFO) << "get:" << taskParam->inx;
		  Base64::getBase64().encode(taskParam->image, imageBase64);
		  Json::Value root;
		  root["bufferedImageStr"] = imageBase64;
		  root["taskId"] = taskParam->task_->getTaskId();
		  root["stamp"] = (unsigned int)(taskParam->timestamp);
		  root["cameraId"] = taskParam->task_->getCameraId();
		  root["cameraName"] = taskParam->task_->getCameraName();
		  root["area"] = taskParam->task_->getAreaName();
		  std::string res = root.toStyledString();
		  if (!taskParam->task_->getTopic().empty()) {
		  	std::unique_ptr<MessageTask> messageTask(new MessageTask(taskParam->task_->getTopic(), res));
		    getStoreService(taskParam->task_->getTaskId() & 0xf)->Execute(std::move(messageTask));
		 }
	   } 
	   std::this_thread::sleep_for(std::chrono::milliseconds(1000/25));

   }
}