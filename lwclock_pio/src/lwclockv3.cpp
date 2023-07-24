// Clock based on ESP8266 и MAX7219

#include <Arduino.h>

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WiFiClientSecure.h>
#include <ESP8266HTTPUpdateServer.h>    // Обновление с web страницы

#include <LittleFS.h>
#include <SPI.h>
#include <Wire.h> // must be included here so that Arduino library object file references work              
#include <ArduinoJson.h>        //https://github.com/bblanchon/ArduinoJson.git
#include <MD_Parola.h>          //https://github.com/MajicDesigns/MD_Parola
#include <MD_MAX72xx.h>         //https://github.com/MajicDesigns/MD_MAX72XX

ESP8266HTTPUpdateServer httpUpdater;  // Объект для обнавления с web страницы

#include <DS3231.h>
#include <Wire.h>

DateTime now;
float nowtime; // для проверки времени в интервале

// +3.3 V
// 5 SCL D1
// 4 SDA D2


#include "everytime.h"
#include "fonts_rus.h"
#include "font_5bite_rus.h"
#include "font_6bite_rus.h"
#include "font_msx.h"

// +5 V
// 13 DIN D7
// 14 CLK D5
// 15 CS D8
#define DATA_PIN  13
#define CLK_PIN   14
#define CS_PIN    15

#define MAX_DEVICES 4  //Number of indicator modules MAX7219
#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
MD_Parola P = MD_Parola(HARDWARE_TYPE, CS_PIN, MAX_DEVICES);

uint8_t rnd = 0;

typedef struct
{
    textEffect_t  effect;   // text effect to display
    String        psz;      // text string nul terminated
    uint16_t      speed;    // speed multiplier of library default
    uint16_t      pause;    // pause multiplier for library default
} sCatalog;

sCatalog  catalog[] =
{
    { PA_PRINT, "PRINT", 1, 5 }, //0
    { PA_SLICE, "SLICE", 1, 5 }, //1
    { PA_MESH, "MESH", 20, 5 },  //2
    { PA_FADE, "FADE", 20, 4 },  //3
    { PA_WIPE, "WIPE", 5, 4 },   //4
    { PA_WIPE_CURSOR, "WPE_C", 4, 4 },  //5
    { PA_OPENING, "OPEN", 3, 4 },   //6
    { PA_OPENING_CURSOR, "OPN_C", 4, 4 },    //7
    { PA_CLOSING, "CLOSE", 3, 4 },   //8
    { PA_CLOSING_CURSOR, "CLS_C", 4, 4 },  //9
    { PA_RANDOM, "RAND", 3, 4 },  //10
    { PA_BLINDS, "BLIND", 7, 4 },  //11
    { PA_DISSOLVE, "DSLVE", 7, 4 },  //12
    { PA_SCROLL_UP, "SC_U", 5, 4 },  //13
    { PA_SCROLL_DOWN, "SC_D", 5, 4 },//14
    { PA_SCROLL_LEFT, "SC_L", 5, 4 },  //15
    { PA_SCROLL_RIGHT, "SC_R", 5, 4 },//16
    { PA_SCROLL_UP_LEFT, "SC_UL", 7, 4 }, //17
    { PA_SCROLL_UP_RIGHT, "SC_UR", 7, 4 }, //18
    { PA_SCROLL_DOWN_LEFT, "SC_DL", 7, 4 }, //19
    { PA_SCROLL_DOWN_RIGHT, "SC_DR", 7, 4 },//20
    { PA_SCAN_HORIZ, "SCANH", 4, 4 },  //21
    { PA_SCAN_VERT, "SCANV", 3, 4 }, //22
    { PA_GROW_UP, "GRW_U", 7, 4 }, //23
    { PA_GROW_DOWN, "GRW_D", 7, 4 }, //24
};

struct ShowTask
{
    bool active;
    bool is_enabled;
    float t_from;
    float t_to;
    bool is_creeping;

    textPosition_t t_pos;
    uint16_t t_speed;
    uint16_t t_pause;
    textEffect_t  t_effectBegin;
    textEffect_t  t_effectEnd;

    String str;
};

