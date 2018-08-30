#ifndef INCLUDE_LOGIN_H
#define INCLUDE_LOGIN_H
#include <map>
#include <mutex>
#include <string>
#include "HCNetSDK.h"
struct HKUser {
  std::string userName;
  std::string password;
  std::string nvrIp;
  int port;
  std::string token;
  int id;
};

class LoginControl {
  public:
    LoginControl() :
      islogin(false) {
    }
    int login(HKUser *user);
    int logout(int userId);
    void userHeartCheck();
    void heartBeat(int userId);
    std::string showOnLineUser();
    bool getDevInfo(NET_DVR_DEVICEINFO_V40 *struDeviceInfoV40);
   
      
  private:
    bool islogin;
    NET_DVR_DEVICEINFO_V40 devInfo;
    std::map<int, long> userMap;
    std::mutex lock;
};

LoginControl &getLoginControl();
#endif
