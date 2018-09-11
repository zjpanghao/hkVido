#ifndef INCLUDE_LOGIN_H
#define INCLUDE_LOGIN_H
#include <map>
#include <mutex>
#include <string>
#include "sdk_common.h"
struct SDKUser {
  std::string userName;
  std::string password;
  std::string nvrIp;
  int port;
  std::string token;
  int id;
  long time;
};

class SdkApi;
class LoginControl {
  public:
    LoginControl() :
      islogin(false) {
    }
    int login(SDKUser *user);
    int logout(int userId);
    void userHeartCheck();
    void heartBeat(int userId);
    std::string showOnLineUser();
    bool getDevInfo(DeviceInfo *info);
    std::string getIp(int userId);
   
      
  private:
    bool islogin;
    DeviceInfo devInfo;
    std::map<int, long> userTimeMap;
    std::map<int, SDKUser> userMap;
    std::mutex lock;
};

LoginControl &getLoginControl();
#endif
