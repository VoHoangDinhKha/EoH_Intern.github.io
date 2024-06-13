#define ERA_DEBUG
#define DEFAULT_MQTT_HOST "mqtt1.eoh.io"
#define ERA_AUTH_TOKEN "846bc96e-9c45-4edd-b46c-39d124854dc7"

  //----------ERa liberary----------//
#include <ERa.hpp>
#include <ERa/ERaTimer.hpp>

#include <Arduino.h>
#include <Wire.h>

#include <WiFi.h>
#include <WiFiUdp.h>
#include <WiFiClient.h>
#include <HTTPClient.h>

#include "SPI.h"
#include "TFT_22_ILI9225.h"
#include <ArduinoJson.h>
#include "DHT.h"

  //----------Bitmap liberaries----------//
#include "img_map.h"
#include "Loading_Frames.h"
#include "Weather-Moon_Icons.h"


  //----------Libraries for time handling----------//
#include <TimeLib.h> 
#include "NTPClient.h"
#include <RTClib.h>
#include <vn_lunar.h>


  //----------Font Libraries----------//
#include <../fonts/Calculator12pt7b.h>
#include <../fonts/Calculator9pt7b.h>

#include <../fonts/VNLUCIDA5pt7b.h>

#include <../fonts/Technology14pt7b.h>
#include <../fonts/Technology20pt7b.h>


#ifdef ARDUINO_ARCH_STM32F1
#define TFT_RST PA1
#define TFT_RS  PA2
#define TFT_CS  PA0 // SS
#define TFT_SDI PA7 // MOSI
#define TFT_CLK PA5 // SCK
#define TFT_LED 0   // 0 if wired to +5V directly
#elif defined(ESP8266)
#define TFT_RST 4   // D2
#define TFT_RS  5   // D1
#define TFT_CLK 14  // D5 SCK
//#define TFT_SDO 12  // D6 MISO
#define TFT_SDI 13  // D7 MOSI
#define TFT_CS  15  // D8 SS
#define TFT_LED 2   // D4     set 0 if wired to +5V directly -> D3=0 is not possible !!
#elif defined(ESP32)
#define TFT_RST 15  // IO 26
#define TFT_RS 25  // IO 25
#define TFT_CLK 18  // HSPI-SCK
//#define TFT_SDO 12  // HSPI-MISO
#define TFT_SDI 23  // HSPI-MOSI
#define TFT_CS  5  // HSPI-SS0
#define TFT_LED 0   // 0 if wired to +5V directly
SPIClass vspi(VSPI);
#else
#define TFT_RST 8
#define TFT_RS  9
#define TFT_CS  10  // SS
#define TFT_SDI 11  // MOSI
#define TFT_CLK 13  // SCK
#define TFT_LED 3   // 0 if wired to +5V directly
#endif

#define TFT_BRIGHTNESS 255 // Initial brightness of TFT backlight (optional)

#define BUTTON 13
#define DHTPIN 26
#define BUZZER 2
#define TONE_KHZ 2500

#define LEDRGB 27
#define LED1 14
#define LED2 12


TFT_22_ILI9225 tft = TFT_22_ILI9225(TFT_RST, TFT_RS, TFT_CS, TFT_LED, TFT_BRIGHTNESS);

const char* ssid     = "";
const char* password = "";

  //Declare the UDP port
WiFiUDP ntpUDP;
  //Create NTP client
NTPClient timeClient(ntpUDP, "pool.ntp.org", 7 * 3600, 60000); // GMT+7, Update every 60 second

  //Declare names of days and months
