
#include <Arduino.h>
#include "system_manager.h"

void setup() {
    // Initialiser le système
    Serial.begin(115200);
    systemManager.init();
}

void loop() {
}