ShowTask show_tasks[] =
{
    // time
    {true, true, 0, 24, false, PA_CENTER, 30, 5000, PA_FADE, PA_FADE, "TIME" },
    // text 1
    {false, true, 0, 24, true, PA_LEFT, 30, 0, PA_SCROLL_LEFT, PA_SCROLL_DOWN, "TXT0" },
    // date
    {false, true, 0, 24, true, PA_CENTER, 30, 0, PA_SCROLL_LEFT, PA_SCROLL_LEFT, "DATE" },
    // text 2
    {false, true, 0, 24, true, PA_CENTER, 30, 5000, PA_FADE, PA_FADE, "TXT1" },
    // text 3
    {false, false, 0, 24, false, PA_CENTER, 30, 5000, PA_FADE, PA_FADE, "TXT2" },
    // text 4
    {false, false, 0, 24, false, PA_CENTER, 30, 5000, PA_FADE, PA_FADE, "TXT3" },
};

uint8_t task_index = 0;
bool task_changed = true;

const char* day_ru[] PROGMEM = {"Воскресенье", "Понедельник", "Вторник", "Среда", "Четверг", "Пятница", "Суббота"};
const char* month_ru[] PROGMEM = {"Января", "Февраля", "Марта", "Апреля", "Мая", "Июня", "Июля", "Августа", "Сентября", "Октября", "Ноября", "Декабря"};

String jsonConfig = "{}";

String strText0 = "Have a nice day!", strText1 = "", strText2 = "", strText3 = "";
bool isTxtOn0 = true, isTxtOn1 = false, isTxtOn2 = false, isTxtOn3 = false;
bool isCrLine0 = true, isCrLine1 = false, isCrLine2 = false, isCrLine3 = false;
float txtFrom0 = 0, txtFrom1 = 0, txtFrom2 = 0, txtFrom3 = 0;
float txtTo0 = 24, txtTo1 = 24, txtTo2 = 24, txtTo3 = 24;
float clockFrom = 0, dateFrom = 0;
float clockTo = 24, dateTo = 24;

//Setup for LED
uint8_t fontUsed = 0; //fonts
uint8_t dmodefrom = 8, dmodeto = 20; //DAY MODE
uint8_t brightd = 5, brightn = 1; //brightness day and night
uint16_t speedTicker = 9; // speed of creeping line
bool isLedClock = true;
bool isLedDate = true;
float global_start = 0;
float global_stop = 24; //Working time

ESP8266WebServer HTTP;


// off wifi after 300 seconds = 5 min
int seconds_before_disable_wifi = 300;
bool wifi_disabled = false;


//predefine functions
void WIFI_init();
bool compTimeInt(float tFrom, float tTo, float tNow);
void next_mode_show();

void rtc_read_date_time();
String get_time_string(const DateTime& d);
String get_date_string(const DateTime& d);

void update_show_tasks();

bool loadConfig();
bool saveConfig();
String getContentType(String filename);
bool handleFileRead(String path);
void FS_init(void);

void Display_init(void);
void setFont(int font_index);
void displayTime(bool lastShow);
bool showText(String sText, textPosition_t t_pos, uint16_t t_speed, uint16_t t_pause, textEffect_t  t_effectBegin, textEffect_t  t_effectEnd);
bool showText(String sText, textPosition_t t_pos, uint16_t t_speed, uint16_t t_pause, textEffect_t  t_effectBegin, textEffect_t  t_effectEnd);
void set_day_night_brightness();

void server_httpinit(void);
void handle_config();
void handle_config_json();
void handle_Restart();
void handle_resetConfig();

String get_time_string(const DateTime& d)
{
    return String(d.hour()) + ":" + (d.minute() < 10 ? "0" + String(d.minute()) : String(d.minute()));
}

String get_date_string(const DateTime& d)
{
    String Date = String(d.day()) + " " + month_ru[d.month() - 1] + " " + String(d.year()) + ", " + day_ru[d.dayOfTheWeek()];
    Date.toLowerCase();
    return Date;
}

