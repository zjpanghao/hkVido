#include "channel.h"
#include <stdio.h>
#include "string.h"
ChannelControl::ChannelControl() {
  for (int i = 0; i < MAX_CHANNUM_V40; i++) {
    channels[i].num = i;
    channels[i].recordState = true;
    channels[i].state = 0;
    strcpy(channels[i].ip, "");
  }
}

std::string ChannelControl::showChannel() {
  std::string s;
  char buf[64];
  for (int i = startChannel; i < startChannel + 16; i++) {
    snprintf(buf, sizeof(buf), "%d %d  %d<br>", channels[i].num, channels[i].state, channels[i].recordState);
    s += buf;
  }
  return s;
}



static BOOL  devStatcb(void* pUserdata, int iUserID, LPNET_DVR_WORKSTATE_V40 lpWorkState) {
  if (lpWorkState == NULL) {
    return false;
  }
  // printf("the state %d,%d\n", iUserID,lpWorkState->dwDeviceStatic);
  for (int i = 0; i < 16; i++) {
    NET_DVR_CHANNELSTATE_V30 *chanStat = &lpWorkState->struChanStatic[i];
    if (chanStat == NULL) {
      continue;
    }
    printf("channel %d  state %d record %d client %d \n", chanStat->dwChannelNo, 
      chanStat->byHardwareStatic, 
      chanStat->byRecordStatic,
      chanStat->dwLinkNum);
    ChannelInfo chan = {chanStat->dwChannelNo, chanStat->byHardwareStatic, chanStat->byRecordStatic};
    if (chan.num != -1) {
      getChannelControl().setChannelInfo(chanStat->dwChannelNo, chan); 
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

void channelInit() {
  getDevStat();
}

ChannelControl & getChannelControl() {
  static ChannelControl channelControl;
  return channelControl;
}

