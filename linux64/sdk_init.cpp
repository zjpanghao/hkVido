#include "sdk_api.h"
#include "hk_api.h"
#include "play.h"
#include <map>
#include <string>
#include "login.h"
#include "channel.h"
#include <glog/logging.h>
std::map<std::string, SdkApi*> *sdkApiMap;

static void attach(std::vector<SDKUser*> *userList, std::map<std::string, SdkApi*> *map) {
  HkApi *api = new HkApi();
  DeviceInfo  info;
  api->init();
  for (auto  user : *userList) {
     if (api->login(user, &info) >= 0) {
       (*sdkApiMap)[user->nvrIp] = api;
       LOG(INFO) << "attach " << user->nvrIp;
       // api->logout(user->id);
       getChannelControl().add(user, FactoryType::HAIKANG, info);
     } 
  }
}

bool sdkInit(std::vector<SDKUser*> *userList) {
  sdkApiMap = new std::map<std::string, SdkApi*>();
  attach(userList, sdkApiMap);
  return !sdkApiMap->empty();
}

SdkApi *getSdkApi(const std::string &key) {
  auto it = sdkApiMap->find(key);
  if (it == sdkApiMap->end()) {
    return NULL;
  }
  return it->second;
}