bool compTimeInt(float tFrom, float tTo, float tNow)  //Comparing time for proper processing from 18.00 to 8.00
{
    if (tFrom < tTo)
    {
        if ((tFrom <= tNow) && (tTo >= tNow))
        {
            return true;
        }
        else
        {
            return false;
        }
    }
    else
        if (tFrom > tTo)
        {
            if (tNow <= 23.59 && tFrom <= tNow)
            {
                return true;
            }
            else
                if (tNow >= 0 && tTo >= tNow)
                {
                    return true;
                }
                else
                {
                    return false;
                }
        }
        else
        {
            return false;
        }
}

void Display_init(void)
{
    P.begin();
    P.setIntensity(1);
    setFont(fontUsed);
    P.setInvert(false);
}

void setFont(int font_index)
{
    switch (font_index)
    {
        case 0:
            P.setFont(_6bite_rus);
            break;
        case 1:
            P.setFont(_5bite_rus);
            break;
        case 2:
            P.setFont(_font_rus);
            break;
        case 3:
            P.setFont(_msx_rus);
            break;
        default:
            P.setFont(_6bite_rus);
    }
}

bool animation_started = false;
// return true if animating
bool showText()
{
    if (!animation_started)
    {
        animation_started = true;
        P.displayReset();
        P.displayText(
            show_tasks[task_index].str.c_str(),
            show_tasks[task_index].t_pos,
            show_tasks[task_index].t_speed,
            show_tasks[task_index].t_pause,
            show_tasks[task_index].t_effectBegin,
            show_tasks[task_index].t_effectEnd);
        return true;
    }
    if (P.displayAnimate())
    {
        animation_started = false;
        return false; // animation finished
    }
    return true; // animation in progress
}

void server_httpinit(void)
{
    HTTP.on("/configs.json", handle_config_json); // create configs.json for sending to WEB interface
    HTTP.on("/setconfig", handle_config);
    HTTP.on("/restart", handle_Restart);   // reset ESP
    HTTP.on("/resetConfig", handle_resetConfig);
    httpUpdater.setup(&HTTP);    // Добавляем функцию Update для перезаписи прошивки по WiFi при 4М(1M SPIFFS) и выше
    HTTP.begin();                // Запускаем HTTP сервер
}

void handle_config()
{
    Serial.println("Receiving new json config");
    if (HTTP.hasArg("plain") == false)
    {
        HTTP.send(200, "text/plain", "Body not received");
        return;
    }
    String root = HTTP.arg("plain");
    Serial.println(root);
    HTTP.send(200, "text/plain", "OK");

    DynamicJsonDocument doc(5096);//4096
    DeserializationError error =  deserializeJson(doc, root);
    if (error)
    {
        Serial.print(F("deserializeJson() failed with code "));
        Serial.println(error.c_str());
        return;
    }

    clockFrom = doc["clockFrom"].as<float>();
    clockTo = doc["clockTo"].as<float>();
    dateFrom = doc["dateFrom"].as<float>();
    dateTo = doc["dateTo"].as<float>();

    isLedClock = doc["isLedClock"].as<bool>();
    isLedDate = doc["isLedDate"].as<bool>();

    strText0 = doc["ledText0"].as<String>();
    strText1 = doc["ledText1"].as<String>();
    strText2 = doc["ledText2"].as<String>();
    strText3 = doc["ledText3"].as<String>();

    isTxtOn0 = doc["isTxtOn0"].as<bool>();
    isTxtOn1 = doc["isTxtOn1"].as<bool>();
    isTxtOn2 = doc["isTxtOn2"].as<bool>();
    isTxtOn3 = doc["isTxtOn3"].as<bool>();

    txtFrom0 = doc["txtFrom0"].as<float>();
    txtFrom1 = doc["txtFrom1"].as<float>();
    txtFrom2 = doc["txtFrom2"].as<float>();
    txtFrom3 = doc["txtFrom3"].as<float>();

    txtTo0 = doc["txtTo0"].as<float>();
    txtTo1 = doc["txtTo1"].as<float>();
    txtTo2 = doc["txtTo2"].as<float>();
    txtTo3 = doc["txtTo3"].as<float>();

    isCrLine0 = doc["isCrLine0"].as<bool>();
    isCrLine1 = doc["isCrLine1"].as<bool>();
    isCrLine2 = doc["isCrLine2"].as<bool>();
    isCrLine3 = doc["isCrLine3"].as<bool>();

    global_start = doc["global_start"].as<float>();
    global_stop = doc["global_stop"].as<float>();

    fontUsed = doc["fontUsed"].as<int>();
    brightd = doc["brightd"].as<int>();
    brightn = doc["brightn"].as<int>();
    speedTicker = 28 - doc["speed_d"].as<int>();

    dmodefrom = doc["dmodefrom"].as<int>();
    dmodeto = doc["dmodeto"].as<int>();

    setFont(fontUsed);

    uint8_t new_h = doc["time_h"].as<int>();
    uint8_t new_m = doc["time_m"].as<int>();
    uint8_t new_s = doc["time_s"].as<int>();
    uint8_t new_day = doc["date_day"].as<int>();
    uint8_t new_month = doc["date_month"].as<int>();
    uint16_t new_year = doc["date_year"].as<int>();

    DS3231 rtc;

    if (new_year > 2000)
    {
        new_year = new_year - 2000;
    }

    rtc.setClockMode(false);  // set to 24h
    rtc.setYear(new_year);
    rtc.setMonth(new_month);
    rtc.setDate(new_day);
    rtc.setHour(new_h);
    rtc.setMinute(new_m);
    rtc.setSecond(new_s);
    rtc.enableOscillator(true, false, 0); // enable osc on battery, without external power
    delay(20)    ;
    saveConfig();
}

