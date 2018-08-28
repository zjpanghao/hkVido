
#include "store_factory.h"
#include "kafka_store.h"
GuardStore& getGuardStore(int type) {
  static KafkaStore store;
  if (type == 0) {
    return store;
  }
  return store;
}

