#include "login.h"
#include "HCNetSDK.h"
#include "time.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <glog/logging.h>
LoginControl & getLoginControl() {
  static LoginControl loginControl;
  return loginControl;
}

int LoginControl::logout(int userId) {
  if (!NET_DVR_Logout(userId)) {
    return NET_DVR_GetLastError();
  }
  std::lock_guard<std::mutex> guard(lock);
  userMap.erase(userId);
}

void LoginControl::userHeartCheck() {
  std::lock_guard<std::mutex> guard(lock);
  long current = time(NULL);
  auto it = userMap.begin();
  while (it != userMap.end()) {
  long time = it->second;
  if (current - time > 1200) {
    LOG(INFO) << "user " <<  it->first << "timeout";
    NET_DVR_Logout(it->first);
    userMap.erase(it++);
  } else {
    it++;
  }
 }
}

int LoginControl::login(HKUser *user) {
  NET_DVR_USER_LOGIN_INFO struLoginInfo = {0};
	struLoginInfo.bUseAsynLogin = 0;
  strcpy(struLoginInfo.sDeviceAddress, user->nvrIp.c_str()); 
  struLoginInfo.wPort = user->port;
  strcpy(struLoginInfo.sUserName, user->userName.c_str()); 
  strcpy(struLoginInfo.sPassword, user->password.c_str()); 

  LONG lUserID = 0;

  NET_DVR_DEVICEINFO_V40 struDeviceInfoV40 = {0};

  lUserID = NET_DVR_Login_V40(&struLoginInfo, &struDeviceInfoV40);
  if (lUserID < 0) {
    printf("Login failed, error code: %d\n", NET_DVR_GetLastError());
    return NET_DVR_GetLastError();
  }
  user->id = lUserID;
  long now = time(NULL);
  int randNum = rand() % 1000000;
  char tmp[64];
  sprintf(tmp, "%u-%d", now, randNum);
  user->token = tmp;
  std::lock_guard<std::mutex> guard(lock);
  userMap[lUserID] = now;
  return 0;
}

std::string LoginControl::showOnLineUser() {
  std::lock_guard<std::mutex> guard(lock);
  std::string s;
  auto it = userMap.begin();
  char buf[64];
  while (it != userMap.end()) {
    snprintf(buf, sizeof(buf), "%d---%ld<br>", it->first, it->second);
    s += buf;
    it++;
  }
  return s;
}

void LoginControl::heartBeat(int userId) {
  std::lock_guard<std::mutex> guard(lock);
  userMap[userId] = time(NULL);
}