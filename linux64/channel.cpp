#include "channel.h"
#include <stdio.h>
#include "string.h"
ChannelControl::ChannelControl() {
  for (int i = 0; i < MAX_CHANNUM_V40; i++) {
    channels[i].channelStat.num = i;
     channels[i].channelStat.recordStat = true;
    channels[i].channelStat.stat = 0;
    strcpy(channels[i].ip, "");
  }
}

std::string ChannelControl::showChannel() {
  std::string s;
  char buf[64];
  for (int i = startChannel; i < startChannel + 16; i++) {
    snprintf(buf, sizeof(buf), "%d %d  %d %s<br>", 
      channels[i].channelStat.num, 
      channels[i].channelStat.stat, 
      channels[i].channelStat.recordStat,
      channels[i].ip);
    s += buf;
  }
  return s;
}

void ChannelControl::setChannelIp(int userId) {
  NET_DVR_IPPARACFG_V40 IPAccessCfgV40;
  BYTE byIPID,byIPIDHigh;
  int iDevInfoIndex, iGroupNO, iIPCh;
  DWORD dwReturned = 0;
  memset(&IPAccessCfgV40, 0, sizeof(NET_DVR_IPPARACFG));
  iGroupNO=0;
  if (!NET_DVR_GetDVRConfig(userId, NET_DVR_GET_IPPARACFG_V40, 
    iGroupNO, &IPAccessCfgV40, sizeof(NET_DVR_IPPARACFG_V40), &dwReturned)) {
      printf("NET_DVR_GET_IPPARACFG_V40 error, %d\n", NET_DVR_GetLastError());
      return;
  } else {
      for (int i=0;i<IPAccessCfgV40.dwDChanNum;i++) {
              switch(IPAccessCfgV40.struStreamMode[i].byGetStreamType)
              {
              case 0:
                if (IPAccessCfgV40.struStreamMode[i].uGetStream.struChanInfo.byEnable)
                  {
                      byIPID=IPAccessCfgV40.struStreamMode[i].uGetStream.struChanInfo.byIPID;
                      byIPIDHigh=IPAccessCfgV40.struStreamMode[i].uGetStream.struChanInfo.byIPIDHigh;
                      iDevInfoIndex=byIPIDHigh*256 + byIPID-1-iGroupNO*64;
                      strcpy(channels[i + startChannel].ip, IPAccessCfgV40.struIPDevInfo[iDevInfoIndex].struIP.sIpV4);
                      printf("IP channel no.%d is online, IP: %s\n", i+1, IPAccessCfgV40.struIPDevInfo[iDevInfoIndex].struIP.sIpV4);
                  }
                  break;
              case 1:  
                  break;
              default:
                  break;
              }
      }

  }

}

static BOOL  devStatcb(void* pUserdata, int iUserID, LPNET_DVR_WORKSTATE_V40 lpWorkState) {
  if (lpWorkState == NULL) {
    return false;
  }
  getChannelControl().setChannelIp(iUserID);
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
    ChannelStat chan = {chanStat->dwChannelNo, chanStat->byHardwareStatic, chanStat->byRecordStatic};
    if (chan.num != -1) {
      getChannelControl().setChannelStat(chanStat->dwChannelNo, chan); 
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

