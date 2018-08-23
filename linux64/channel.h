#ifndef INCLUDE_CHANNEL_H
#define INCLUDE_CHANNEL_H
#include "HCNetSDK.h"
#include <string>
struct ChannelInfo {
  int num;
  int state;
  bool recordState;
  char ip[16];
};
class ChannelControl {
  public:
    ChannelControl();
    void getChannelInfo(int channel, ChannelInfo *info) {
      *info =  channels[channel];
    }

    void setChannelInfo(int channel, const ChannelInfo &info) {
      if (channel > 0 && channel < MAX_CHANNUM_V40) {
        channels[channel] = info;
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