void handle_config_json()
{
    P.displaySuspend(true);
    String root = "{}";
    DynamicJsonDocument jsonDoc(5096);//4096
    DeserializationError error =  deserializeJson(jsonDoc, root);
    if (error)
    {
        Serial.print(F("deserializeJson() failed with code "));
        Serial.println(error.c_str());
        return;
    }
    JsonObject json = jsonDoc.as<JsonObject>();
    json["isLedClock"] = (isLedClock ? "checked" : "");
    json["isLedDate"] = (isLedDate ? "checked" : "");
    json["clockFrom"] = clockFrom;
    json["clockTo"] = clockTo;
    json["dateFrom"] = dateFrom;
    json["dateTo"] = dateTo;
    json["ledText0"] = strText0;
    json["ledText1"] = strText1;
    json["ledText2"] = strText2;
    json["ledText3"] = strText3;
    json["isTxtOn0"] = (isTxtOn0 ? "checked" : "");
    json["isTxtOn1"] = (isTxtOn1 ? "checked" : "");
    json["isTxtOn2"] = (isTxtOn2 ? "checked" : "");
    json["isTxtOn3"] = (isTxtOn3 ? "checked" : "");
    json["txtFrom0"] = txtFrom0;
    json["txtFrom1"] = txtFrom1;
    json["txtFrom2"] = txtFrom2;
    json["txtFrom3"] = txtFrom3;
    json["txtTo0"] = txtTo0;
    json["txtTo1"] = txtTo1;
    json["txtTo2"] = txtTo2;
    json["txtTo3"] = txtTo3;
    json["isCrLine0"] = (isCrLine0 ? "checked" : "");
    json["isCrLine1"] = (isCrLine1 ? "checked" : "");
    json["isCrLine2"] = (isCrLine2 ? "checked" : "");
    json["isCrLine3"] = (isCrLine3 ? "checked" : "");
    json["global_start"] = global_start;
    json["global_stop"] = global_stop;
    json["fontUsed"] = fontUsed;
    json["brightd"] = brightd;
    json["brightn"] = brightn;
    json["speed_d"] = 28 - speedTicker;
    json["dmodefrom"] = dmodefrom;
    json["dmodeto"] = dmodeto;
    root = "";
    serializeJson(json, root);
    HTTP.send(200, "text/json", root);
    P.displaySuspend(false);
}

// перезагрузка по команде со страницы
void handle_Restart()
{
    Serial.println("Resetting...");
    String restart = HTTP.arg("device");
    if (restart == "ok")
    {
        HTTP.send(200, "text/plain", "Reset OK");
        ESP.restart();
    }
    else
    {
        HTTP.send(200, "text/plain", "No Reset");
    }
}

