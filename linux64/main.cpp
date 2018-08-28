#include <stdio.h>
#include "public.h"
#include "string.h"
#include <stdlib.h>
#include <unistd.h>
#include "time.h"
#include <string>
#include <vector>
#include <thread>
#include <glog/logging.h>
#include "play.h"
#include "dvr_control.h"
#include "store.h"
#include "kafka_store.h"
#include "store_factory.h"
#include "config.h"
#include "channel.h"
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>

int ev_server_start(int);
void showIpChannelInfo(int lUserID) {
  NET_DVR_IPPARACFG_V40 IPAccessCfgV40;
    BYTE byIPID,byIPIDHigh;
  int iDevInfoIndex, iGroupNO, iIPCh;
  DWORD dwReturned = 0;
    memset(&IPAccessCfgV40, 0, sizeof(NET_DVR_IPPARACFG));
    iGroupNO=0;
    if (!NET_DVR_GetDVRConfig(lUserID, NET_DVR_GET_IPPARACFG_V40, iGroupNO, &IPAccessCfgV40, sizeof(NET_DVR_IPPARACFG_V40), &dwReturned))
    {
        printf("NET_DVR_GET_IPPARACFG_V40 error, %d\n", NET_DVR_GetLastError());
        NET_DVR_Logout(lUserID);
        NET_DVR_Cleanup();
        return;
    }
      else
      {
          for (int i=0;i<IPAccessCfgV40.dwDChanNum;i++)
          {
              switch(IPAccessCfgV40.struStreamMode[i].byGetStreamType)
              {
              case 0:
                if (IPAccessCfgV40.struStreamMode[i].uGetStream.struChanInfo.byEnable)
                  {
                      byIPID=IPAccessCfgV40.struStreamMode[i].uGetStream.struChanInfo.byIPID;
                      byIPIDHigh=IPAccessCfgV40.struStreamMode[i].uGetStream.struChanInfo.byIPIDHigh;
                      iDevInfoIndex=byIPIDHigh*256 + byIPID-1-iGroupNO*64;
                      printf("IP channel no.%d is online, IP: %s\n", i+1, IPAccessCfgV40.struIPDevInfo[iDevInfoIndex].struIP.sIpV4);
                  }
                  break;
              case 1:  
                  if (IPAccessCfgV40.struStreamMode[i].uGetStream.struPUStream.struStreamMediaSvrCfg.byValid)
                  {
                      printf("IP channel %d connected with the IP device by stream server.\n", i+1);
                      printf("IP of stream server: %s, IP of IP device: %s\n",IPAccessCfgV40.struStreamMode[i].uGetStream.\
                      struPUStream.struStreamMediaSvrCfg.struDevIP.sIpV4, IPAccessCfgV40.struStreamMode[i].uGetStream.\
                      struPUStream.struDevChanInfo.struIP.sIpV4);
                  }
                  break;
              default:
                  break;
              }
          }
      }
}

void showIpProc(int lUserID) {
  NET_DVR_IPC_PROTO_LIST m_struProtoList;
  if (!NET_DVR_GetIPCProtoList(lUserID, &m_struProtoList)) {
      printf("NET_DVR_GetIPCProtoList error, %d\n", NET_DVR_GetLastError());
      return;
  }
  for (int i = 0; i < 10; i++) {
    printf("%s\n", m_struProtoList.struProto[i].byDescribe);
  }
}