String weekDays[7]={"Sunday", "Monday", "Tuesday", "Wednesday", "Thusday", "Friday", "Saturday"};
String months[12]={"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

unsigned long lastNTPUpdate = 0;
const unsigned long NTP_UPDATE_INTERVAL = 180000; //30 minute

bool isNetworkConnected = false;
bool rtcSynced = false;

RTC_DS1307 rtc;

const int DHTTYPE = DHT22;
DHT dht(DHTPIN, DHTTYPE);
float t, h;

ERaTimer timer;
void timerEvent() {
    ERA_LOG("Timer", "Uptime: %d", ERaMillis() / 1000L);
}

TaskHandle_t Era;
TaskHandle_t Led1;
TaskHandle_t Led2;

  //-------------------------------------------------------Setup-------------------------------------------------------//
void setup(void) {
  pinMode(BUTTON, INPUT_PULLUP);
  Serial.begin(115100);

  #if defined(ESP32)
    vspi.begin();
    tft.begin(vspi);
  #else
    tft.begin();
  #endif

  #ifdef AVR
    Wire.begin();
  #else
    Wire1.begin(); // Shield I2C pins connect to alt I2C bus on Arduino Due
  #endif

  Loading();

  Draw_map1();

  dht.begin();

  rtc.begin();
  
    //Connect to Wi-Fi
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  Serial.println("");
  Serial.println("WiFi connected.");

  delay(3000);

  timeClient.begin();
  Localtime();

  pinMode(BUZZER, OUTPUT);
  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);
  pinMode(LEDRGB, OUTPUT);

  digitalWrite(LEDRGB, HIGH);

  xTaskCreatePinnedToCore(Task_Era, "Era", 10000, NULL, 1, &Era, 0);  //delay(500);
  xTaskCreatePinnedToCore(Task_Led1, "Led1", 10000, NULL, 1, &Led1, 1); //delay(500);
  xTaskCreatePinnedToCore(Task_Led2, "Led2", 10000, NULL, 1, &Led2, 1); //delay(500);
}

  //-------------------------------------------------------FreeRTOS-------------------------------------------------------//
void Task_Era(void * parameter) {
  Serial.println(xPortGetCoreID()); 
  ERa.begin(ssid, password);
  timer.setInterval(1000L, timerEvent);
  for(;;){
    ERa.run();
    timer.run();
  }
}

void Task_Led1(void * parameter) {
  for(;;){
    digitalWrite(LED1, HIGH);  
    delay(50);              
    digitalWrite(LED1, LOW);    
    delay(50);  

    digitalWrite(LED1, HIGH);  
    delay(50);              
    digitalWrite(LED1, LOW);    
    delay(3000);
  }
}

void Task_Led2(void * parameter) {
  for(;;){
    delay(3000);
    digitalWrite(LED2, HIGH);  
    delay(100);              
    digitalWrite(LED2, LOW);    
    delay(100);

    digitalWrite(LED2, HIGH);  
    delay(100);              
    digitalWrite(LED2, LOW);  
  }
}

  //-------------------------------------------------------Loading-------------------------------------------------------//