// удаление конфига и перезагрузка
void handle_resetConfig()
{
    String restart = HTTP.arg("device");
    if (restart == "ok")
    {
        LittleFS.remove("/myconfig.json");
        Serial.println("ESP erase Config file");
        delay(3000);
        HTTP.send(200, "text/plain", "OK");
        delay(3000);
        ESP.restart();
    }
}

String getContentType(String filename)
{
    if (HTTP.hasArg("download"))
    {
        return "application/octet-stream";
    }
    else
        if (filename.endsWith(".htm"))
        {
            return "text/html";
        }
        else
            if (filename.endsWith(".html"))
            {
                return "text/html";
            }
            else
                if (filename.endsWith(".json"))
                {
                    return "application/json";
                }
                else
                    if (filename.endsWith(".css"))
                    {
                        return "text/css";
                    }
                    else
                        if (filename.endsWith(".js"))
                        {
                            return "application/javascript";
                        }
                        else
                            if (filename.endsWith(".png"))
                            {
                                return "image/png";
                            }
                            else
                                if (filename.endsWith(".gif"))
                                {
                                    return "image/gif";
                                }
                                else
                                    if (filename.endsWith(".jpg"))
                                    {
                                        return "image/jpeg";
                                    }
                                    else
                                        if (filename.endsWith(".ico"))
                                        {
                                            return "image/x-icon";
                                        }
                                        else
                                            if (filename.endsWith(".xml"))
                                            {
                                                return "text/xml";
                                            }
                                            else
                                                if (filename.endsWith(".pdf"))
                                                {
                                                    return "application/x-pdf";
                                                }
                                                else
                                                    if (filename.endsWith(".zip"))
                                                    {
                                                        return "application/x-zip";
                                                    }
                                                    else
                                                        if (filename.endsWith(".gz"))
                                                        {
                                                            return "application/x-gzip";
                                                        }
    return "text/plain";
}

bool handleFileRead(String path)
{
    if (path.endsWith("/"))
    {
        path += "index.htm";
    }
    String contentType = getContentType(path);
    String pathWithGz = path + ".gz";
    if (LittleFS.exists(pathWithGz) || LittleFS.exists(path))
    {
        if (LittleFS.exists(pathWithGz))
        {
            path += ".gz";
        }
        File file = LittleFS.open(path, "r");
        size_t sent = HTTP.streamFile(file, contentType);
        file.close();
        return true;
    }
    return false;
}

void FS_init(void)
{
    if (!LittleFS.begin())  // Initialize SPIFFS
    {
        Serial.println("An Error has occurred while mounting SPIFFS");
    }
    HTTP.serveStatic("/", LittleFS, "/index.html");
    HTTP.onNotFound([]()
    {
        if (!handleFileRead(HTTP.uri()))
        {
            HTTP.send(404, "text/plain", "FileNotFound");
        }
    });
}

bool loadConfig()
{
    File configFile = LittleFS.open("/myconfig.json", "r");
    if (!configFile)    // not found
    {
        Serial.println(F("Failed to open config file"));
        saveConfig();   //  create file
        configFile.close();
        return false;
    }
    size_t size = configFile.size();   // myconfig.json must be less 2048 byte
    if (size > 2048)
    {
        Serial.println(F("Config file size is too large"));
        configFile.close();
        return false;
    }
    jsonConfig = configFile.readString(); // load config
    configFile.close();
    DynamicJsonDocument jsonDoc(5096); //4096
    DeserializationError error = deserializeJson(jsonDoc, jsonConfig);
    if (error)
    {
        Serial.print(F("deserializeJson() failed with code "));
        Serial.println(error.c_str());
        return false;
    }
    JsonObject root = jsonDoc.as<JsonObject>();
    strText0 = root["ledText0"].as<String>();
    strText1 = root["ledText1"].as<String>();
    strText2 = root["ledText2"].as<String>();
    strText3 = root["ledText3"].as<String>();
    isTxtOn0 = root["isTxtOn0"];
    isTxtOn1 = root["isTxtOn1"];
    isTxtOn2 = root["isTxtOn2"];
    isTxtOn3 = root["isTxtOn3"];
    txtFrom0 = root["txtFrom0"];
    txtFrom1 = root["txtFrom1"];
    txtFrom2 = root["txtFrom2"];
    txtFrom3 = root["txtFrom3"];
    txtTo0 = root["txtTo0"];
    txtTo1 = root["txtTo1"];
    txtTo2 = root["txtTo2"];
    txtTo3 = root["txtTo3"];
    isCrLine0 = root["isCrLine0"];
    isCrLine1 = root["isCrLine1"];
    isCrLine2 = root["isCrLine2"];
    isCrLine3 = root["isCrLine3"];
    global_start = root["global_start"];
    global_stop = root["global_stop"];
    fontUsed = root["fontUsed"];
    brightd = root["brightd"];
    brightn = root["brightn"];
    dmodefrom = root["dmodefrom"];
    dmodeto = root["dmodeto"];
    speedTicker = root["speed_d"];
    isLedClock = root["isLedClock"];
    isLedDate = root["isLedDate"];
    return true;
}