void showFiles(int lUserId, int startD) {
  NET_DVR_FILECOND_V40 struFileCond={0};
  struFileCond.dwFileType = 0; 
  struFileCond.lChannel = startD; 
  struFileCond.dwUseCardNo = 0; 
  struFileCond.struStartTime.dwYear = 2018; 
  struFileCond.struStartTime.dwMonth = 8;
  struFileCond.struStartTime.dwDay = 8;
  struFileCond.struStartTime.dwHour = 0;
  struFileCond.struStartTime.dwMinute = 6;
  struFileCond.struStartTime.dwSecond =50; 
  struFileCond.struStopTime.dwYear = 2018; 
  struFileCond.struStopTime.dwMonth = 8;
  struFileCond.struStopTime.dwDay = 8;
  struFileCond.struStopTime.dwHour = 12;
  struFileCond.struStopTime.dwMinute = 7;
  struFileCond.struStopTime.dwSecond = 0;
   //---------------------------------------
  //查找录像文件
  int lFindHandle = NET_DVR_FindFile_V40(lUserId, &struFileCond);
  if(lFindHandle < 0)
  {
      printf("find file fail,last error %d\n",NET_DVR_GetLastError());
      return;
  }
  NET_DVR_FINDDATA_V40 struFileData;
  while(true)
  {
      int result = NET_DVR_FindNextFile_V40(lFindHandle, &struFileData);
      if(result == NET_DVR_ISFINDING)
      {
          continue;
      }
      else if(result == NET_DVR_FILE_SUCCESS)
      {
          char strFileName[256] = {0};
          sprintf(strFileName, "./%s", struFileData.sFileName);
 
          printf("find file %s\n", strFileName);
          continue;
      }
    else if(result == NET_DVR_FILE_NOFIND || result == NET_DVR_NOMOREFILE)
      {	
          break;
      }
    else
    {
       // printf("find file fail for illegal get file state");
        break;
    }
  }

  //停止查找
  if(lFindHandle >= 0)
  {
      NET_DVR_FindClose_V30(lFindHandle);
  }
}

void queryFiles(int lUserId, int channel, std::vector<std::string> *files) {
  NET_DVR_FILECOND_V40 struFileCond={0};
  struFileCond.dwFileType = 0; 
  struFileCond.lChannel = channel; 
  struFileCond.dwUseCardNo = 0; 
  struFileCond.struStartTime.dwYear = 2018; 
  struFileCond.struStartTime.dwMonth = 8;
  struFileCond.struStartTime.dwDay = 9;
  struFileCond.struStartTime.dwHour = 10;
  struFileCond.struStartTime.dwMinute = 6;
  struFileCond.struStartTime.dwSecond =50; 
  struFileCond.struStopTime.dwYear = 2018; 
  struFileCond.struStopTime.dwMonth = 8;
  struFileCond.struStopTime.dwDay = 9;
  struFileCond.struStopTime.dwHour = 13;
  struFileCond.struStopTime.dwMinute = 7;
  struFileCond.struStopTime.dwSecond = 0;
   //---------------------------------------
  //查找录像文件
  int lFindHandle = NET_DVR_FindFile_V40(lUserId, &struFileCond);
  if(lFindHandle < 0)
  {
      printf("find file fail,last error %d\n",NET_DVR_GetLastError());
      return;
  }
  NET_DVR_FINDDATA_V40 struFileData;
  while(true)
  {
      int result = NET_DVR_FindNextFile_V40(lFindHandle, &struFileData);
      if(result == NET_DVR_ISFINDING)
      {
          continue;
      }
      else if(result == NET_DVR_FILE_SUCCESS)
      {
          char strFileName[256] = {0};
          sprintf(strFileName, "./%s", struFileData.sFileName);
          files->push_back(struFileData.sFileName);
          continue;
      }
    else if(result == NET_DVR_FILE_NOFIND || result == NET_DVR_NOMOREFILE)
      {	
          break;
      }
    else
    {
        // printf("find file fail for illegal get file state");
        break;
    }
}
}


static void initGlog(const std::string &name) {
  DIR *dir = opendir("log");
  if (!dir) {
    mkdir("log", S_IRWXU);
  } else {
    closedir(dir);
  }
  google::InitGoogleLogging(name.c_str());
  google::SetLogDestination(google::INFO,std::string("log/"+ name + "info").c_str());
  google::SetLogDestination(google::WARNING,std::string("log/"+ name + "warn").c_str());
  google::SetLogDestination(google::GLOG_ERROR,std::string("log/"+ name + "error").c_str());
  FLAGS_logbufsecs = 10;
}


