
#include "comm_manager.h"
#include "sd_manager.h"
#include "system_manager.h"
#include "gcode_parser.h"
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include "../debug_manager.h"
extern QueueHandle_t gcodeQueue;
extern QueueHandle_t sdQueue;

CommManager commManager;

void CommManager::commTask(void *pvParameters) {
  while (1) {
    if (Serial.available()) {
      String line = Serial.readStringUntil('\n');
      line.trim();
      if (line.isEmpty()) {
        DEBUG_PRINTF_AUTO("Commande série vide ignorée");
        continue;
      }
      DEBUG_PRINTF_AUTO("Commande série reçue: %s", line.c_str());
      if (line.startsWith("READ_SD ")) {
        String filename = line.substring(8);
        filename.trim();
        if (filename.isEmpty()) {
          DEBUG_PRINTF_AUTO("Erreur: Nom de fichier vide pour READ_SD");
          Serial.println("ERROR: Empty filename");
          continue;
        }
        sdManager.readFile(filename);
        Serial.println("OK: READ_SD command sent");
      } else if (line.startsWith("TEST_SD ")) {
        String filename = line.substring(8);
        filename.trim();
        if (filename.isEmpty()) {
          DEBUG_PRINTF_AUTO("Erreur: Nom de fichier vide pour TEST_SD");
          Serial.println("ERROR: Empty filename");
          continue;
        }
        sdManager.testReadSD(filename);
        Serial.println("OK: TEST_SD command sent");
      } else if (line.startsWith("TEST_SYSTEM")) {
        systemManager.testSystem();
        Serial.println("OK: TEST_SYSTEM command sent");
      } else if (line.startsWith("CLEAR_GCODE")) {
        xQueueReset(gcodeQueue);
        DEBUG_PRINTF_AUTO("gcodeQueue vidée via commande CLEAR_GCODE");
        Serial.println("OK: gcodeQueue cleared");
      } else if (line.startsWith("LIST_SD")) {
        sdManager.listFiles();
        DEBUG_PRINTF_AUTO("Commande LIST_SD exécutée");
      } else if (line.startsWith("TEST_PARSE ")) {
        String cmd = line.substring(11);
        cmd.trim();
        if (cmd.isEmpty()) {
          DEBUG_PRINTF_AUTO("Erreur: Commande vide pour TEST_PARSE");
          Serial.println("ERROR: Empty command");
          continue;
        }
        gcodeParser.testParse(cmd);
        Serial.println("OK: TEST_PARSE command sent");
      } else {
        DEBUG_PRINTF_AUTO("Commande non reconnue: %s", line.c_str());
        Serial.println("ERROR: Unknown command");
      }
    }
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

void CommManager::init() {
  Serial.begin(115200);
}

void CommManager::testComm(String cmd) {
  cmd.trim();
  DEBUG_PRINTF_AUTO("Test: Envoi commande %s", cmd.c_str());
  if (cmd.startsWith("READ_SD ")) {
    String filename = cmd.substring(8);
    filename.trim();
    sdManager.readFile(filename);
  } else if (cmd.startsWith("TEST_SD ")) {
    String filename = cmd.substring(8);
    filename.trim();
    sdManager.testReadSD(filename);
  } else if (cmd.startsWith("CLEAR_GCODE")) {
    xQueueReset(gcodeQueue);
    DEBUG_PRINTF_AUTO("Test: gcodeQueue vidée");
  } else if (cmd.startsWith("LIST_SD")) {
    sdManager.listFiles();
    DEBUG_PRINTF_AUTO("Test: Commande LIST_SD exécutée");
  } else if (cmd.startsWith("TEST_PARSE ")) {
    String cmd_str = cmd.substring(11);
    cmd_str.trim();
    gcodeParser.testParse(cmd_str);
  } else {
    DEBUG_PRINTF_AUTO("Test: Commande non gérée %s", cmd.c_str());
  }
}