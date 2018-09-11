#ifndef HK_API_H
#define HK_API_H
#include "sdk_api.h"
#include "HCNetSDK.h"
#include "PlayM4.h"
class HkApi : public SdkApi {
  public:
    virtual int init();
    virtual int login(SDKUser *user, DeviceInfo *info);
    virtual int logout(int userId);
    virtual int stopPlay(int handle, int port, PlayType type);
    virtual int playByTime(PlayTask *playTask);
    virtual int playBackControl(int handle, int flag, long param);
    virtual int playReal(PlayTask *playTask);
    virtual int playGetPos(int handle, int *pos);
    virtual bool hasFilePlay(int lUserId, int channel, long startTime, long endTime);
    static void hkDataCallBack(LONG handle, DWORD dataType, BYTE *buffer, DWORD size, void *puser);
    static void  DecCBFun(int nPort,
                          char * pBuf,
                          int nSize,
                          FRAME_INFO * pFrameInfo, 
                          void *puser,
                          int nReserved2);
};
#endif
