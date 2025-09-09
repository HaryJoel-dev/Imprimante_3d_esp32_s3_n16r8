
#pragma once

#include <Arduino.h>

class SystemManager {
private:
  static void systemTask(void *pvParameters);

public:
  void init();
  void testSystem();
  void stabilisation();
};

extern SystemManager systemManager;
extern QueueHandle_t gcodeQueue;
extern QueueHandle_t sdQueue;
extern QueueHandle_t motionQueue;
extern SemaphoreHandle_t errorSemaphore;
