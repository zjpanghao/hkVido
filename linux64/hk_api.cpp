#include "hk_api.h"
#include <memory>
#include <string>
#include <vector>
#include "HCNetSDK.h"
#include "play.h"
#include "play_task.h"
#include <glog/logging.h>
#include "sdk_common.h"
#include "login.h"
#include "json/json.h"

static std::string getTimeStr(const NET_DVR_TIME &netTime) {
  char buf[64];
  snprintf(buf, sizeof(buf), "%d-%d-%d %d:%d:%d\n", 
    netTime.dwYear, 
    netTime.dwMonth, 
    netTime.dwDay,
    netTime.dwHour,
    netTime.dwMinute,
    netTime.dwSecond);
  return buf;
}

void CALLBACK HkApi::DecCBFun(int nPort,
                          char * pBuf,
                          int nSize,
                          FRAME_INFO * pFrameInfo, 
                          void *puser,
                          int nReserved2) { 
    
    PlayTask *task = (PlayTask*)puser;
    if (task->getStatus() == PlayTaskStatus::STOP) {
      return;
    }
    SdkApi *api = task->getSdkApi();
    long lFrameType = pFrameInfo->nType;
    if (lFrameType ==T_AUDIO16) {
      
    } else if(lFrameType ==T_YV12) {	
      std::vector<unsigned char> inImage;
      if (nSize > 0) {
        if (task->getPlayType() == PlayType::PLAYBACK) {
          int pos = 0;
          api->playGetPos(task->getPlayHandle(), &pos);
          if (pos != -1) {
            task->setPos(pos);
          }  
        }
        TaskParam param_ = {task->getSdkApi(), nPort, task->getTaskId(), task->getTopic(),
        pBuf, nSize, pFrameInfo->nWidth, pFrameInfo->nHeight,
        api->getTimeStamp(nPort), task->getCameraId(), task->getCameraName(), task->getAreaName()};
		param_.inx = task->getWritePackIndex();
		task->setWritePackIndex(param_.inx + 1);
        std::unique_ptr<Runnable> decodeTask (new DecodeTask(param_));
        getPlayService(param_.taskId & 0xf)->Execute(std::move(decodeTask));
      }
     
    } else {
 
    }
}

void HkApi::hkDataCallBack(LONG handle, DWORD dataType, BYTE *buffer, DWORD size, void *puser) {
  int rc;
  PLAYM4_HWND hwnd = 0;
  PlayTask *task = (PlayTask*)puser;
  if (task->getStatus() == PlayTaskStatus::STOP) {
    return;
  }
  int nPort = task->getDePort();
  int maxCount = 3;
  switch (dataType) {
	  case NET_DVR_SYSHEAD: {  
      LOG(INFO) << "new data coming type" << dataType <<"   size   "  << size;
      if (nPort == -1) {
         if (!PlayM4_GetPort(&nPort)) {
          printf("PlayM4_GetPort error rc: %d\n", NET_DVR_GetLastError());
  			  break;
         } else {
            task->setPlayDePort(nPort);
            LOG(INFO) <<  "The nport is " <<  nPort << " " << task->getTaskId();
         }
		  } 
    
      if (nPort == -1) {
        return;
      }
  
      if (!PlayM4_SetStreamOpenMode(nPort, STREAME_REALTIME))  { 
          printf("PlayM4_SetStreamOpenMode Error\n");
          break; 
      } 
      if (!PlayM4_OpenStream(nPort, buffer, size, 1024*1024)) {
				rc = PlayM4_GetLastError(nPort);
        printf("open stream error rc: %d\n", rc);
				break;
			}
      //设置解码回调函数 只解码不显示
 			if (!PlayM4_SetDecCallBackMend(nPort, DecCBFun, puser)) {
        rc = PlayM4_GetLastError(nPort);
        printf("set callback error rc: %d\n", rc);
        break;
 			}
      if (!PlayM4_Play(nPort, hwnd)) {
        rc = PlayM4_GetLastError(nPort);
        printf("play error rc: %d\n", rc);
      }
      break;
    }
  	case NET_DVR_STREAMDATA:   {
    	if (size > 0 && nPort != -1) {  
    		bool inData = PlayM4_InputData(nPort, buffer, size);
    		while (!inData && maxCount-- > 0) {
    			sleep(1);
    			inData=PlayM4_InputData(nPort,buffer, size);
        }
        if (!inData) {
          LOG(ERROR) << "input data error handle:" << handle <<"error " << PlayM4_GetLastError(nPort);
        }
        task->addBytes(size / 1024);
    	}
  	  break;
    }
  }		
 
}

