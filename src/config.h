#pragma once
#include <pgmspace.h>

// WiFi credentials
const char PROGMEM *WIFI_SSID_1 = "";
const char PROGMEM *WIFI_PASSWORD_1 = "";
const char PROGMEM *WIFI_SSID_2 = "";
const char PROGMEM *WIFI_PASSWORD_2 = "";

// API endpoints and keys
const char PROGMEM AQI_REQUEST[] = "";
const char PROGMEM AQI_URL[] = "";
const char PROGMEM WEATHER_URL[] = "";

// Hardware configuration
#define BATTERY_PIN 35             // GPIO35 for battery voltage monitoring
#define BATTERY_READS 20           // Number of readings to average
#define VOLTAGE_DIVIDER_RATIO 2.21 // Voltage divider ratio if used (change according to your hardware)

// Sleep configuration
#define uS_TO_S_FACTOR 1000000ULL /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP 600         /* Time ESP32 will go to sleep (in seconds) */