// Write to myconfig.json
bool saveConfig()
{
    DynamicJsonDocument jsonDoc(5096);//4096
    DeserializationError error = deserializeJson(jsonDoc, jsonConfig);
    if (error)
    {
        Serial.print(F("deserializeJson() failed with code "));
        Serial.println(error.c_str());
        return false;
    }
    JsonObject json = jsonDoc.as<JsonObject>();
    json["clockFrom"] = clockFrom;
    json["clockTo"] = clockTo;
    json["dateFrom"] = dateFrom;
    json["dateTo"] = dateTo;
    json["ledText0"] = strText0;
    json["ledText1"] = strText1;
    json["ledText2"] = strText2;
    json["ledText3"] = strText3;
    json["isTxtOn0"] = isTxtOn0;
    json["isTxtOn1"] = isTxtOn1;
    json["isTxtOn2"] = isTxtOn2;
    json["isTxtOn3"] = isTxtOn3;
    json["txtFrom0"] = txtFrom0;
    json["txtFrom1"] = txtFrom1;
    json["txtFrom2"] = txtFrom2;
    json["txtFrom3"] = txtFrom3;
    json["txtTo0"] = txtTo0;
    json["txtTo1"] = txtTo1;
    json["txtTo2"] = txtTo2;
    json["txtTo3"] = txtTo3;
    json["isCrLine0"] = isCrLine0;
    json["isCrLine1"] = isCrLine1;
    json["isCrLine2"] = isCrLine2;
    json["isCrLine3"] = isCrLine3;
    json["global_start"] = global_start;
    json["global_stop"] = global_stop;
    json["fontUsed"] = fontUsed;
    json["brightd"] = brightd;
    json["brightn"] = brightn;
    json["dmodefrom"] = dmodefrom;
    json["dmodeto"] = dmodeto;
    json["speed_d"] = speedTicker;
    json["isLedClock"] = isLedClock;
    json["isLedDate"] = isLedDate;
    serializeJson(json, jsonConfig);
    File configFile = LittleFS.open("/myconfig.json", "w");
    if (!configFile)
    {
        Serial.println(F("Failed to open config file for writing"));
        configFile.close();
        return false;
    }
    serializeJson(json, configFile);
    configFile.close();
    return true;
}

// уникальное имя точки доступа по 4 последним символам mac адреса
String ap_uniq_name()
{
    // copy the mac address to a byte array
    uint8_t mac[WL_MAC_ADDR_LENGTH];
    WiFi.softAPmacAddress(mac);

    // format the last two digits to hex character array, like 0A0B
    char macID[15];
    sprintf(macID, "LWCLOCK-%02X%02X", mac[WL_MAC_ADDR_LENGTH - 2], mac[WL_MAC_ADDR_LENGTH - 1]);

    // convert the character array to a string
    String macIdString {macID};
    macIdString.toUpperCase();
    Serial.print("AP name: ");
    Serial.println(macIdString);
    return macIdString;
}

// подготовка WiFi
void Wifi_init()
{
    WiFi.disconnect();
    delay(100);
    WiFi.mode(WIFI_AP);
    WiFi.softAPConfig(IPAddress(192, 168, 4, 1), IPAddress(192, 168, 4, 10), IPAddress(255, 255, 255, 0));

    WiFi.softAP(ap_uniq_name(), "31415926");

    delay(100);
}