void Loading() {
  int x = 3;
  int y = 37;

  tft.drawBitmap(x, y, Loading1, 170, 147);   delay(75);
  tft.drawBitmap(x, y, Loading2, 170, 147);   delay(75);
  tft.drawBitmap(x, y, Loading3, 170, 147);   delay(75);
  tft.drawBitmap(x, y, Loading4, 170, 147);   delay(75);
  tft.drawBitmap(x, y, Loading5, 170, 147);   delay(75);
  tft.drawBitmap(x, y, Loading6, 170, 147);   delay(75);
  tft.drawBitmap(x, y, Loading7, 170, 147);   delay(75);
  tft.drawBitmap(x, y, Loading8, 170, 147);   delay(75);
  tft.drawBitmap(x, y, Loading9, 170, 147);   delay(75);
  tft.drawBitmap(x, y, Loading10, 170, 147);  delay(75);
  tft.drawBitmap(x, y, Loading11, 170, 147);  delay(75);
  tft.drawBitmap(x, y, Loading12, 170, 147);  delay(75);
  tft.drawBitmap(x, y, Loading13, 170, 147);  delay(75);
  tft.drawBitmap(x, y, Loading14, 170, 147);  delay(75);
  tft.drawBitmap(x, y, Loading15, 170, 147);  delay(75);
  tft.drawBitmap(x, y, Loading16, 170, 147);  delay(75);
  tft.drawBitmap(x, y, Loading17, 170, 147);  delay(75);
  tft.drawBitmap(x, y, Loading18, 170, 147);  delay(75);
  tft.drawBitmap(x, y, Loading19, 170, 147);  delay(75);
  tft.drawBitmap(x, y, Loading20, 170, 147);  delay(75);
  tft.drawBitmap(x, y, Loading21, 170, 147);  delay(75);
  tft.drawBitmap(x, y, Loading22, 170, 147);  delay(75);
  tft.drawBitmap(x, y, Loading23, 170, 147);  delay(75);
  tft.drawBitmap(x, y, Loading24, 170, 147);  delay(75);
  tft.drawBitmap(x, y, Loading25, 170, 147);  delay(75);

  tft.drawBitmap(x, y, Loading1, 170, 147);   delay(75);
  tft.drawBitmap(x, y, Loading2, 170, 147);   delay(75);
  tft.drawBitmap(x, y, Loading3, 170, 147);   delay(75);
  tft.drawBitmap(x, y, Loading4, 170, 147);   delay(75);
  tft.drawBitmap(x, y, Loading5, 170, 147);   delay(75);
  tft.drawBitmap(x, y, Loading6, 170, 147);   delay(75);
  tft.drawBitmap(x, y, Loading7, 170, 147);   delay(75);
  tft.drawBitmap(x, y, Loading8, 170, 147);   delay(75);
  tft.drawBitmap(x, y, Loading9, 170, 147);   delay(75);
  tft.drawBitmap(x, y, Loading10, 170, 147);  delay(75);
  tft.drawBitmap(x, y, Loading11, 170, 147);  delay(75);
  tft.drawBitmap(x, y, Loading12, 170, 147);  delay(75);
  tft.drawBitmap(x, y, Loading13, 170, 147);  delay(75);
  tft.drawBitmap(x, y, Loading14, 170, 147);  delay(75);
  tft.drawBitmap(x, y, Loading15, 170, 147);  delay(75);
  tft.drawBitmap(x, y, Loading16, 170, 147);  delay(75);
  tft.drawBitmap(x, y, Loading17, 170, 147);  delay(75);
  tft.drawBitmap(x, y, Loading18, 170, 147);  delay(75);
  tft.drawBitmap(x, y, Loading19, 170, 147);  delay(75);
  tft.drawBitmap(x, y, Loading20, 170, 147);  delay(75);
  tft.drawBitmap(x, y, Loading21, 170, 147);  delay(75);
  tft.drawBitmap(x, y, Loading22, 170, 147);  delay(75);
  tft.drawBitmap(x, y, Loading23, 170, 147);  delay(75);
  tft.drawBitmap(x, y, Loading24, 170, 147);  delay(75);
  tft.drawBitmap(x, y, Loading25, 170, 147);  delay(300);

  tft.clear();
}

  //-------------------------------------------------------Draw Maps-------------------------------------------------------//
