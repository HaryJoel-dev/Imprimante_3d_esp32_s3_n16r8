
#include "gcode_parser.h"
#include "../debug_manager.h"
#include "system_manager.h"

GcodeParser gcodeParser;

void GcodeParser::init() {
  DEBUG_PRINTF_AUTO("Initialisation du Gcode Parser");
  absolute_positioning = true; // G90 par défaut
}

void GcodeParser::testParse(String cmd) {
  cmd.trim();
  if (cmd.isEmpty()) {
    DEBUG_PRINTF_AUTO("Test: Commande vide ignorée");
    Serial.println("ERROR: Empty command");
    return;
  }
  DEBUG_PRINTF_AUTO("Test: Parsing commande '%s'", cmd.c_str());

  MotionCommand parsed_cmd = {0, 0, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, false, false, false, false, false, false};
  int space_pos = cmd.indexOf(' ');
  String params = (space_pos == -1) ? "" : cmd.substring(space_pos + 1);
  String code_str = cmd.substring(0, space_pos == -1 ? cmd.length() : space_pos);

  if (!code_str.startsWith("G") && !code_str.startsWith("M")) {
    DEBUG_PRINTF_AUTO("Test: Type de commande inconnu '%s'", code_str.c_str());
    Serial.println("ERROR: Invalid command type");
    return;
  }

  parsed_cmd.type = code_str.charAt(0);
  parsed_cmd.code = code_str.substring(1).toInt();

  bool valid = false;
  switch (static_cast<GcodeType>(parsed_cmd.code)) {
    case GcodeType::G0:
    case GcodeType::G1:
      valid = gcodeParser.parseMovementCommand(params, parsed_cmd, static_cast<GcodeType>(parsed_cmd.code));
      break;
    case GcodeType::G28:
      valid = gcodeParser.parseHomingCommand(params, parsed_cmd);
      break;
    case GcodeType::G90:
    case GcodeType::G91:
    case GcodeType::G21:
      valid = gcodeParser.parsePositioningCommand(params, parsed_cmd, static_cast<GcodeType>(parsed_cmd.code));
      break;
    case GcodeType::M104:
    case GcodeType::M109:
    case GcodeType::M140:
    case GcodeType::M190:
      valid = gcodeParser.parseTemperatureCommand(params, parsed_cmd, static_cast<GcodeType>(parsed_cmd.code));
      break;
    case GcodeType::M106:
    case GcodeType::M107:
      valid = gcodeParser.parseFanCommand(params, parsed_cmd, static_cast<GcodeType>(parsed_cmd.code));
      break;
    default:
      DEBUG_PRINTF_AUTO("Test: Code %c%d non supporté", parsed_cmd.type, parsed_cmd.code);
      Serial.println("ERROR: Unsupported command");
      return;
  }

  if (valid) {
    DEBUG_PRINTF_AUTO("Test: Commande valide, type=%c, code=%d", parsed_cmd.type, parsed_cmd.code);
    Serial.println("OK: Command parsed");
  } else {
    DEBUG_PRINTF_AUTO("Test: Erreur parsing '%s'", cmd.c_str());
    Serial.println("ERROR: Invalid command");
  }
}

bool GcodeParser::parseParameters(String params, MotionCommand &cmd) {
  cmd.has_x = cmd.has_y = cmd.has_z = cmd.has_e = cmd.has_f = cmd.has_s = false;
  params.trim();

  while (!params.isEmpty()) {
    int next_space = params.indexOf(' ');
    String param = (next_space == -1) ? params : params.substring(0, next_space);
    if (param.length() < 2) {
      DEBUG_PRINTF_AUTO("Erreur: Paramètre invalide '%s'", param.c_str());
      return false;
    }
    char param_type = param.charAt(0);
    float value = param.substring(1).toFloat();
    switch (param_type) {
      case 'X': cmd.x = value; cmd.has_x = true; break;
      case 'Y': cmd.y = value; cmd.has_y = true; break;
      case 'Z': cmd.z = value; cmd.has_z = true; break;
      case 'E': cmd.e = value; cmd.has_e = true; break;
      case 'F': cmd.f = value / 60.0; cmd.has_f = true; break; // Convertir mm/min en mm/s
      case 'S': cmd.s = value; cmd.has_s = true; break;
      default:
        DEBUG_PRINTF_AUTO("Erreur: Paramètre inconnu '%c'", param_type);
        return false;
    }
    params = (next_space == -1) ? "" : params.substring(next_space + 1);
    params.trim();
  }
  return true;
}

bool GcodeParser::parseMovementCommand(String params, MotionCommand &cmd, GcodeType code) {
  cmd.type = 'G';
  cmd.code = static_cast<int>(code);
  if (!parseParameters(params, cmd)) return false;
  if (!cmd.has_x && !cmd.has_y && !cmd.has_z && !cmd.has_e) {
    DEBUG_PRINTF_AUTO("Erreur: G%d sans paramètres X, Y, Z, ou E", cmd.code);
    return false;
  }
  return true;
}

bool GcodeParser::parseHomingCommand(String params, MotionCommand &cmd) {
  cmd.type = 'G';
  cmd.code = static_cast<int>(GcodeType::G28);
  if (params.isEmpty()) return true; // G28 sans paramètres = homing tous axes
  if (!parseParameters(params, cmd)) return false;
  if (!(cmd.has_x || cmd.has_y || cmd.has_z)) {
    DEBUG_PRINTF_AUTO("Erreur: G28 avec paramètres invalides");
    return false;
  }
  return true;
}