// выключение WiFi после заданного времени
void check_wifi_disable()
{
    if (!wifi_disabled)
    {
        if ((millis()/1000)>seconds_before_disable_wifi)
        {
            wifi_disabled=true;
            WiFi.softAPdisconnect(true);
            WiFi.mode(WIFI_OFF);
        }
    }
}

// чтение времени из RTC
void rtc_read_date_time()
{
    now = RTClib::now();
    nowtime = float(now.hour()) + float(now.minute()) / 100;
}

void setup()
{
    delay(250);
    Serial.begin(115200);
    delay(250);
    Wire.begin();
    FS_init();
    loadConfig();
    Display_init();
    Wifi_init();

    String sText = "  " + WiFi.softAPIP().toString();
    P.displayScroll(sText.c_str(), PA_LEFT, PA_SCROLL_LEFT, 20);
    Serial.println(sText);

    server_httpinit();

    for (uint8_t i = 0; i < ARRAY_SIZE(catalog); i++)
    {
        //catalog[i].speed *= P.getSpeed();
        //catalog[i].speed *= speedTicker;
        catalog[i].pause *= 500;
    }
    while (!P.displayAnimate())
    {
        delay(10);
    }

    rtc_read_date_time();
    set_day_night_brightness();
    update_show_tasks();

    Serial.println(get_time_string(now));
    Serial.print("millis=");
    Serial.println(millis());
}

