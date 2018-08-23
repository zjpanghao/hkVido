#include "store.h"
#include "redis_pool.h"
#include "glog/logging.h"
bool GuardStore::init() {
  pool = new RedisPool("192.168.1.113", 6379, 10, 20, "3", "7ifW4i@M");
  return true;
}

bool GuardStore::init(const std::string &ip, int port, int size, int max, const std::string &db, const std::string &password) {
  pool = new RedisPool(ip, port, size, max, db, password);
  return true;
}

bool GuardStore::Send(const std::string &message) {
  RedisControlGuard guard(pool);
  std::shared_ptr<RedisControl> control = guard.GetControl();
  if (!control) {
    LOG(INFO) << "Get redis error";
    return false;
  }
  LOG(INFO) << "Send mess";
  return control->Lpush("camera", message);
}