void Draw_map1() {
    //Draw screen lightgreen boundaries
  tft.drawRectangle(0, 0, 175, 219, COLOR_GREENYELLOW);

  tft.drawLine(5, 89, 170, 89, COLOR_GREENYELLOW);

  tft.fillTriangle(1, 1, 1, 2, 2, 1, COLOR_GREENYELLOW);
  tft.fillTriangle(173, 1, 174, 1, 174, 2, COLOR_GREENYELLOW);
  tft.fillTriangle(1, 217, 1, 218, 2, 218, COLOR_GREENYELLOW);
  tft.fillTriangle(174, 218, 173, 218, 174, 217, COLOR_GREENYELLOW);

  tft.fillTriangle(0, 85, 0, 93, 4, 89, COLOR_GREENYELLOW);
  tft.fillTriangle(171, 89, 175, 85, 175, 93, COLOR_GREENYELLOW);
  tft.fillTriangle(83, 89, 93, 89, 88, 94, COLOR_GREENYELLOW);
  tft.fillTriangle(84, 219, 88, 215, 92, 219, COLOR_GREENYELLOW);

    //Draw screen gray boundaries
  tft.drawLine(3, 1, 172, 1, COLOR_GRAY);
  tft.drawLine(1, 3, 1, 85, COLOR_GRAY);
  tft.drawLine(4, 88, 171, 88, COLOR_GRAY);
  tft.drawLine(174, 3, 174, 85, COLOR_GRAY);
  tft.drawPixel(2, 2, COLOR_GRAY);
  tft.drawPixel(173, 2, COLOR_GRAY);
  tft.drawLine(2, 86, 3, 87, COLOR_GRAY);
  tft.drawLine(173, 86, 172, 87, COLOR_GRAY);

  tft.drawLine(4, 90, 83, 90, COLOR_GRAY);
  tft.drawLine(1, 93, 1, 216, COLOR_GRAY);
  tft.drawLine(3, 218, 84, 218, COLOR_GRAY);
  tft.drawLine(2, 92, 3, 91, COLOR_GRAY);
  tft.drawLine(84, 91, 86, 93, COLOR_GRAY);
  tft.drawLine(85, 217, 86, 216, COLOR_GRAY);
  tft.drawPixel(2, 217, COLOR_GRAY);

  tft.drawLine(93, 90, 171, 90, COLOR_GRAY);
  tft.drawLine(92, 218, 172, 218, COLOR_GRAY);
  tft.drawLine(174, 93, 174, 216, COLOR_GRAY);
  tft.drawLine(90, 93, 92, 91, COLOR_GRAY);
  tft.drawLine(172, 91, 173, 92, COLOR_GRAY);
  tft.drawLine(90, 216, 91, 217, COLOR_GRAY);
  tft.drawPixel(173, 217, COLOR_GRAY);
}

void Draw_map2() {
  tft.drawLine(87, 94, 87, 215, COLOR_GRAY);
  tft.drawLine(89, 94, 89, 215, COLOR_GRAY);
  tft.drawLine(88, 94, 88, 215, COLOR_GREENYELLOW);

  tft.setGFXFont(&Calculator12pt7b);
  tft.drawGFXText(13, 108, "Indoor", COLOR_GREEN);
  tft.drawGFXText(27, 126, "Sensor", COLOR_GREEN);

  tft.drawGFXText(108, 108, "Lunar", COLOR_GREEN);
  tft.drawGFXText(93, 126, "Calendar", COLOR_GREEN);

  tft.setGFXFont(&Calculator9pt7b);
  tft.drawGFXText(5, 147, "Temp:", COLOR_GOLD);
  tft.drawGFXText(5, 189, "Humi:", COLOR_GOLD);

  tft.setFont(Terminal6x8);
  tft.drawText(29, 78, "-Clock by Dinh Kha-", COLOR_CYAN);

  tft.drawBitmap(3, 2, EoH, 60, 30);
}


  //-------------------------------------------------------Draw Bitmaps-------------------------------------------------------//
void Drawbitmaps() {
  tft.drawBitmap(61, 153, temp_map1, 24, 24);
  tft.drawBitmap(2, 192, humi_map1, 24, 24);
  tft.drawBitmap(3, 110, indoor, 24, 24);
  tft.drawBitmap(91, 136, lunar_calender, 24, 24);
}

  //-------------------------------------------------------DHT22 Sensor-------------------------------------------------------//
