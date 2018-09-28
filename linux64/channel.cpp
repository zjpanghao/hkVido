#include "channel.h"
#include <stdio.h>
#include "string.h"
#include <glog/logging.h>
#include "login.h"
#include "sdk_api.h"

ChannelDVR::ChannelDVR()   {
  for (int i = 0; i < MAX_CHANNUM_V40; i++) {
    channels[i].channelStat.num = i;
     channels[i].channelStat.recordStat = true;
    channels[i].channelStat.stat = 0;
    strcpy(channels[i].ip, "");
  }
}

std::string ChannelDVR::showChannel() {
  std::string s;
  char buf[64];
  for (int i = startChannel; i < startChannel + nums; i++) {
    snprintf(buf, sizeof(buf), "%d %d  %d %s<br>", 
      channels[i].channelStat.num, 
      channels[i].channelStat.stat, 
      channels[i].channelStat.recordStat,
      channels[i].ip);
    s += buf;
  }
  return s;
}

static BOOL  devStatcb(void* pUserdata, int iUserID, LPNET_DVR_WORKSTATE_V40 lpWorkState) {
  if (lpWorkState == NULL) {
    return false;
  }
  LOG(INFO) << iUserID;
  //getChannelControl().setChannelIp(iUserID);
  // printf("the state %d,%d\n", iUserID,lpWorkState->dwDeviceStatic);
  for (int i = 0; i < 16; i++) {
    NET_DVR_CHANNELSTATE_V30 *chanStat = &lpWorkState->struChanStatic[i];
    if (chanStat == NULL) {
      continue;
    }
    printf("channel %d  state %d record %d client %d  \n", chanStat->dwChannelNo, 
      chanStat->byHardwareStatic, 
      chanStat->byRecordStatic,
      chanStat->dwLinkNum);
    ChannelStat chan = {chanStat->dwChannelNo, chanStat->byHardwareStatic, chanStat->byRecordStatic};
    if (chan.num != -1) {
      // getChannelControl().setChannelStat(chanStat->dwChannelNo, chan); 
    }
  }
}
static void getDevStat() {
  NET_DVR_CHECK_DEV_STATE *devStat = new NET_DVR_CHECK_DEV_STATE;
  devStat->dwTimeout = 0;
  devStat->pUserData = NULL;
  devStat->fnStateCB = devStatcb;
  NET_DVR_StartGetDevState(devStat);
}

void ChannelDVR::init(int userId, std::string ip, const DeviceInfo &info) {
  std::string key = ip;
  std::map<int, std::string> cmap;
  getSdkApi(ip)->getChannelIp(userId, &cmap);
  auto it = cmap.begin();
  while (it != cmap.end()) {
    strncpy(channels[it->first + info.byStartDChan].ip, it->second.c_str(), it->second.length() < 16 ? it->second.length():16);
    it++;
  }
  startChannel = info.byStartDChan;
  nums = info.byIPChanNum;
  info_ = new DeviceInfo(info);
}

std::string ChannelControl::showChannel() {
  auto it = dvrChannel.begin();
  std::string res;
  while (it != dvrChannel.end()) {
    res += it->second.showChannel();
    res += "-------------------\n";
    it++;
  }
  return res;
}

void ChannelDVR::setUser(const SDKUser &user) {
   this->user = new SDKUser(user);
}

void ChannelControl::add(SDKUser *user, FactoryType type, const DeviceInfo &info) {
   ChannelDVR dvr;
   dvr.init(user->id, user->nvrIp, info);
   dvr.setUser(*user);
   dvr.setFactoryType(type);
   getChannelControl().setDVR(user->nvrIp, dvr);
}

ChannelControl & getChannelControl() {
  static ChannelControl channelControl;
  return channelControl;
}

