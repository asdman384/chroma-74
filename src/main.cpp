#include <SPI.h>
#include <WiFi.h>
#include "time.h"
#include <Arduino.h>
#include <ArduinoJSON.h> // https://arduinojson.org/v7/how-to/use-arduinojson-with-httpclient/
#include <GxEPD2_3C.h>
#include <config.h>
#include <HTTPClient.h>
#include <images.h>

#include <JetBrainsMono_Regular18pt7b.h>
#include <JetBrainsMono_Bold30pt7b.h>
#include <JetBrainsMono_Regular8pt7b.h>
#include <JetBrainsMonoNL_Regular7pt7b.h>
#include <JetBrainsMono_Bold18pt7b.h>

static const uint8_t EPD_BUSY = D6; // to EPD BUSY
static const uint8_t EPD_RST = D5;  // to EPD RST
static const uint8_t EPD_DC = D4;   // to EPD DC
static const uint8_t EPD_CS = D3;   // to EPD CS
static const uint8_t EPD_SCK = D2;  // to EPD CLK
static const uint8_t EPD_MOSI = D1; // to EPD DIN (SDI)
// static const uint8_t EPD_MISO = D10; // not used

GxEPD2_3C<GxEPD2_750c_Z08, GxEPD2_750c_Z08::HEIGHT> display(GxEPD2_750c_Z08(EPD_CS, EPD_DC, EPD_RST, EPD_BUSY));

class ErrorHandler
{
public:
  static void handleHttpError(JsonDocument &doc, int responseCode, const char *service)
  {
    doc["error"] = String(service) + " HTTP Error code: " + String(responseCode);
  }

  static void handleJsonError(JsonDocument &doc, DeserializationError error, const char *service)
  {
    doc["error"] = String("Deserialize ") + service + " failed: " + error.f_str();
  }
};

// ################ VARIABLES ###########################################
int wifi_signal;
RTC_DATA_ATTR int bootCount = 0;
// #######################################################################
void init_display();
float get_battery_voltage();
void print_wakeup_reason();
tm *parse_iso_datetime(const char *iso_string);
void begin_sleep();
void print_error(const char *text);
bool fetch_data(JsonDocument &doc, const char *url, const char *payload);
uint8_t start_wifi();
void draw_grid(uint16_t cell = 10, uint16_t color = GxEPD_BLACK);
void stop_wifi()
{
  WiFi.disconnect();
  WiFi.mode(WIFI_OFF);
}

void draw_image_weather_condition(String condition, bool isDaytime)
{
  if (condition == "CLEAR" || condition == "MOSTLY_CLEAR" || condition == "PARTLY_CLOUDY")
  {
    display.drawInvertedBitmap(10, 60, isDaytime ? clear : clear_n, 128, 128, GxEPD_BLACK);
  }
  else if (condition == "MOSTLY_CLOUDY")
  {
    display.drawInvertedBitmap(10, 60, isDaytime ? mostly_cloudy : mostly_cloudy_n, 128, 128, GxEPD_BLACK);
  }
  else if (condition == "CLOUDY")
  {
    display.drawInvertedBitmap(10, 60, cloudy, 128, 128, GxEPD_BLACK);
  }
  else if (
      condition == "LIGHT_RAIN_SHOWERS" ||
      condition == "CHANCE_OF_SHOWERS" ||
      condition == "SCATTERED_SHOWERS" ||
      condition == "LIGHT_RAIN")
  {
    display.drawInvertedBitmap(10, 60, light_rain_showers, 128, 128, GxEPD_BLACK);
  }
  else if (
      condition == "LIGHT_TO_MODERATE_RAIN" ||
      condition == "RAIN" ||

      condition == "MODERATE_TO_HEAVY_RAIN" ||
      condition == "RAIN_PERIODICALLY_HEAVY")
  {
    display.drawInvertedBitmap(10, 60, moderate_rain, 50, 50, GxEPD_BLACK);
  }
  else if (
      condition == "RAIN_AND_SNOW" || // TODO: add RAIN_AND_SNOW image
      condition == "SNOW" ||
      condition == "LIGHT_SNOW_SHOWERS" || // TODO: add LIGHT_SNOW_SHOWERS image
      condition == "LIGHT_SNOW"            // TODO: add LIGHT_SNOW image
  )
  {
    display.drawInvertedBitmap(10, 60, cloud_snow, 128, 128, GxEPD_BLACK);
  }
  else if (

      condition == "HEAVY_RAIN" ||
      condition == "RAIN_SHOWERS" ||
      condition == "HEAVY_RAIN_SHOWERS")
  {
    display.drawInvertedBitmap(10, 60, heavy_rain, 50, 50, GxEPD_BLACK);
  }
  else if (
      condition == "THUNDERSTORM" ||
      condition == "THUNDERSHOWER" ||
      condition == "LIGHT_THUNDERSTORM_RAIN" ||
      condition == "SCATTERED_THUNDERSTORMS" ||
      condition == "HEAVY_THUNDERSTORM")
  {
    display.drawInvertedBitmap(10, 60, isDaytime ? thunderstorm : thunderstorm_n, 50, 50, GxEPD_BLACK);
  }
  else
  {
    display.setCursor(10, 60);
    display.setFont(&JetBrainsMono_Regular8pt7b);
    display.print(condition);
  }
}