void Sensor() {
  bool temp_change, humi_change;
  temp_change = t != dht.readTemperature();
  humi_change = h != dht.readHumidity();

    //Draw DHT data 
  tft.setGFXFont(&Technology14pt7b);
    
  if (temp_change) {
    t = dht.readTemperature();
  }
  tft.fillRectangle(4, 151, 59, 173, COLOR_BLACK);
  String dht_temp = String(t);
  tft.drawGFXText(4, 173, dht_temp, COLOR_RED); 
  
  if (humi_change) {
    h = dht.readHumidity();
    tft.fillRectangle(27, 191, 82, 213, COLOR_BLACK);
  }
  String dht_humi = String(h);
  tft.drawGFXText(27, 213, dht_humi, COLOR_TURQUOISE);

    //Send data to ERa
  ERa.virtualWrite(V5, t);
  ERa.virtualWrite(V6, h);

}

  //-------------------------------------------------------Localtime-------------------------------------------------------//
void Print_Localtime() {
  if (WiFi.status() == WL_CONNECTED && !isNetworkConnected) {
    Serial.println("Network connected.");
    isNetworkConnected = true;
    Localtime(); //Check RTC synchronization when connecting to the network again
  }

  if (WiFi.status() != WL_CONNECTED && isNetworkConnected) {
    Serial.println("Network disconnected.");
    isNetworkConnected = false;
  }

  unsigned long currentMillis = millis();

    //Check if enough time has passed to update NTP
  if (currentMillis - lastNTPUpdate >= NTP_UPDATE_INTERVAL) {
    Localtime();
  }

    //Check if RTC time update from NTP is needed
  if (rtcSynced) {
    DateTime rtcTime = rtc.now();

    //Draw Localtime
    tft.fillRectangle(73, 38, 170, 67, COLOR_BLACK);
    tft.setGFXFont(&Technology20pt7b);

    //Check and display the standard time form 
    String h = (rtcTime.hour() < 10 ? "0" : "") + String(rtcTime.hour());
    String m = (rtcTime.minute() < 10 ? "0" : "") + String(rtcTime.minute());
    String s = (rtcTime.second() < 10 ? "0" : "") + String(rtcTime.second());

    tft.drawGFXText(73, 67, h + ":" + m, COLOR_LIGHTGREEN);
    tft.setGFXFont(&Calculator9pt7b);
    tft.drawGFXText(151, 67, ":" + s, COLOR_LIGHTGREEN);

    tft.fillRectangle(70, 3, 170, 37, COLOR_BLACK);

    String dayName = weekDays[rtcTime.dayOfTheWeek()];
    String monthName = months[rtcTime.month() - 1];

    tft.setGFXFont(&Calculator9pt7b);
    tft.drawGFXText(83, 15, dayName, COLOR_YELLOW);
    tft.drawGFXText(70, 32, monthName + " " + String(rtcTime.day()) + ", " +
                            String(rtcTime.year()), COLOR_YELLOW);
    
    unsigned long ntpEpochTime = timeClient.getEpochTime();
      
      //Compare the RTC time with the time from NTP
    if (ntpEpochTime != rtcTime.unixtime()) {

      rtc.adjust(DateTime(ntpEpochTime));
      rtcSynced = true;
      Serial.println("RTC time adjusted!");
    }

      //Lunar Calendar
    int dd = rtcTime.day();
    int mm = rtcTime.month();
    int yy = rtcTime.year(); 

    vn_lunar lunar;
    lunar.convertSolar2Lunar(dd, mm, yy); 

    int lunar_dd = lunar.get_lunar_dd();
    int lunar_mm = lunar.get_lunar_mm();
    int lunar_yy = lunar.get_lunar_yy();

    tft.setGFXFont(&Calculator9pt7b);
    tft.drawGFXText(124, 142, String(lunar_yy), COLOR_YELLOW);

    String monthLunarName = months[lunar_mm - 1];
    
    tft.setGFXFont(&Calculator12pt7b);
    tft.fillRectangle(116, 146, 173, 160, COLOR_BLACK);
    tft.drawGFXText(116, 160, monthLunarName + "," + String(lunar_dd), COLOR_YELLOW);

      //Send data to ERa
    ERa.virtualWrite(V7, lunar_dd);
    ERa.virtualWrite(V8, lunar_mm);
    ERa.virtualWrite(V9, lunar_yy);
    
    delay(1000);

      //Display moonPhase
    getPhase(yy, mm, dd);

      //Set Alarm 
    if (rtcTime.hour() == 7 && rtcTime.minute() >= 30 && rtcTime.minute() < 31) {
      tone(BUZZER, TONE_KHZ);
      delay(100);
      noTone(BUZZER);
      delay(125);
      tone(BUZZER, TONE_KHZ);
      delay(100);
      noTone(BUZZER);
    }
  }
}

