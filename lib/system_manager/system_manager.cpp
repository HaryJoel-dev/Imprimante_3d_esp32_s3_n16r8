
#include "system_manager.h"
#include "sd_manager.h"
#include "comm_manager.h"
#include "gcode_parser.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>
#include "../debug_manager.h"
#include <FastLED.h>

#define LED_PIN    48
#define NUM_LEDS   1
#define COLOR_ORDER GRB
#define STABILITY_DELAY 3000

QueueHandle_t gcodeQueue = NULL;
QueueHandle_t sdQueue = NULL;
QueueHandle_t motionQueue = NULL;
SemaphoreHandle_t errorSemaphore = NULL;
SystemManager systemManager;
CRGB leds[NUM_LEDS];

void SystemManager::stabilisation() {
  FastLED.addLeds<WS2812, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  unsigned long startTime = millis();
  const uint16_t breathingPeriod = 1200;
  while (millis() - startTime < STABILITY_DELAY) {
    float progress = (float)(millis() % breathingPeriod) / breathingPeriod;
    uint8_t brightness = 32 + 223 * (0.5f * (1 + sinf(progress * 2 * 3.14159f)));
    leds[0] = CRGB::Blue;
    leds[0].fadeLightBy(255 - brightness);
    FastLED.show();
    vTaskDelay(pdMS_TO_TICKS(20));
  }
  leds[0] = CRGB::Green;
  FastLED.show();
  vTaskDelay(pdMS_TO_TICKS(1000));
  leds[0] = CRGB::Black;
  FastLED.show();
  DEBUG_PRINTF_AUTO("Système stabilisé");
}

void SystemManager::systemTask(void *pvParameters) {
  while (1) {
    if (xSemaphoreTake(errorSemaphore, pdMS_TO_TICKS(1000)) == pdTRUE) {
      DEBUG_PRINTF_AUTO("Erreur détectée, arrêt d'urgence");
      leds[0] = CRGB::Red;
      FastLED.show();
      xQueueReset(sdQueue);
      xQueueReset(gcodeQueue);
    }
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

void SystemManager::init() {
  DEBUG_PRINTF_AUTO("Initialisation du System Manager");
  stabilisation();
  gcodeQueue = xQueueCreate(10, sizeof(String));
  sdQueue = xQueueCreate(5, sizeof(String));
  motionQueue = xQueueCreate(10, sizeof(MotionCommand));
  if (!gcodeQueue || !sdQueue || !motionQueue) {
    DEBUG_PRINTF_AUTO("Erreur: Impossible de créer les queues");
    if (errorSemaphore) xSemaphoreGive(errorSemaphore);
    return;
  }
  DEBUG_PRINTF_AUTO("Queues créées avec succès");
  errorSemaphore = xSemaphoreCreateBinary();
  if (!errorSemaphore) {
    DEBUG_PRINTF_AUTO("Erreur: Impossible de créer les sémaphores");
    return;
  }
  DEBUG_PRINTF_AUTO("Sémaphores créés avec succès");
  if (!sdManager.init()) {
    DEBUG_PRINTF_AUTO("Erreur: Échec init SDManager");
    if (errorSemaphore) xSemaphoreGive(errorSemaphore);
    return;
  }
  gcodeParser.init();
  commManager.init();
  xTaskCreatePinnedToCore(
    CommManager::commTask, "CommTask", 4096, NULL, 1, NULL, 1
  );
  xTaskCreatePinnedToCore(
    SDManager::sdTask, "SDTask", 4096, NULL, 1, NULL, 1
  );
  xTaskCreatePinnedToCore(
    GcodeParser::parserTask, "ParserTask", 4096, NULL, 3, NULL, 1
  );
  xTaskCreatePinnedToCore(
    systemTask, "SystemTask", 2048, NULL, 1, NULL, 1
  );
  delay(1000);
}

void SystemManager::testSystem() {
  DEBUG_PRINTF_AUTO("Test System Manager: Vérification des ressources");
  if (gcodeQueue && sdQueue && motionQueue && errorSemaphore) {
    DEBUG_PRINTF_AUTO("Toutes les ressources sont initialisées");
  } else {
    DEBUG_PRINTF_AUTO("Erreur: Certaines ressources non initialisées");
    if (errorSemaphore) xSemaphoreGive(errorSemaphore);
  }
}
