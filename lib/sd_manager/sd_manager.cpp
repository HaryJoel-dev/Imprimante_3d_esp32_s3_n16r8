
#include "sd_manager.h"
#include "../config.h"
#include <SdFat.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include "../debug_manager.h"
#include "system_manager.h"

extern QueueHandle_t sdQueue;
extern QueueHandle_t gcodeQueue;
extern SemaphoreHandle_t errorSemaphore;

SdFat SD;
SDManager sdManager;

void SDManager::sdTask(void *pvParameters) {
  String filename;
  while (1) {
    if (xQueueReceive(sdQueue, &filename, portMAX_DELAY) == pdTRUE) {
      xQueueReset(gcodeQueue);
      DEBUG_PRINTF_AUTO("gcodeQueue vidée avant lecture de %s", filename.c_str());

      File32 file = SD.open(filename.c_str(), FILE_READ);
      if (file) {
        DEBUG_PRINTF_AUTO("Lecture du fichier %s", filename.c_str());
        char buffer[512];
        while (file.available()) {
          int bytesRead = file.readBytesUntil('\n', buffer, sizeof(buffer) - 1);
          buffer[bytesRead] = '\0';
          String line = String(buffer);
          line.trim();
          if (line.isEmpty()) {
            DEBUG_PRINTF_AUTO("Debug: Ligne vide ignorée");
            continue;
          }
          if (line.startsWith(";")) {
            DEBUG_PRINTF_AUTO("Debug: Ignored line: '%s'", line.c_str());
            continue;
          }
          int commentPos = line.indexOf(';');
          if (commentPos != -1) {
            line = line.substring(0, commentPos);
            line.trim();
          }
          if (line.isEmpty()) {
            DEBUG_PRINTF_AUTO("Debug: Ligne vide après suppression du commentaire");
            continue;
          }
          DEBUG_PRINTF_AUTO("Debug: Envoi ligne à gcodeQueue: '%s'", line.c_str());
          if (xQueueSend(gcodeQueue, &line, pdMS_TO_TICKS(5000)) != pdTRUE) {
            DEBUG_PRINTF_AUTO("Erreur: Impossible d'envoyer à gcodeQueue après 5s");
            if (errorSemaphore) xSemaphoreGive(errorSemaphore);
            Serial.println("ERROR: Failed to send to gcodeQueue");
          }
          vTaskDelay(pdMS_TO_TICKS(10));
        }
        file.close();
        DEBUG_PRINTF_AUTO("Fin de lecture de %s", filename.c_str());
      } else {
        DEBUG_PRINTF_AUTO("Erreur: Impossible d'ouvrir %s", filename.c_str());
        if (errorSemaphore) xSemaphoreGive(errorSemaphore);
        Serial.println("ERROR: Failed to open file");
      }
    }
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

bool SDManager::init() {
  if (!SD.begin(CS_GPIO, SPI_HALF_SPEED)) {
    DEBUG_PRINTF_AUTO("Erreur: Initialisation SD échouée");
    if (errorSemaphore) xSemaphoreGive(errorSemaphore);
    Serial.println("ERROR: SD initialization failed");
    return false;
  }
  DEBUG_PRINTF_AUTO("Carte SD initialisée");
  return true;
}

void SDManager::readFile(String filename) {
  filename.trim();
  if (filename.isEmpty()) {
    DEBUG_PRINTF_AUTO("Erreur: Nom de fichier vide");
    Serial.println("ERROR: Empty filename");
    if (errorSemaphore) xSemaphoreGive(errorSemaphore);
    return;
  }
  if (xQueueSend(sdQueue, &filename, pdMS_TO_TICKS(100)) != pdTRUE) {
    DEBUG_PRINTF_AUTO("Erreur: Impossible d'envoyer filename à sdQueue");
    Serial.println("ERROR: Failed to send to sdQueue");
    if (errorSemaphore) xSemaphoreGive(errorSemaphore);
  }
}

void SDManager::testReadSD(String filename) {
  filename.trim();
  if (filename.isEmpty()) {
    DEBUG_PRINTF_AUTO("Test: Nom de fichier vide");
    Serial.println("ERROR: Empty filename");
    return;
  }
  File32 file = SD.open(filename.c_str(), FILE_READ);
  if (file) {
    DEBUG_PRINTF_AUTO("Test: Lecture de %s", filename.c_str());
    char buffer[512];
    while (file.available()) {
      int bytesRead = file.readBytesUntil('\n', buffer, sizeof(buffer) - 1);
      buffer[bytesRead] = '\0';
      String line = String(buffer);
      line.trim();
      if (line.isEmpty()) {
        DEBUG_PRINTF_AUTO("Debug: Ligne vide ignorée");
        continue;
      }
      if (line.startsWith(";")) {
        DEBUG_PRINTF_AUTO("Debug: Ignored line: '%s'", line.c_str());
        continue;
      }
      int commentPos = line.indexOf(';');
      if (commentPos != -1) {
        line = line.substring(0, commentPos);
        line.trim();
      }
      if (line.isEmpty()) {
        DEBUG_PRINTF_AUTO("Debug: Ligne vide après suppression du commentaire");
        continue;
      }
      DEBUG_PRINTF_AUTO("Ligne: %s", line.c_str());
    }
    file.close();
    DEBUG_PRINTF_AUTO("Test: Fin de lecture de %s", filename.c_str());
  } else {
    DEBUG_PRINTF_AUTO("Test: Erreur ouverture %s", filename.c_str());
    Serial.println("ERROR: Failed to open file");
  }
}

void SDManager::listFiles() {
  File32 dir = SD.open("/");
  if (!dir) {
    DEBUG_PRINTF_AUTO("Erreur: Impossible d'ouvrir le répertoire racine");
    Serial.println("ERROR: Failed to open root directory");
    if (errorSemaphore) xSemaphoreGive(errorSemaphore);
    return;
  }
  DEBUG_PRINTF_AUTO("Liste des fichiers sur la carte SD:");
  Serial.println("Files on SD card:");
  bool foundFiles = false;
  char fileName[256]; // Buffer pour stocker le nom du fichier
  File32 file;
  while (file.openNext(&dir, FILE_READ)) {
    if (!file.isDir()) {
      file.getName(fileName, sizeof(fileName));
      DEBUG_PRINTF_AUTO("Fichier: %s", fileName);
      Serial.print("File: ");
      Serial.println(fileName);
      foundFiles = true;
    }
    file.close();
  }
  dir.close();
  if (!foundFiles) {
    DEBUG_PRINTF_AUTO("Aucun fichier trouvé sur la carte SD");
    Serial.println("No files found on SD card");
  }
  Serial.println("OK: File list completed");
}