void Localtime() {
  if (WiFi.status() == WL_CONNECTED) {
    timeClient.update();
    lastNTPUpdate = millis();
    rtc.adjust(DateTime(timeClient.getEpochTime()));
    rtcSynced = true;
    Serial.println("NTP synchronizes RTC!");
  } else {
    Serial.println("WiFi connection lost. Stop NTP synchronization!");
  }
}

  //Calculate the current phase of the moon
void getPhase(int Y, int M, int D) {  
  double AG, IP;                                        
                                      
  long YY, MM, K1, K2, K3, JD;        
                                     
  // calculate julian date
  YY = Y - floor((12 - M) / 10);
  MM = M + 9;
  if(MM >= 12)
    MM = MM - 12;
  
  K1 = floor(365.25 * (YY + 4712));
  K2 = floor(30.6 * MM + 0.5);
  K3 = floor(floor((YY / 100) + 49) * 0.75) - 38;

  JD = K1 + K2 + D + 59;
  if(JD > 2299160)
    JD = JD -K3;
  
  IP = normalize((JD - 2451550.1) / 29.530588853);
  AG = IP*29.53;
  
    //Draw Moon Phases
  int x = 107;
  int y = 166;

  if(AG < 1.20369)        tft.drawBitmap(x, y, Phase0, 50, 50);
  else if(AG < 3.61108)   tft.drawBitmap(x, y, Phase1, 50, 50);
  else if(AG < 6.01846)   tft.drawBitmap(x, y, Phase2, 50, 50);
  else if(AG < 8.42595)   tft.drawBitmap(x, y, Phase3, 50, 50);
  else if(AG < 10.83323)  tft.drawBitmap(x, y, Phase4, 50, 50); 
  else if(AG < 13.24062)  tft.drawBitmap(x, y, Phase5, 50, 50);   
  else if(AG < 15.64800)  tft.drawBitmap(x, y, Phase6, 50, 50);  
  else if(AG < 18.05539)  tft.drawBitmap(x, y, Phase7, 50, 50);   
  else if(AG < 20.46277)  tft.drawBitmap(x, y, Phase8, 50, 50);   
  else if(AG < 22.87016)  tft.drawBitmap(x, y, Phase9, 50, 50);   
  else if(AG < 25.27754)  tft.drawBitmap(x, y, Phase10, 50, 50);  
  else if(AG < 27.68493)  tft.drawBitmap(x, y, Phase11, 50, 50);  
  else                    tft.drawBitmap(x, y, Phase0, 50, 50);  
}

  //Normalize moon calculation between 0-1
double normalize(double v) {           
    v = v - floor(v);
    if (v < 0)
        v = v + 1;
    return v;
}

  //-------------------------------------------------------API Weather-------------------------------------------------------//
