
#pragma once

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

// Structure pour représenter une commande GCode
struct MotionCommand {
  char type; // 'G' ou 'M'
  int code;  // ex. 1 pour G1, 104 pour M104
  float x, y, z, e, f, s; // Paramètres : X, Y, Z, E, F (vitesse), S (température/vitesse ventilateur)
  bool has_x, has_y, has_z, has_e, has_f, has_s; // Indicateurs de présence
};

// Énumération des codes de commande supportés
enum class GcodeType {
  G0 = 0, G1 = 1, G28 = 28, G90 = 90, G91 = 91, G21 = 21,
  M104 = 104, M109 = 109, M140 = 140, M190 = 190, M106 = 106, M107 = 107
};

class GcodeParser {
private:
  bool absolute_positioning; // G90 (true) ou G91 (false)
  bool parseParameters(String params, MotionCommand &cmd);
  bool parseMovementCommand(String params, MotionCommand &cmd, GcodeType code);
  bool parseHomingCommand(String params, MotionCommand &cmd);
  bool parsePositioningCommand(String params, MotionCommand &cmd, GcodeType code);
  bool parseTemperatureCommand(String params, MotionCommand &cmd, GcodeType code);
  bool parseFanCommand(String params, MotionCommand &cmd, GcodeType code);

public:
  GcodeParser() : absolute_positioning(true) {}
  void init();
  void testParse(String cmd);
  static void parserTask(void *pvParameters);
};

extern GcodeParser gcodeParser;
extern QueueHandle_t gcodeQueue;
extern QueueHandle_t motionQueue;
extern SemaphoreHandle_t errorSemaphore;
