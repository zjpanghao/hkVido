#include "play.h"
#include <stdio.h>
#include "public.h"
#include "string.h"
#include <stdlib.h>
#include <unistd.h>
#include "HCNetSDK.h"
#include"opencv2/core/core.hpp"
#include"opencv2/highgui/highgui.hpp"
#include"opencv2/imgproc/imgproc.hpp"

#include "base64.h"
#include <memory>
#include <string>
#include <vector>
#include <glog/logging.h>
#include "json/json.h"
#include "store.h"
#include "store_factory.h"
#include "PlayM4.h"
#include "dvr_control.h"

using namespace cv;
static ExecutorService *executorService;
void exeServiceInit() {
  executorService = Executors::NewFixPool(10);
}


bool YV12ToBGR24_OpenCV(unsigned char* pYUV,std::vector<unsigned char> *inImage,int width,int height) {
    if (width < 1 || height < 1 || pYUV == NULL)
        return false;
    Mat dst(height, width, CV_8UC3);
    Mat src(height + height/2, width, CV_8UC1, pYUV);
    cvtColor(src, dst, CV_YUV2BGR_YV12);
    imencode(".jpg", dst, *inImage);
    return true;
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

bool hasFilePlay(int lUserId, int channel, long startTime, long endTime) {
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

static std::string getSerial(int taskId, 
  const PLAYM4_SYSTEM_TIME &playm4SystemTime, 
  const std::string &data) {
  Json::Value root;
  //root["channel"] = channel;
  root["data"] = data;
  root["taskId"] = taskId;
  //root["type"] = (int)playType;
  root["time"] = getPlayTimeStr(playm4SystemTime);
  LOG(INFO) << getPlayTimeStr(playm4SystemTime);
  return root.toStyledString();
}

DecodeTask::DecodeTask(
  int port,  
  int taskId, 
  const std::string topic,
  const char *buf, 
  int size, 
  int width, 
  int height)
    : nPort(port),taskId_(taskId), topic_(topic), size_(size), pbuf_(NULL), width_(width), height_(height) {
  pbuf_ = (char*)malloc(size);
  memcpy(pbuf_, buf, size);
}

DecodeTask::~DecodeTask() {
  free(pbuf_);
}

void DecodeTask::Run() {
  std::vector<unsigned char> image;
  YV12ToBGR24_OpenCV((unsigned char*)pbuf_, &image, width_, height_);
  std::string imageBase64;
  if (image.empty()) {
    return;
  }
  base64Encry((char*)&image[0], image.size(), &imageBase64);
  PLAYM4_SYSTEM_TIME playm4SystemTime;
  PlayM4_GetSystemTime(nPort, &playm4SystemTime);
  // LOG(INFO) << channel;
  String res = getSerial(taskId_, playm4SystemTime, imageBase64);
  if (!topic_.empty()) {
    getGuardStore(0).Send(topic_, res, 0);
  }
}

std::string getTimeStr(const NET_DVR_TIME &netTime) {
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

void CALLBACK DecCBFun(int nPort,
                          char * pBuf,
                          int nSize,
                          FRAME_INFO * pFrameInfo, 
                          void *puser,
                          int nReserved2) { 
    //LOG(INFO) << "DBFUNC" << nPort;
    PlayTask *task = (PlayTask*)puser;
    if (task->getStatus() == PlayTaskStatus::STOP) {
      return;
    }
    
    long lFrameType = pFrameInfo->nType;
    if (lFrameType ==T_AUDIO16) {
        //printf("Audio nStamp:%d\n",pFrameInfo->nStamp);
    } else if(lFrameType ==T_YV12) {	
      // LOG(INFO) << "YV12FRame" << pFrameInfo->nWidth << " " << pFrameInfo->nHeight;
      std::vector<unsigned char> inImage;
      if (nSize > 0) {
        int pos;
        playGetPos(task->getPlayHandle(), &pos);
        if (pos != -1) {
          task->setPos(pos);
        }
        std::unique_ptr<Runnable> decodeTask (new DecodeTask(nPort, task->getTaskId(), task->getTopic(), pBuf, nSize, pFrameInfo->nWidth, pFrameInfo->nHeight));
        executorService->Execute(std::move(decodeTask));
      }
     
    } else {
 
    }
}

void hkDataCallBack(LONG handle, DWORD dataType, BYTE *buffer, DWORD size, void *puser) {
  int rc;
  PLAYM4_HWND hwnd = 0;
  PlayTask *task = (PlayTask*)puser;
  int status = task->getStatus();
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
          LOG(ERROR) << "input data error" <<  PlayM4_GetLastError(nPort);
        }
        task->addBytes(size / 1024);
    	}
  	  break;
    }
  }		
 
}