void render_pollutant(int x, int y, JsonArray pollutants, int index)
{
  String name = pollutants[index]["displayName"].as<String>();
  display.setCursor(x, y);
  display.print(name + ":");
  display.setCursor(x + 60, y);
  int value = pollutants[index]["concentration"]["value"].as<int>();
  display.print(value);
  display.setCursor(x + 105, y);

  String units = pollutants[index]["concentration"]["units"].as<String>();
  display.print(
      units == "MICROGRAMS_PER_CUBIC_METER"   ? "ug/m3"
      : units == "PARTS_PER_BILLION"          ? "ppb"
      : units == "PARTS_PER_MILLION"          ? "ppm"
      : units == "MILLIGRAMS_PER_CUBIC_METER" ? "mg/m3"
                                              : units);

  display.setCursor(x + 150, y);
  String interpretation = "-";
  if (name == "SO2")
  {
    if (value < 20)
      interpretation = "Good";
    else if (value < 80)
      interpretation = "Fair";
    else if (value < 250)
      interpretation = "Moderate";
    else if (value < 350)
      interpretation = "Poor";
    else
      interpretation = "Very Poor";
  }
  else if (name == "NO2")
  {
    if (value < 40)
      interpretation = "Good";
    else if (value < 70)
      interpretation = "Fair";
    else if (value < 150)
      interpretation = "Moderate";
    else if (value < 200)
      interpretation = "Poor";
    else
      interpretation = "Very Poor";
  }
  else if (name == "PM10")
  {
    if (value < 20)
      interpretation = "Good";
    else if (value < 50)
      interpretation = "Fair";
    else if (value < 100)
      interpretation = "Moderate";
    else if (value < 200)
      interpretation = "Poor";
    else
      interpretation = "Very Poor";
  }
  else if (name == "PM2.5")
  {
    if (value < 10)
      interpretation = "Good";
    else if (value < 25)
      interpretation = "Fair";
    else if (value < 50)
      interpretation = "Moderate";
    else if (value < 75)
      interpretation = "Poor";
    else
      interpretation = "Very Poor";
  }
  else if (name == "O3")
  {
    if (value < 60)
      interpretation = "Good";
    else if (value < 100)
      interpretation = "Fair";
    else if (value < 140)
      interpretation = "Moderate";
    else if (value < 180)
      interpretation = "Poor";
    else
      interpretation = "Very Poor";
  }
  else if (name == "CO")
  {
    if (value < 4400)
      interpretation = "Good";
    else if (value < 9400)
      interpretation = "Fair";
    else if (value < 12400)
      interpretation = "Moderate";
    else if (value < 15400)
      interpretation = "Poor";
    else
      interpretation = "Very Poor";
  }
  display.print(" [" + interpretation + "]");
}