bool GcodeParser::parsePositioningCommand(String params, MotionCommand &cmd, GcodeType code) {
  cmd.type = 'G';
  cmd.code = static_cast<int>(code);
  if (!params.isEmpty()) {
    if (!parseParameters(params, cmd)) return false;
    if (cmd.has_x || cmd.has_y || cmd.has_z || cmd.has_e || cmd.has_f || cmd.has_s) {
      DEBUG_PRINTF_AUTO("Erreur: G%d ne doit pas avoir de paramètres", cmd.code);
      return false;
    }
  }
  if (code == GcodeType::G90) absolute_positioning = true;
  if (code == GcodeType::G91) absolute_positioning = false;
  return true;
}

bool GcodeParser::parseTemperatureCommand(String params, MotionCommand &cmd, GcodeType code) {
  cmd.type = 'M';
  cmd.code = static_cast<int>(code);
  if (!parseParameters(params, cmd)) return false;
  if (!cmd.has_s) {
    DEBUG_PRINTF_AUTO("Erreur: M%d nécessite un paramètre S", cmd.code);
    return false;
  }
  if (cmd.has_x || cmd.has_y || cmd.has_z || cmd.has_e || cmd.has_f) {
    DEBUG_PRINTF_AUTO("Erreur: M%d ne doit pas avoir de paramètres X, Y, Z, E, ou F", cmd.code);
    return false;
  }
  return true;
}

bool GcodeParser::parseFanCommand(String params, MotionCommand &cmd, GcodeType code) {
  cmd.type = 'M';
  cmd.code = static_cast<int>(code);
  if (code == GcodeType::M106) {
    if (!parseParameters(params, cmd)) return false;
    if (!cmd.has_s) {
      DEBUG_PRINTF_AUTO("Erreur: M106 nécessite un paramètre S");
      return false;
    }
  } else if (code == GcodeType::M107) {
    if (!params.isEmpty()) {
      if (!parseParameters(params, cmd)) return false;
      if (cmd.has_s) {
        DEBUG_PRINTF_AUTO("Erreur: M107 ne doit pas avoir de paramètre S");
        return false;
      }
    }
  }
  if (cmd.has_x || cmd.has_y || cmd.has_z || cmd.has_e || cmd.has_f) {
    DEBUG_PRINTF_AUTO("Erreur: M%d ne doit pas avoir de paramètres X, Y, Z, E, ou F", cmd.code);
    return false;
  }
  return true;
}

void GcodeParser::parserTask(void *pvParameters) {
  String line;
  while (1) {
    if (xQueueReceive(gcodeQueue, &line, portMAX_DELAY) == pdTRUE) {
      DEBUG_PRINTF_AUTO("Parsing ligne: '%s'", line.c_str());
      MotionCommand cmd = {0, 0, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, false, false, false, false, false, false};
      int space_pos = line.indexOf(' ');
      String params = (space_pos == -1) ? "" : line.substring(space_pos + 1);
      String code_str = line.substring(0, space_pos == -1 ? line.length() : space_pos);

      if (!code_str.startsWith("G") && !code_str.startsWith("M")) {
        DEBUG_PRINTF_AUTO("Erreur: Type de commande inconnu '%s'", code_str.c_str());
        if (errorSemaphore) xSemaphoreGive(errorSemaphore);
        Serial.println("ERROR: Invalid command type");
        continue;
      }

      cmd.type = code_str.charAt(0);
      cmd.code = code_str.substring(1).toInt();

      bool valid = false;
      switch (static_cast<GcodeType>(cmd.code)) {
        case GcodeType::G0:
        case GcodeType::G1:
          valid = gcodeParser.parseMovementCommand(params, cmd, static_cast<GcodeType>(cmd.code));
          break;
        case GcodeType::G28:
          valid = gcodeParser.parseHomingCommand(params, cmd);
          break;
        case GcodeType::G90:
        case GcodeType::G91:
        case GcodeType::G21:
          valid = gcodeParser.parsePositioningCommand(params, cmd, static_cast<GcodeType>(cmd.code));
          break;
        case GcodeType::M104:
        case GcodeType::M109:
        case GcodeType::M140:
        case GcodeType::M190:
          valid = gcodeParser.parseTemperatureCommand(params, cmd, static_cast<GcodeType>(cmd.code));
          break;
        case GcodeType::M106:
        case GcodeType::M107:
          valid = gcodeParser.parseFanCommand(params, cmd, static_cast<GcodeType>(cmd.code));
          break;
        default:
          DEBUG_PRINTF_AUTO("Erreur: Code %c%d non supporté", cmd.type, cmd.code);
          if (errorSemaphore) xSemaphoreGive(errorSemaphore);
          Serial.println("ERROR: Unsupported command");
          continue;
      }

      if (valid) {
        if (xQueueSend(motionQueue, &cmd, pdMS_TO_TICKS(5000)) != pdTRUE) {
          DEBUG_PRINTF_AUTO("Erreur: Impossible d'envoyer à motionQueue après 5s");
          if (errorSemaphore) xSemaphoreGive(errorSemaphore);
          Serial.println("ERROR: Failed to send to motionQueue");
        } else {
          DEBUG_PRINTF_AUTO("Commande envoyée à motionQueue: %c%d", cmd.type, cmd.code);
        }
      } else {
        DEBUG_PRINTF_AUTO("Erreur: Commande invalide '%s'", line.c_str());
        if (errorSemaphore) xSemaphoreGive(errorSemaphore);
        Serial.println("ERROR: Invalid command");
      }
    }
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}
