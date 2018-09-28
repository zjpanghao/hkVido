#ifndef INCLUDE_CHANNEL_H
#define INCLUDE_CHANNEL_H
#include "HCNetSDK.h"
#include <map>
#include <string>
#include "sdk_common.h"
class SdkApi;
class DeviceInfo;
class SDKUser;
struct DeviceInfo;
struct ChannelStat {
  int num;
  int stat;
  bool recordStat;
};
struct ChannelInfo {
  ChannelStat channelStat;
  char ip[16];
};

class ChannelDVR {
  public:
    ChannelDVR();
    void init(int userId, std::string ip, const DeviceInfo &info);
    void getChannelInfo(int channel, ChannelInfo *info) {
      *info =  channels[channel];
    }

    void setChannelStat(int channel, const ChannelStat &info) {
      if (channel > 0 && channel < MAX_CHANNUM_V40) {
        channels[channel].channelStat = info;
      }
    }
    std::string showChannel();

    void setUser(const SDKUser &user);
   

    SDKUser *getUser() {
       return user;
    }

    void setFactoryType(FactoryType type) {
      this->factoryType = type;
    }

    FactoryType getFactoryType() {
      return factoryType;
    }

    DeviceInfo *getDevInfo() {
      return info_;
    }
    
  private:
    int startChannel;
    int nums;
    ChannelInfo channels[MAX_CHANNUM_V40];
    SDKUser *user;
    SdkApi *api;
    DeviceInfo *info_;
    FactoryType factoryType;
};

class ChannelControl {
  public:
    ChannelDVR *getDVR(std::string key) {
      auto it = dvrChannel.find(key);
      if (it == dvrChannel.end()) {
        return NULL;
      }
      return &it->second;
    }

    void setDVR(std::string key, ChannelDVR dvr) {
      dvrChannel[key] = dvr;
    }
    std::string showChannel();

    void add(SDKUser *user, FactoryType factoryType, const DeviceInfo &info);    
    
  private:
    std::map<std::string, ChannelDVR> dvrChannel; 
};

void channelInit();
ChannelControl & getChannelControl();
#endif
