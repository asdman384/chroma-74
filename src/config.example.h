#pragma once
#include <pgmspace.h>

// WiFi credentials
const char PROGMEM *WIFI_SSID_1 = "";
const char PROGMEM *WIFI_PASSWORD_1 = "";
const char PROGMEM *WIFI_SSID_2 = "";
const char PROGMEM *WIFI_PASSWORD_2 = "";

// API endpoints and keys
const char PROGMEM AQI_REQUEST[] = "{\"location\": {\"latitude\": ,\"longitude\": },\"extraComputations\": [\"POLLUTANT_CONCENTRATION\"]}";
const char PROGMEM AQI_URL[] = "https://airquality.googleapis.com/v1/currentConditions:lookup?key=";
const char PROGMEM WEATHER_URL[] = "https://weather.googleapis.com/v1/currentConditions:lookup?location.latitude=1&location.longitude=1&key=";

// Hardware configuration
#define BATTERY_PIN 35             // GPIO35 for battery voltage monitoring
#define BATTERY_READS 20           // Number of readings to average
#define VOLTAGE_DIVIDER_RATIO 2.21 // Voltage divider ratio if used (change according to your hardware)

// Sleep configuration
#define uS_TO_S_FACTOR 1000000ULL /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP 600         /* Time ESP32 will go to sleep (in seconds) */

// Display configuration (defaults are in main.cpp)
// #define EPD_FAST_BW 1            /* 1: B/W mode, ~4 s refresh; 0: 3-color driver, ~18 s refresh */
// #define EPD_COLOR_CLEAN_EVERY 24 /* B/W mode: 3-color clean refresh every N boots, 0 = never */