void API_Weather() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
      //URL to the OpenWeatherMap API
    String url = "https://api.openweathermap.org/data/2.5/weather?lat=10.75&lon=106.6667&units=metric&appid=4dae1895563bbad78d270f0a48cf0484";
    http.begin(url);

    int httpCode = http.GET();
    if (httpCode > 0) {
      if (httpCode == HTTP_CODE_OK) {
          //Get JSON data
        String payload = http.getString();

          //Create a cache to store JSON data
        const size_t capacity = JSON_OBJECT_SIZE(3) + JSON_OBJECT_SIZE(4) + 120;
        DynamicJsonDocument doc(capacity);

          //Parse JSON data
        deserializeJson(doc, payload);

          //Get weather data
        String str_current_weather = doc["weather"]["main"];
        const char* main= doc["weather"][0]["main"];
        const char* description = doc["weather"][0]["description"];
        String icon = doc["weather"][0]["icon"];

        float temperature = doc["main"]["temp"];
        float humidity = doc["main"]["humidity"];
        int pressure = doc["main"]["pressure"];

        float windSpeed = doc["wind"]["speed"];

        tft.fillRectangle(6, 76, 168, 87, COLOR_BLACK);
        tft.setFont(Terminal6x8);
        tft.drawText(33, 78, "-Weather Station-", COLOR_CYAN);

          //Draw weather data
        tft.fillRectangle(2, 94, 173, 215, COLOR_BLACK);
        tft.setGFXFont(&Calculator9pt7b);
        tft.drawGFXText(32, 106, doc["name"], COLOR_PINK);

        tft.setGFXFont(&Calculator9pt7b);
        tft.drawGFXText(5, 119, "Weather Description:", COLOR_GREEN);
        tft.drawGFXText(105, 143, main, COLOR_PURPLE);

        tft.drawGFXText(10, 132, "Windspeed", COLOR_SKYBLUE);
        tft.drawGFXText(25, 147, String(windSpeed) + " m/s", COLOR_GOLD);
        tft.drawGFXText(10, 159, "Pressure", COLOR_SKYBLUE);
        tft.drawGFXText(25, 173, String(pressure) + " hPa", COLOR_GOLD);

        tft.drawGFXText(25, 192, String(temperature), COLOR_RED);
        tft.drawGFXText(25, 213, String(humidity), COLOR_TURQUOISE);
        tft.drawGFXText(65, 192, " C", COLOR_RED);
        tft.drawGFXText(65, 213, " %", COLOR_TURQUOISE);

        tft.drawBitmap(5, 176, temp_map2, 20, 20);
        tft.drawBitmap(5, 197, humi_map2, 20, 20);

          //Draw Weather Icons
        int x = 99;
        int y = 145;

        if (icon == "01d") { tft.drawBitmap(x, y, clear_sky_day, 64, 64); }

        else if(icon == "01n") { tft.drawBitmap(x, y, clear_sky_night, 64, 64); }

        else if(icon == "02d") { tft.drawBitmap(x, y, few_clouds_day, 64, 64); }

        else if(icon == "02n") { tft.drawBitmap(x, y, few_clouds_night, 64, 64); }

        else if(icon == "03d") { tft.drawBitmap(x, y, scattered_clouds_day, 64, 64); }

        else if(icon == "03n") { tft.drawBitmap(x, y, scattered_clouds_night, 64, 64); }

        else if(icon == "04d" || icon == "04n") { tft.drawBitmap(x, y, broken_clouds, 64, 64); }

        else if(icon == "09d" || icon == "10d") { tft.drawBitmap(x, y, rain_day, 64, 64); }

        else if(icon == "09n" || icon == "10n") { tft.drawBitmap(x, y, rain_night, 64, 64); }

        else if(icon == "11d") { tft.drawBitmap(x, y, thunderstorm, 64, 64); }

          //Send data to ERa
        ERa.virtualWrite(V0,description);
        ERa.virtualWrite(V1,temperature);
        ERa.virtualWrite(V2,humidity);
        ERa.virtualWrite(V3,windSpeed);
        ERa.virtualWrite(V4,pressure);
      }
      delay(5000);
    } 
    
    else {
      Serial.println("Error on HTTP request");
    }
  http.end();
  }

    //Notification when internet connection disconnected
  else {
    tft.fillRectangle(2, 94, 173, 215, COLOR_BLACK);
    tft.setGFXFont(&Calculator12pt7b);
    tft.drawGFXText(10, 115, "Wifi is disconnect!", COLOR_RED);
    tft.drawGFXText(26, 210, "See you again", COLOR_TURQUOISE);
    
      // Draw Gif
    int z = 43;
    int p = 120;

    tft.drawBitmap(z, p, Disconnect1, 91, 70);  delay(200);
    tft.drawBitmap(z, p, Disconnect2, 91, 70);  delay(200);
    tft.drawBitmap(z, p, Disconnect3, 91, 70);  delay(200);
    tft.drawBitmap(z, p, Disconnect4, 91, 70);  delay(200);
    tft.drawBitmap(z, p, Disconnect5, 91, 70);  delay(200);
    tft.drawBitmap(z, p, Disconnect6, 91, 70);  delay(200);
    tft.drawBitmap(z, p, Disconnect7, 91, 70);  delay(200);
    tft.drawBitmap(z, p, Disconnect8, 91, 70);  delay(200);
    tft.drawBitmap(z, p, Disconnect9, 91, 70);  delay(200);
    tft.drawBitmap(z, p, Disconnect10, 91, 70);  delay(200);
    tft.drawBitmap(z, p, Disconnect11, 91, 70);  delay(200);
    tft.drawBitmap(z, p, Disconnect12, 91, 70);  delay(200);
    tft.drawBitmap(z, p, Disconnect13, 91, 70);  delay(200);
    tft.drawBitmap(z, p, Disconnect14, 91, 70);  delay(200);
  }
}

  //-------------------------------------------------------Draw API Icons-------------------------------------------------------//
