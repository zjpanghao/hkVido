#ifndef SDK_API_H
#define SDK_API_H
#include <map>
#include <vector>
#include <string>
class SDKUser;
class DeviceInfo;
class PlayTask;
//#include "login.h"
#include "sdk_common.h"
class SdkApi {
  public:
    virtual int init() = 0;
    virtual int login(SDKUser *user, DeviceInfo *info) = 0;
    virtual int logout(int userId) = 0;
    virtual int playByTime(PlayTask *playTask) = 0;
    virtual int playReal(PlayTask *playTask) = 0;
    virtual int stopPlay(int handle, int port, PlayType type) = 0;
    virtual int playBackControl(int handle, int flag, long param) = 0;
    virtual int playGetPos(int handle, int *pos) = 0;
    virtual bool hasFilePlay(int lUserId, int channel, long startTime, long endTime) = 0;
    virtual long getTimeStamp(int nPort) = 0;
    virtual void getChannelIp(int userId, std::map<int, std::string> *channelIpMp) = 0;
};

SdkApi *getSdkApi(const std::string &key);
bool sdkInit(std::vector<SDKUser *> *userList);

#endif
