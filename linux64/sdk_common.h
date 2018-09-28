#ifndef INCLUDE_SDK_COMMON_H
#define INCLUDE_SDK_COMMON_H
enum PlayType {
  PLAYREAL,
  PLAYBACK
};

enum StreamType {
  MAIN_STREAM,
  CHILD_STREAM
};

enum FactoryType {
  HAIKANG,
  DAHUA
};

struct DeviceInfo {
  unsigned char type;
  unsigned char byChanNum;                    //模拟通道个数
  unsigned char byStartChan;
  unsigned char byIPChanNum;
  unsigned char byStartDChan;
};

enum PlayTaskStatus {
  START,
  STOP
};
  
#endif
