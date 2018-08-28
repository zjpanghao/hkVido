#ifndef INCLUDE_KAFKA_STORE_H
#define INCLUDE_KAFKA_STORE_H
#include "store.h"
#include "kafka_producer.h"
class KafkaStore : public GuardStore {
  public:
    void setProducer(KafkaProducer *producer) {
      this->producer_ = producer;
    }
    virtual bool Send(const std::string &message);
    virtual bool Send(const std::string &topic, const std::string &message, int partition);
    
  private:
    KafkaProducer *producer_;
};
#endif