int HkApi::init() {
  NET_DVR_Init();
  NET_DVR_SetConnectTime(2000, 1);
  NET_DVR_SetReconnect(10000, true); 
  return 0;
}

int HkApi::login(SDKUser * user, DeviceInfo *info) {
  NET_DVR_USER_LOGIN_INFO struLoginInfo = {0};
	struLoginInfo.bUseAsynLogin = 0;
  strcpy(struLoginInfo.sDeviceAddress, user->nvrIp.c_str()); 
  struLoginInfo.wPort = user->port;
  strcpy(struLoginInfo.sUserName, user->userName.c_str()); 
  strcpy(struLoginInfo.sPassword, user->password.c_str()); 

  LONG lUserID = 0; 
  NET_DVR_DEVICEINFO_V40 struDeviceInfoV40 = {0};
  lUserID = NET_DVR_Login_V40(&struLoginInfo, &struDeviceInfoV40);
  if (lUserID < 0) {
    LOG(ERROR) << "Login failed, error code: " << NET_DVR_GetLastError();
    return -1;
  }
  info->type = struDeviceInfoV40.struDeviceV30.byDVRType;
  info->byChanNum = struDeviceInfoV40.struDeviceV30.byStartChan;
  info->byStartChan = struDeviceInfoV40.struDeviceV30.byStartChan;
  info->byIPChanNum = struDeviceInfoV40.struDeviceV30.byIPChanNum;
  info->byStartDChan = struDeviceInfoV40.struDeviceV30.byStartDChan;
  user->id = lUserID;
  return 0;
}

int HkApi::logout(int userId) {
  if (!NET_DVR_Logout(userId)) {
    return NET_DVR_GetLastError();
  }
  return 0;
}

int HkApi::stopPlay(int handle, int port, PlayType type) {
  int rc = 0;
  if (handle < 0) {
    return 0;
  }
  if (type == PlayType::PLAYBACK) {
    int idx = NET_DVR_GetPlayBackPlayerIndex(handle);
    if (idx >= 0) {
      PlayM4_SetDecCallBack(idx, NULL);
      NET_DVR_PlayBackControl(handle, NET_DVR_PLAYSTOPAUDIO, 0, NULL);
      if (!NET_DVR_StopPlayBack(handle)) {
        LOG(ERROR) << "stop playback  failed, error code: " << (rc = PlayM4_GetLastError(port));
        return rc;
      }
      LOG(INFO) << "hk stop playback handle: " << handle;
    }
  } else if (type == PlayType::PLAYREAL) {
    if (!NET_DVR_StopRealPlay(handle)) {
       LOG(ERROR) << "stop realplay  failed, error code:" << (rc = PlayM4_GetLastError(port));
       return rc;
    }
    LOG(INFO) << "hk stop play real handle :" << handle;
  }

  if (port != -1) {
    if (!PlayM4_Stop(port)) {
       LOG(ERROR) << "m4stop play error code" <<  (rc = PlayM4_GetLastError(port));
    }

    if (!PlayM4_CloseStream(port)) {
      LOG(ERROR) << "m4close stream error code" <<  (rc = PlayM4_GetLastError(port));
    }
    
    if (!PlayM4_FreePort(port)) {
       LOG(ERROR) << "m4close stream error code" <<  (rc = PlayM4_GetLastError(port));
    }
  }
 
  return rc;
}