void configure_wakeup_timer(struct tm *local_time)
{
  // https://github.com/espressif/arduino-esp32/blob/master/libraries/ESP32/examples/DeepSleep/TimerWakeUp/TimerWakeUp.ino
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);

  if (local_time->tm_hour >= 0 && local_time->tm_hour < 4)
  {
    esp_sleep_enable_timer_wakeup(28800 * uS_TO_S_FACTOR); // Sleep for 8 hours
  }

  Serial.println("Setup ESP32 to sleep for every " + String(TIME_TO_SLEEP) + " Seconds");
}

void setup()
{
  pinMode(A0, INPUT); // ADC
  Serial.begin(115200);
  delay(1000); // Take some time to open up the Serial Monitor
  Serial.println("Boot number: " + String(++bootCount));
  print_wakeup_reason();

  if (start_wifi() != WL_CONNECTED)
  {
    display.drawInvertedBitmap(0, 0, offline, 122, 122, GxEPD_BLACK);
    Serial.println("Failed to connect to WiFi.");
    display.display(true);
    return begin_sleep();
  }

  JsonDocument aqi_doc;     // https://developers.google.com/maps/documentation/air-quality/current-conditions
  JsonDocument weather_doc; // https://developers.google.com/maps/documentation/weather/current-conditions
  init_display();
  Serial.println("init_display");
  if (!(fetch_data(aqi_doc, AQI_URL, AQI_REQUEST) && fetch_data(weather_doc, WEATHER_URL, nullptr)))
  {
    stop_wifi();
    Serial.println(aqi_doc["error"].as<String>() + "\n" + weather_doc["error"].as<String>());
    print_error((aqi_doc["error"].as<String>() + "\n" + weather_doc["error"].as<String>()).c_str());
    display.display(true);
    return begin_sleep();
  }

  Serial.println("start render");
  stop_wifi();
  float batteryVoltage = get_battery_voltage();
  char time[14]; // Format: "Apr 30  HH:MM" (N chars + null terminator)
  struct tm *local_time = parse_iso_datetime(weather_doc["currentTime"].as<String>().c_str());
  strftime(time, sizeof(time), "%b %d  %H:%M", local_time);

  configure_wakeup_timer(local_time);

  display.setTextColor(GxEPD_BLACK);

  // Draw a 10x10 px grid as background
  // draw_grid(10, GxEPD_BLACK);

  // TIME and Battery
  display.setFont(&JetBrainsMono_Regular18pt7b);
  display.setCursor(10, 40);
  Serial.println((String(time)).c_str());
  display.print((String(time)).c_str());
  display.setCursor(350, 40);
  display.print(("V:" + String(batteryVoltage, 2)).c_str());

  String condition = weather_doc["weatherCondition"]["type"].as<String>();
  boolean isDaytime = weather_doc["isDaytime"].as<boolean>();
  draw_image_weather_condition(condition, isDaytime);

  // JetBrainsMono_Bold30pt7b
  // temperature
  int temp = ceil(weather_doc["temperature"]["degrees"].as<float>());
  display.setFont(&JetBrainsMono_Bold30pt7b);
  display.setCursor(150, 110);
  display.print((temp > 0 ? "+" : "") + String(temp));

  // JetBrainsMono_Regular18pt7b
  // feels Like Temperature
  display.setFont(&JetBrainsMono_Regular18pt7b);
  display.setCursor(150, 155);
  float feel = weather_doc["feelsLikeTemperature"]["degrees"].as<float>();
  display.print(String(feel > 0 ? "+" : "") + String(feel, 1));

  // humidity
  int y = 75;
  display.drawInvertedBitmap(280, y, humidity32, 32, 32, GxEPD_BLACK);
  display.setCursor(320, y + 25);
  display.print(weather_doc["relativeHumidity"].as<int>());

  // cloud cover
  display.drawInvertedBitmap(380, y, cloud32, 32, 32, GxEPD_BLACK);
  display.setCursor(420, y + 25);
  display.print(weather_doc["cloudCover"].as<int>());

  // wind
  display.drawInvertedBitmap(280, y + 45, windsock, 32, 32, GxEPD_BLACK);
  display.setCursor(320, y + 75);
  display.print(weather_doc["wind"]["speed"]["value"].as<int>());

  // probability of precipitation
  int precipitation = weather_doc["precipitation"]["probability"]["percent"].as<int>();
  String type = weather_doc["precipitation"]["probability"]["type"].as<String>();
  display.drawInvertedBitmap(380, y + 45, type == "RAIN" ? rain_prob : snow_prob, 32, 32, GxEPD_BLACK);
  display.setCursor(420, y + 75);
  display.print(precipitation);

  // JetBrainsMono_Regular18pt7b
  // air quality index
  display.setFont(&JetBrainsMono_Regular18pt7b);
  display.setCursor(10, 220);
  display.print("Air Quality: ");
  display.setFont(&JetBrainsMono_Bold18pt7b);
  String aqiDisplay = aqi_doc["indexes"][0]["aqiDisplay"].as<String>();
  display.print(aqiDisplay + "-");
  String category = aqi_doc["indexes"][0]["category"].as<String>().substring(0, aqi_doc["indexes"][0]["category"].as<String>().indexOf(" air quality"));
  display.print(category);

  // pollutants
  display.setFont(&JetBrainsMonoNL_Regular7pt7b);
  y = 230;
  render_pollutant(10, y += 20, aqi_doc["pollutants"], 3); // PM10
  render_pollutant(250, y, aqi_doc["pollutants"], 4);      // PM2.5
  render_pollutant(10, y += 20, aqi_doc["pollutants"], 2); // O3
  render_pollutant(250, y, aqi_doc["pollutants"], 0);      // CO
  render_pollutant(10, y += 20, aqi_doc["pollutants"], 5); // SO2
  render_pollutant(250, y, aqi_doc["pollutants"], 1);      // NO2

  display.display(true);
  begin_sleep();
}