void DrawAPI() { 
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http1;

      //URL to the OpenWeatherMap API
    String url1 = "http://api.openweathermap.org/data/2.5/weather?q=Ho%20Chi%20Minh%20City&units=metric&appid=4dae1895563bbad78d270f0a48cf0484";
    http1.begin(url1);

    int httpCode1 = http1.GET();
    if (httpCode1 > 0) {
      if (httpCode1 == HTTP_CODE_OK) {
          //Get JSON data
        String payload1 = http1.getString();

          //Create a cache to store JSON data
        const size_t capacity1 = JSON_OBJECT_SIZE(3) + JSON_OBJECT_SIZE(4) + 120;
        DynamicJsonDocument doc1(capacity1);

          //Parse JSON data
        deserializeJson(doc1, payload1);

          //Get weather data
        String main = doc1["weather"][0]["main"];

        int x = 15;
        int y = 32;

          //Draw Weather icons
        if (main == "Clear") { tft.drawBitmap(x, y, clear, 45, 45); }

        else if (main == "Clouds") { tft.drawBitmap(x, y, clouds, 45, 45); }

        else if (main == "Rain") { tft.drawBitmap(x, y, rain, 45, 45); }

        else if (main == "Thunderstorm") { tft.drawBitmap(x, y, thunder, 45, 45); }
      } 
      else {
        Serial.println("Error on HTTP request");
      }
    http1.end();
    }
  }
}

  //-------------------------------------------------------Switch between 2 screens-------------------------------------------------------//
void Screen_switching() {
  int buttonState = digitalRead(BUTTON);

  if(buttonState == HIGH) {
    API_Weather();
    tft.fillRectangle(2, 94, 173, 215, COLOR_BLACK);
    tft.fillRectangle(6, 76, 168, 87, COLOR_BLACK);
  } 
  
  else {
    Sensor();
    Draw_map2();
    DrawAPI();
    Drawbitmaps();
  }
}

  //-------------------------------------------------------Loop-------------------------------------------------------//
void loop() {  
  Print_Localtime();
  Screen_switching();
  if (WiFi.status() != WL_CONNECTED) {
    tft.fillRectangle(15, 32, 60, 77, COLOR_BLACK);
    tft.drawBitmap(15, 32, no_internet, 45, 45);
  }
}

