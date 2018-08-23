#include "kafka_store.h"
bool KafkaStore::Send(const std :: string & message) {
  producer_->Send((char*)message.c_str(), message.length());
  return true;
}

