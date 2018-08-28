#include "kafka_store.h"
bool KafkaStore::Send(const std :: string & message) {
  producer_->Send((char*)message.c_str(), message.length());
  return true;
}

bool KafkaStore::Send(const std::string &topic, const std :: string & message, int partition) {
  producer_->Send(topic.c_str(), (char*)message.c_str(), message.length(), partition);
  return true;
}

