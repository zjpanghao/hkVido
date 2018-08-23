#ifndef INCLUDE_LOGIN_H
#define INCLUDE_LOGIN_H
#include <map>
#include <mutex>
#include <string>
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
    int login(HKUser *user);
    int logout(int userId);
    void userHeartCheck();
    void heartBeat(int userId);
    std::string showOnLineUser();
  private:
    std::map<int, long> userMap;
    std::mutex lock;
};

LoginControl &getLoginControl();
#endif
