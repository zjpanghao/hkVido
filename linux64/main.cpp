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
#include "sdk_api.h"
#include "sdk_common.h"
#include "login.h"
int ev_server_start(int);

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
  FLAGS_logbufsecs = 0;
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
  SDKUser user = {"admin", 
                    "ky221data", 
                   "192.168.2.3", 
                   8000};
  SDKUser user2 = {"admin", 
                   "12345", 
                  "192.168.4.233", 
                  8000};
  std::vector<SDKUser*> userList = { &user2, &user};
  if (sdkInit(&userList) == false) {
    LOG(ERROR) << "SDK INIT ERROR";
    return -1;
  }
  
  // channelInit();
  std::thread heartCheck(heartBeatCheck);
  heartCheck.detach();
  ev_server_start(httpPort);
  return 0;
}
