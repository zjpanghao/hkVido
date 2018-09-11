#include "login.h"

#include "time.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <glog/logging.h>
#include "sdk_common.h"
#include "sdk_api.h"
LoginControl & getLoginControl() {
  static LoginControl loginControl;
  return loginControl;
}

int LoginControl::logout(int userId) {
  SdkApi *api = NULL;
  {
    std::lock_guard<std::mutex> guard(lock);
    auto it = userMap.find(userId);
    if (it == userMap.end()) {
      return -1;
    }
    std::string &ip = it->second.nvrIp;
    api = getSdkApi(ip);
    if (!api) {
      return -1;
    }
  }
  api->logout(userId);
  std::lock_guard<std::mutex> guard(lock);
  userMap.erase(userId);
}

void LoginControl::userHeartCheck() {
  std::lock_guard<std::mutex> guard(lock);
  long current = time(NULL);
  auto it = userMap.begin();
  while (it != userMap.end()) {
  long time = it->second.time;
  if (current - time > 3600 *24) {
    LOG(INFO) << "user " <<  it->first << "timeout";
    std::string &ip = it->second.nvrIp;
    SdkApi *api = getSdkApi(ip);
    if (!api) {
      return;
    }
    api->logout(it->first);
    userMap.erase(it++);
  } else {
    it++;
  }
 }
}

int LoginControl::login(SDKUser *user) {
  DeviceInfo info;
  SdkApi *api = getSdkApi(user->nvrIp);
  if (!api) {
    return -1;
  }
  int rc = 0;
  if ((rc = api->login(user, &info)) < 0) {
    return rc;
  }
  long now = time(NULL);
  int randNum = rand() % 1000000;
  char tmp[64];
  sprintf(tmp, "%u-%d", now, randNum);
  user->token = tmp;
  user->time = now;
  std::lock_guard<std::mutex> guard(lock);
  userMap[user->id] = *user;
  devInfo = info;
  islogin = true;
  return 0;
}

bool LoginControl::getDevInfo(DeviceInfo *info) {
  std::lock_guard<std::mutex> guard(lock);
  if (!islogin) {
    return false;
  }
  *info = devInfo;
  return true;
}

std::string LoginControl::showOnLineUser() {
  std::lock_guard<std::mutex> guard(lock);
  std::string s;
  auto it = userMap.begin();
  char buf[64];
  while (it != userMap.end()) {
    snprintf(buf, sizeof(buf), "%d---%ld<br>", it->first, it->second.time);
    s += buf;
    it++;
  }
  return s;
}

void LoginControl::heartBeat(int userId) {
  std::lock_guard<std::mutex> guard(lock);
  userMap[userId].time = time(NULL);
}

 std::string LoginControl::getIp(int userId) {
   std::lock_guard<std::mutex> guard(lock);
   auto it = userMap.find(userId);
   if (it == userMap.end()) {
     return "";
   }
   return it->second.nvrIp;
 }