int stopPlay(int handle, int port, PlayType type) {
  if (type == PlayType::PLAYBACK) {
    if (!NET_DVR_StopPlayBack(handle)) {
      printf("control  failed, error code: %d\n", NET_DVR_GetLastError());
      return NET_DVR_GetLastError();
    }
  } else if (type == PlayType::PLAYREAL) {
    if (!NET_DVR_StopRealPlay(handle)) {
     printf("control  failed, error code: %d\n", NET_DVR_GetLastError());
      return NET_DVR_GetLastError();
    }
  }

  if (port != -1) {
    if (!PlayM4_Stop(port)) {
       printf("stop play error %d\n", PlayM4_GetLastError(port));
       return PlayM4_GetLastError(port);
    }
    
    if (!PlayM4_FreePort(port)) {
       return PlayM4_GetLastError(port);
    }
  }
  return 0;
}

int playGetPos(int handle, int *pos) {
    unsigned int len;
    if (!NET_DVR_PlayBackControl_V40(handle, NET_DVR_PLAYGETPOS, NULL, 0, (void*)pos, &len)) {
      printf("play get pos  failed, error code: %d\n", NET_DVR_GetLastError());
      return NET_DVR_GetLastError();
    }
    return 0;
}

int playBackSetStartTime(int handle) {
  NET_DVR_TIME dvrStartTime = {2018,8,16,10,0,0};
  if (!NET_DVR_PlayBackControl_V40(handle, NET_DVR_PLAYSETTIME, &dvrStartTime, sizeof(dvrStartTime), NULL, NULL)) {
    printf("update starttime failed, error code: %d\n", NET_DVR_GetLastError());
    return NET_DVR_GetLastError();
  }
  return 0;
}
  

int playBackControl(int handle, int flag, long param) {
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
  }
  return 0;
}

int playReal(PlayTask *playTask) {
  
//启动预览并设置回调数据流
  LONG lid;
  NET_DVR_PREVIEWINFO struPreviewInfo = {0};
  struPreviewInfo.lChannel = playTask->getChannel();
  struPreviewInfo.dwStreamType = 1;
  struPreviewInfo.dwLinkMode = 0;
  struPreviewInfo.bBlocked = 1;
  struPreviewInfo.bPassbackRecord  = 1;
  lid = NET_DVR_RealPlay_V40(playTask->getUserId(), &struPreviewInfo, hkDataCallBack, (void*)playTask);
  if (lid < 0) {
    printf("get play real handle failed, error code: %d\n", NET_DVR_GetLastError());
    return NET_DVR_GetLastError();
  } else {
     printf("get play real handle success\n");
  }
  playTask->setPlayHandle(lid);
  playTask->setStatus(PlayTaskStatus::START);
  return 0;
}


int playByTime(int lUserID, int channelId, NET_DVR_TIME *dvrStartTime, NET_DVR_TIME *dvrEndTime, PlayTask *playTask) {
  printf("startTime  %s\n", getTimeStr(*dvrStartTime).c_str());
  printf("endTime    %s\n", getTimeStr(*dvrEndTime).c_str());
  LONG lid = NET_DVR_PlayBackByTime(lUserID, channelId, dvrStartTime, dvrEndTime, NULL);

  if (lid == -1) {
    printf("get playbytime failed, error code: %d\n", NET_DVR_GetLastError());
    return NET_DVR_GetLastError();
  }

  playTask->setPlayHandle(lid);

  if (!NET_DVR_SetPlayDataCallBack_V40(lid, hkDataCallBack, (void*)playTask)) {
    printf("save data failed, error code: %d\n", NET_DVR_GetLastError());
    return NET_DVR_GetLastError();
  }
  if (!NET_DVR_PlayBackControl_V40(lid, NET_DVR_PLAYSTART, NULL, 0, NULL, NULL)) {
    printf("control  failed, error code: %d\n", NET_DVR_GetLastError());
    return NET_DVR_GetLastError();
  }
  playTask->setStatus(PlayTaskStatus::START);
  return 0;
}
