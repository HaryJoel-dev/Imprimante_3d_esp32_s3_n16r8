#pragma once
#define DEBUG 1
#if DEBUG
#define __ORIGIN_FILENAME__ (strrchr("/" __FILE__, '/') + 1)
#define DEBUG_PRINT(x) Serial.println(x)
#define DEBUG_PRINTF(x, ...) Serial.printf(x, ##__VA_ARGS__)
  #define DEBUG_PRINT_AUTO(msg)  Serial.printf("[%s] -> %s\n", __ORIGIN_FILENAME__, msg)
  #define DEBUG_PRINTF_AUTO(fmt, ...) Serial.printf("[%s] -> " fmt "\n", __ORIGIN_FILENAME__, ##__VA_ARGS__)
#else
#define DEBUG_PRINT(x)
#define DEBUG_PRINTF(x, ...)
  #define DEBUG_PRINT_AUTO(msg)
  #define DEBUG_PRINTF_AUTO(fmt, ...)
#endif