int main() {
  initGlog("guard");
  kunyan::Config config("../guard.conf");
  #if 0
  std::string ip = config.get("redis", "ip");
  int port = atoi(config.get("redis", "port").c_str());
  std::string db = config.get("redis", "db");
  std::string password = config.get("redis", "password");
  int size = atoi(config.get("redis", "size").c_str());
  int max = atoi(config.get("redis", "max").c_str());
  #endif
  std::string broker = config.get("kafka", "broker");
  std::string group = config.get("kafka", "group");
  std::string topic = config.get("kafka", "topic");
  KafkaStore &kafkaStore = (KafkaStore&)getGuardStore(0);

  KafkaProducer *kafkaProducer = new KafkaProducer();
  kafkaProducer->Init(broker, topic, group);
  kafkaStore.setProducer(kafkaProducer);
  #if 0
  kafkaStore.Send("rtsp://admin:ky221data@192.168.2.2:554/cam/realmonitor?channel=1&subtype=0");
  while (true) {
    sleep(1);
  }
  #endif
  int httpPort = atoi(config.get("http", "port").c_str());
  exeServiceInit();
  LOG(INFO) << "start";
  srand(time(NULL));
	NET_DVR_Init();
  NET_DVR_SetConnectTime(2000, 1);
  NET_DVR_SetReconnect(10000, true); 
  #if 0
  NET_DVR_USER_LOGIN_INFO struLoginInfo = {0};
	struLoginInfo.bUseAsynLogin = 0;
  strcpy(struLoginInfo.sDeviceAddress, "192.168.2.3"); 
  struLoginInfo.wPort = 8000;
  strcpy(struLoginInfo.sUserName, "admin"); 
  strcpy(struLoginInfo.sPassword, "ky221data"); 

  LONG lUserID;

  NET_DVR_DEVICEINFO_V40 struDeviceInfoV40 = {0};

  lUserID = NET_DVR_Login_V40(&struLoginInfo, &struDeviceInfoV40);
  if (lUserID < 0) {
      printf("Login failed, error code: %d\n", NET_DVR_GetLastError());
      NET_DVR_Cleanup();
      return -1;
  }
 
  //printf("The max number of analog channels: %d\n",struDeviceInfoV40.struDeviceV30.byChanNum); 
  //printf("The max number of IP channels: %d\n", struDeviceInfoV40.struDeviceV30.byIPChanNum + struDeviceInfoV40.struDeviceV30.byHighDChanNum * 256);
  //int nDnum = struDeviceInfoV40.struDeviceV30.byIPChanNum + struDeviceInfoV40.struDeviceV30.byHighDChanNum * 256;
  int startD = struDeviceInfoV40.struDeviceV30.byStartDChan;
  printf("%d\n", startD);
  showIpChannelInfo(lUserID);
  // showIpProc(lUserID);
  // showFiles(lUserID, startD);
  
  return -1;
  PlayTask playTask(1, lUserID, startD);
  #if 1
  if (0 != playReal(&playTask)) {
     printf("play error userid %d channel %d\n", lUserID, startD + 4);
  }
  #else
  NET_DVR_TIME dvrStartTime = {2018, 8, 17, 10, 0, 0};
  NET_DVR_TIME dvrEndTime = {2018, 8, 17, 11, 0, 0};
  if (0 != playByTime(lUserID, startD + 4, &dvrStartTime, &dvrEndTime, &playTask)) {
    printf("play error userid %d channel %d\n", lUserID, startD + 4);
  }
  #endif
 // if (!NET_DVR_PlayBackSaveData(lid, saveName)) {
  //  printf("save data failed, error code: %d\n", NET_DVR_GetLastError());
  //  return -1;
 // }
 while(true) {
    sleep(1);
 }
#else
  channelInit();
  std::thread heartCheck(heartBeatCheck);
  heartCheck.detach();
  ev_server_start(httpPort);
#endif
  NET_DVR_Cleanup();
  return 0;
}