void update_show_tasks()
{
    rnd = random(0, ARRAY_SIZE(catalog));
    show_tasks[0].is_enabled = isLedClock && compTimeInt(clockFrom, clockTo, nowtime);
    show_tasks[0].t_from = clockFrom;
    show_tasks[0].t_to = clockTo;
    show_tasks[0].is_creeping = false;
    show_tasks[0].t_pos = PA_CENTER;
    show_tasks[0].t_speed = catalog[rnd].speed * speedTicker;
    show_tasks[0].t_pause = 5000;
    show_tasks[0].t_effectBegin = catalog[rnd].effect;
    show_tasks[0].t_effectEnd = catalog[rnd].effect;
    show_tasks[0].str = get_time_string(now);

    rnd = random(0, ARRAY_SIZE(catalog));
    show_tasks[1].is_enabled = isTxtOn0 && compTimeInt(txtFrom0, txtTo0, nowtime);
    show_tasks[1].t_from = txtFrom0;
    show_tasks[1].t_to = txtTo0;
    show_tasks[1].is_creeping = isCrLine0;
    show_tasks[1].t_pos = (isCrLine0) ? PA_LEFT : PA_CENTER;
    show_tasks[1].t_speed = ((isCrLine0)) ? (5 * speedTicker) : (catalog[rnd].speed * speedTicker);
    show_tasks[1].t_pause = (isCrLine0) ? 0 : (catalog[rnd].pause * 3);
    show_tasks[1].t_effectBegin = ((isCrLine0)) ? PA_SCROLL_LEFT : catalog[rnd].effect;
    show_tasks[1].t_effectEnd = ((isCrLine0)) ? PA_SCROLL_LEFT : catalog[rnd].effect;
    show_tasks[1].str = strText0;

    rnd = random(0, ARRAY_SIZE(catalog));
    show_tasks[2].is_enabled = isLedDate  && compTimeInt(dateFrom, dateTo, nowtime);
    show_tasks[2].t_from = dateFrom;
    show_tasks[2].t_to = dateTo;
    show_tasks[2].is_creeping = true;
    show_tasks[2].t_pos = PA_LEFT;
    show_tasks[2].t_speed = 5 * speedTicker;
    show_tasks[2].t_pause = 0;
    show_tasks[2].t_effectBegin = PA_SCROLL_LEFT;
    show_tasks[2].t_effectEnd = PA_SCROLL_LEFT;
    show_tasks[2].str = get_date_string(now);

    rnd = random(0, ARRAY_SIZE(catalog));
    show_tasks[3].is_enabled = isTxtOn1  && compTimeInt(txtFrom1, txtTo1, nowtime);
    show_tasks[3].t_from = txtFrom1;
    show_tasks[3].t_to = txtTo1;
    show_tasks[3].is_creeping = isCrLine1;
    show_tasks[3].t_pos = (isCrLine1) ? PA_LEFT : PA_CENTER;
    show_tasks[3].t_speed = ((isCrLine1)) ? (5 * speedTicker) : (catalog[rnd].speed * speedTicker);
    show_tasks[3].t_pause = (isCrLine1) ? 0 : (catalog[rnd].pause * 3);
    show_tasks[3].t_effectBegin = ((isCrLine1)) ? PA_SCROLL_LEFT : catalog[rnd].effect;
    show_tasks[3].t_effectEnd = ((isCrLine1)) ? PA_SCROLL_LEFT : catalog[rnd].effect;
    show_tasks[3].str = strText1;

    rnd = random(0, ARRAY_SIZE(catalog));
    show_tasks[4].is_enabled = isTxtOn2 && compTimeInt(txtFrom2, txtTo2, nowtime);
    show_tasks[4].t_from = txtFrom2;
    show_tasks[4].t_to = txtTo2;
    show_tasks[4].is_creeping = isCrLine2;
    show_tasks[4].t_pos = (isCrLine2) ? PA_LEFT : PA_CENTER;
    show_tasks[4].t_speed = ((isCrLine2)) ? (5 * speedTicker) : (catalog[rnd].speed * speedTicker);
    show_tasks[4].t_pause = (isCrLine2) ? 0 : (catalog[rnd].pause * 3);
    show_tasks[4].t_effectBegin = ((isCrLine2)) ? PA_SCROLL_LEFT : catalog[rnd].effect;
    show_tasks[4].t_effectEnd = ((isCrLine2)) ? PA_SCROLL_LEFT : catalog[rnd].effect;
    show_tasks[4].str = strText2;

    rnd = random(0, ARRAY_SIZE(catalog));
    show_tasks[5].is_enabled = isTxtOn3 && compTimeInt(txtFrom3, txtTo3, nowtime);
    show_tasks[5].t_from = txtFrom3;
    show_tasks[5].t_to = txtTo3;
    show_tasks[5].is_creeping = isCrLine3;
    show_tasks[5].t_pos = (isCrLine3) ? PA_LEFT : PA_CENTER;
    show_tasks[5].t_speed = ((isCrLine3)) ? (5 * speedTicker) : (catalog[rnd].speed * speedTicker);
    show_tasks[5].t_pause = (isCrLine3) ? 0 : (catalog[rnd].pause * 3);
    show_tasks[5].t_effectBegin = ((isCrLine3)) ? PA_SCROLL_LEFT : catalog[rnd].effect;
    show_tasks[5].t_effectEnd = ((isCrLine3)) ? PA_SCROLL_LEFT : catalog[rnd].effect;
    show_tasks[5].str = strText3;
}

// установка яркости по времени
void set_day_night_brightness()
{
    if (compTimeInt(dmodefrom, dmodeto, nowtime))
    {
        P.setIntensity(brightd);
    }
    else
    {
        P.setIntensity(brightn);
    }
}

void loop()
{
    delay(10);
    HTTP.handleClient();
    yield();

    EVERY_N_SECONDS(1)
    {
        // каждую секунду считываем время из RTC
        rtc_read_date_time();
    }

    EVERY_N_SECONDS(30)
    {
        // проверка разрешения работы WIFI
        check_wifi_disable();
        // обновления уровня яркости
        set_day_night_brightness(); // check time for two brightness mode
    }

    if (task_changed)
    {
        task_changed = false;
        update_show_tasks();
    }

    if (compTimeInt(global_start, global_stop, nowtime))   // Global check the time of display data
    {
        if (show_tasks[task_index].is_enabled)
        {
            if (!showText())
            {
                task_index = (task_index + 1) % ARRAY_SIZE(show_tasks);
                task_changed = true;
            }
        }
        else
        {
            task_index = (task_index + 1) % ARRAY_SIZE(show_tasks);
            task_changed = true;
        }
    }
    else
    {
        P.displayReset();
        P.displayClear();
    }

}