void loop()
{
}

// ...existing code...

// Draw a uniform grid with given cell size and color
void draw_grid(uint16_t cell, uint16_t color)
{
  uint16_t w = display.width();
  uint16_t h = display.height();

  // Vertical lines
  for (uint16_t x = 0; x <= w; x += cell)
  {
    display.drawFastVLine(x, 0, h, color);
  }
  // Horizontal lines
  for (uint16_t y = 0; y <= h; y += cell)
  {
    display.drawFastHLine(0, y, w, color);
  }
}

bool fetch_data(JsonDocument &doc, const char *url, const char *payload)
{
  HTTPClient http;
  http.useHTTP10(true);
  http.begin(url);
  int maxRetries = 3;
  for (int i = 0; i < maxRetries; i++)
  {
    int responseCode = payload ? http.POST(payload) : http.GET();
    if (responseCode == 200)
    {
      DeserializationError error = deserializeJson(doc, http.getStream());
      if (!error)
      {
        http.end();
        return true;
      }
      ErrorHandler::handleJsonError(doc, error, payload ? "AQI" : "Weather");
    }
    else
    {
      ErrorHandler::handleHttpError(doc, responseCode, payload ? "AQI" : "Weather");
    }
    delay(1000);
  }

  http.end();
  return false;
}

void begin_sleep()
{
  Serial.println("begin_sleep");
  display.powerOff();
  stop_wifi();

  Serial.println("Entering deep sleep...");
  esp_deep_sleep_start();
}