int HkApi::playByTime(PlayTask *playTask) {
  struct tm start;
  struct tm end;
  int rc = 0;
  localtime_r(&playTask->getVide().startTime, &start);
  localtime_r(&playTask->getVide().endTime, &end);
  NET_DVR_TIME   dvrStartTime = {start.tm_year + 1900, 
      start.tm_mon + 1, start.tm_mday, 
      start.tm_hour, start.tm_min, start.tm_sec};
  NET_DVR_TIME   dvrEndTime = {end.tm_year + 1900, end.tm_mon + 1, end.tm_mday, 
      end.tm_hour, end.tm_min, start.tm_sec};
  printf("startTime  %s\n", getTimeStr(dvrStartTime).c_str());
  printf("endTime    %s\n", getTimeStr(dvrEndTime).c_str());
  LONG lid = NET_DVR_PlayBackByTime(playTask->getUserId(), playTask->getChannel(), &dvrStartTime, &dvrEndTime, NULL);

  if (lid == -1) {
    LOG(ERROR) << "get playbytime failed, error code:" << (rc = NET_DVR_GetLastError());
    return rc;
  }

  if (!NET_DVR_SetPlayDataCallBack_V40(lid, hkDataCallBack, (void*)playTask)) {
    LOG(ERROR) << "NET_DVR_SetPlayDataCallBack_V40 FAILED, error code:" << (rc = NET_DVR_GetLastError());
    return rc;
  }
  if (!NET_DVR_PlayBackControl_V40(lid, NET_DVR_PLAYSTART, NULL, 0, NULL, NULL)) {
    LOG(ERROR) << "NET_DVR_PLAYSTART  failed, error code: " << (rc=NET_DVR_GetLastError());
    NET_DVR_StopPlayBack(lid);
    return rc;
  }
  playTask->setPlayHandle(lid);
  playTask->setStatus(PlayTaskStatus::START);
  return 0;

}

int HkApi::playBackControl(int handle, int flag, long param) {
  switch (flag) {
    case NET_DVR_PLAYSETTIME: {
      struct tm start;
      localtime_r(&param, &start);
      NET_DVR_TIME   dvrStartTime = {start.tm_year + 1900, 
      start.tm_mon + 1, start.tm_mday, 
      start.tm_hour, start.tm_min, start.tm_sec};
      if (!NET_DVR_PlayBackControl_V40(handle, flag, &dvrStartTime, sizeof(dvrStartTime), NULL, NULL)) {
        printf("control  failed, error code: %d\n", NET_DVR_GetLastError());
        return NET_DVR_GetLastError();
      }
      break;
    }
    case NET_DVR_PLAYSLOW:
    case NET_DVR_PLAYNORMAL:
    case NET_DVR_PLAYFAST:
    case NET_DVR_PLAY_FORWARD:
    case NET_DVR_PLAY_REVERSE:{
      if (!NET_DVR_PlayBackControl_V40(handle, flag)) {
        LOG(ERROR) << "control  failed, error code" << NET_DVR_GetLastError();
        return NET_DVR_GetLastError();
      }
      break;
    }
    
  }
  return 0;
}

int HkApi::playReal(PlayTask *playTask) {
  
//启动预览并设置回调数据流
  LONG lid;
  int rc = 0;
  NET_DVR_PREVIEWINFO struPreviewInfo = {0};
  struPreviewInfo.lChannel = playTask->getChannel();
  struPreviewInfo.dwStreamType = playTask->getStreamType();
  struPreviewInfo.dwLinkMode = 0;
  struPreviewInfo.bBlocked = 1;
  struPreviewInfo.bPassbackRecord  = 1;
  lid = NET_DVR_RealPlay_V40(playTask->getUserId(), &struPreviewInfo, hkDataCallBack, (void*)playTask);
  if (lid < 0) {
    LOG(ERROR) << "get play real handle failed, error code: " << (rc = NET_DVR_GetLastError());
    return rc;
  } 
  playTask->setPlayHandle(lid);
  playTask->setStatus(PlayTaskStatus::START);
  return 0;
}

