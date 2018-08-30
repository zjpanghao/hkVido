#ifndef INCLUDE_CHANNEL_H
#define INCLUDE_CHANNEL_H
#include "HCNetSDK.h"
#include <string>
struct ChannelStat {
  int num;
  int stat;
  bool recordStat;
};
struct ChannelInfo {
  ChannelStat channelStat;
  char ip[16];
};
class ChannelControl {
  public:
    ChannelControl();
    void setChannelIp(int userId);
    void getChannelInfo(int channel, ChannelInfo *info) {
      *info =  channels[channel];
    }

    void setChannelStat(int channel, const ChannelStat &info) {
      if (channel > 0 && channel < MAX_CHANNUM_V40) {
        channels[channel].channelStat = info;
      }
    }
    std::string showChannel();
  private:
    const static int startChannel = 33;
    ChannelInfo channels[MAX_CHANNUM_V40];
};



void channelInit();
ChannelControl & getChannelControl();
#endif