uint8_t start_wifi()
{
  Serial.print("\r\nConnecting to: ");
  Serial.println(String(WIFI_SSID_1));
  IPAddress dns(8, 8, 8, 8); // Google DNS
  WiFi.disconnect();
  WiFi.mode(WIFI_STA); // switch off AP
  WiFi.begin(WIFI_SSID_1, WIFI_PASSWORD_1);
  unsigned long start = millis();
  uint8_t connectionStatus;
  bool AttemptConnection = true;
  while (AttemptConnection)
  {
    connectionStatus = WiFi.status();
    if (millis() > start + 15000)
    { // Wait 15-secs maximum
      AttemptConnection = false;
    }
    if (connectionStatus == WL_CONNECTED || connectionStatus == WL_CONNECT_FAILED)
    {
      AttemptConnection = false;
    }
    delay(50);
  }
  if (connectionStatus == WL_CONNECTED)
  {
    wifi_signal = WiFi.RSSI(); // Get Wifi Signal strength now, because the WiFi will be turned off to save power!
    Serial.println("WiFi connected at: " + WiFi.localIP().toString());
  }
  else
    Serial.println("WiFi connection *** FAILED ***");
  return connectionStatus;
}

void init_display()
{
  // Explicitly map SPI pins for Seeed XIAO ESP32C3 (or your chosen wiring)
  // If you wire SCK/MOSI to different pins, adjust EPD_SCK/EPD_MOSI above.
  SPI.end();
  SPI.begin(EPD_SCK, /*MISO=*/-1, EPD_MOSI, /*SS=*/EPD_CS);

  // Some big tri-color panels need a moment after power-on
  delay(100);

  display.init(115200);
  display.fillScreen(GxEPD_WHITE);
  display.setFullWindow();
  display.setRotation(3);
}

float get_battery_voltage()
{
  uint32_t sumMv = 0;
  const int samples = 16;
  for (int i = 0; i < samples; ++i)
  {
    sumMv += analogReadMilliVolts(A0); // ADC in mV
    delay(2);
  }
  float voltage = (2.0f * sumMv) / samples / 1000.0f; // account for 1/2 divider, convert mV->V
  Serial.println("Battery Voltage: " + String(voltage, 3) + " V");
  return voltage;
}

// Print the wakeup reason for ESP32
void print_wakeup_reason()
{
  esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();

  switch (wakeup_reason)
  {
  case ESP_SLEEP_WAKEUP_EXT0:
    Serial.println("Wakeup caused by external signal using RTC_IO");
    break;
  case ESP_SLEEP_WAKEUP_EXT1:
    Serial.println("Wakeup caused by external signal using RTC_CNTL");
    break;
  case ESP_SLEEP_WAKEUP_TIMER:
    Serial.println("Wakeup caused by timer");
    break;
  case ESP_SLEEP_WAKEUP_TOUCHPAD:
    Serial.println("Wakeup caused by touchpad");
    break;
  case ESP_SLEEP_WAKEUP_ULP:
    Serial.println("Wakeup caused by ULP program");
    break;
  default:
    Serial.printf("Wakeup was not caused by deep sleep: %d\n", wakeup_reason);
    break;
  }
}

void print_error(const char *text)
{
  display.setFont(&JetBrainsMono_Regular18pt7b);
  display.setTextColor(GxEPD_BLACK);
  int16_t tbx, tby;
  uint16_t tbw, tbh;
  display.getTextBounds(text, 0, 0, &tbx, &tby, &tbw, &tbh);
  // center bounding box by transposition of origin:
  uint16_t x = ((display.width() - tbw) / 2) - tbx;
  uint16_t y = ((display.height() - tbh) / 2) - tby;
  display.setCursor(x, y);
  display.print(text);
}

// Helper function to parse ISO datetime string
tm *parse_iso_datetime(const char *iso_string)
{
  struct tm tm = {0};
  char *ret;

  // Parse ISO format: "2024-04-11T15:30:00Z"
  ret = strptime(iso_string, "%Y-%m-%dT%H:%M:%S", &tm);

  if (ret == NULL)
  {
    Serial.println("Failed to parse datetime");
    return NULL;
  }

  // Convert to time_t (UTC) and adjust for local timezone (UTC+2)
  time_t time = mktime(&tm) + (2 * 3600);
  return localtime(&time);
}
