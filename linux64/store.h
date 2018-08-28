#ifndef INCLUDE_STORE_H
#define INCLUDE_STORE_H
#include <string>
class RedisPool;

class GuardStore {
 public:
  bool init();
  bool init(const std::string &ip, int port, int size, int max, const std::string &db, const std::string &password);
  virtual bool Send(const std::string &message);
  virtual bool Send(const std::string &topic, const std::string &message, int partition) = 0;
  
 private:
  RedisPool *pool;
};

#endif