int HkApi::playGetPos(int handle, int *pos)  {
  unsigned int len;
  if (!NET_DVR_PlayBackControl_V40(handle, NET_DVR_PLAYGETPOS, NULL, 0, (void*)pos, &len)) {
    printf("play get pos  failed, error code: %d\n", NET_DVR_GetLastError());
    return NET_DVR_GetLastError();
  }
  return 0;
}

bool HkApi::hasFilePlay(int lUserId, int channel, long startTime, long endTime) {
  NET_DVR_FILECOND_V40 struFileCond={0};
  struct tm start;
  struct tm end;
  localtime_r(&startTime, &start);
  localtime_r(&endTime, &end);
  NET_DVR_TIME   dvrStartTime = {start.tm_year + 1900, 
      start.tm_mon + 1, start.tm_mday, 
      start.tm_hour, start.tm_min, start.tm_sec};
  NET_DVR_TIME   dvrEndTime = {end.tm_year + 1900, end.tm_mon + 1, end.tm_mday, 
      end.tm_hour, end.tm_min, start.tm_sec};
  struFileCond.dwFileType = 0; 
  struFileCond.lChannel = channel; 
  struFileCond.dwUseCardNo = 0; 
  struFileCond.struStartTime = dvrStartTime;
  struFileCond.struStopTime = dvrEndTime;
   //---------------------------------------
  //查找录像文件
  int lFindHandle = NET_DVR_FindFile_V40(lUserId, &struFileCond);
  if(lFindHandle < 0) {
      LOG(ERROR) << "find file fail,last error: " << NET_DVR_GetLastError();
      return false;
  }
  NET_DVR_FINDDATA_V40 struFileData;
  while(true) {
      int result = NET_DVR_FindNextFile_V40(lFindHandle, &struFileData);
      if(result == NET_DVR_ISFINDING) {
          continue;
      } else if(result == NET_DVR_FILE_SUCCESS) {
          return true;
      } else if(result == NET_DVR_FILE_NOFIND || result == NET_DVR_NOMOREFILE) {	
          break;
      } else {
        // printf("find file fail for illegal get file state");
        break;
      }
  }
  return false;
}

void HkApi::getChannelIp(int userId, std::map<int, std::string> *channelIpMp) {
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
                      //strcpy(channels[i + startChannel].ip, IPAccessCfgV40.struIPDevInfo[iDevInfoIndex].struIP.sIpV4);
                      //printf("IP channel no.%d is online, IP: %s\n", i+1, IPAccessCfgV40.struIPDevInfo[iDevInfoIndex].struIP.sIpV4);
                      (*channelIpMp)[i] = IPAccessCfgV40.struIPDevInfo[iDevInfoIndex].struIP.sIpV4;
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

long HkApi::getTimeStamp(int nPort) {
  PLAYM4_SYSTEM_TIME playm4SystemTime;
  PlayM4_GetSystemTime(nPort, &playm4SystemTime);
  struct tm  playTime = {playm4SystemTime.dwSec, 
                         playm4SystemTime.dwMin, 
                         playm4SystemTime.dwHour,
                         playm4SystemTime.dwDay,
                         playm4SystemTime.dwMon - 1,
                         playm4SystemTime.dwYear - 1900
                         };
  return mktime(&playTime);
  
}


static std::string getPlayTimeStr(const PLAYM4_SYSTEM_TIME &playm4SystemTime) {
   char buf[64];
  snprintf(buf, sizeof(buf), "%d-%d-%d %d:%d:%d\n", 
    playm4SystemTime.dwYear, 
    playm4SystemTime.dwMon, 
    playm4SystemTime.dwDay,
    playm4SystemTime.dwHour,
    playm4SystemTime.dwMin,
    playm4SystemTime.dwSec);
    return buf;

}

