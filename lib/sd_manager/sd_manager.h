
#pragma once

#include <Arduino.h>

class SDManager {
private:
public:
  bool init();
  void readFile(String filename);
  void testReadSD(String filename);
  void listFiles();
  static void sdTask(void *pvParameters);
};

extern SDManager sdManager;
