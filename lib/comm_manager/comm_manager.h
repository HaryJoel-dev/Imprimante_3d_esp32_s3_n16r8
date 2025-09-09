#pragma once

#include <Arduino.h>

class CommManager {
private:
public:
    void init();
    void testComm(String cmd);
    static void commTask(void *pvParameters);
};

extern CommManager commManager;
extern QueueHandle_t sdQueue;  // Déjà extern dans system_manager