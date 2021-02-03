#include "OLedSPI.h" 
#include <WiFiManager.h> // https://github.com/tzapu/WiFiManager
#include <ArduinoJson.h>
#include <ESP8266HTTPClient.h> 
#include <Ticker.h>
#include <simpleDSTadjust.h>
#include <EEPROM.h>
#include <sys/time.h>                   // struct timeval
#include <time.h>               

Ticker ticker;
Ticker shaker;

tm* crt;
enum dis
{
    date = 0,
    bfans,
    weather,
    weather1,
    weather2,
};

dis mode = date;
int motiontye = 0;
int fonttye = 0;
OLedSPI oled;
bool shouldSaveConfig = false;
void ICACHE_RAM_ATTR keyHandle();
void ICACHE_RAM_ATTR shakeHandle();
 
#define KEY_MENU 5
#define KEY_SW 4
#define KEY_MODE 0

int shake = 0;
char Address[7] = "110000";
char Key[33] = "ac2f3457cc2d7928a8b4600e9759be1a";
char Bid[20] = "49890113";

u16 ConvertWeatherNum(String data_content)
{
  int i;
  String weather_phenomena1[] = { "晴", "少云", "晴间多云", "多云", "阴", "有风", "平静", "微风", "和风", "清风", "强风/劲风", "疾风", "大风", "烈风", "风暴",
    "狂爆风", "飓风", "热带风暴", "阵雨", "雷阵雨", "雷阵雨并伴有冰雹", "小雨", "中雨", "大雨", "暴雨", "大暴雨", "特大暴雨",
    "强阵雨", "强雷阵雨", "极端降雨", "毛毛雨/细雨", "雨", "小雨-中雨", "中雨-大雨", "大雨-暴雨", "暴雨-大暴雨", "大暴雨-特大暴雨",
    "雨雪天气", "雨夹雪", "阵雨夹雪", "冻雨", "雪", "阵雪", "小雪", "中雪", "大雪", "暴雪", "小雪-中雪", "中雪-大雪", "大雪-暴雪",
    "浮尘", "扬沙", "沙尘暴", "强沙尘暴", "龙卷风", "雾", "浓雾", "强浓雾", "轻雾", "大雾", "特强浓雾", "霾", "中度霾", "重度霾",
    "严重霾", "热", "冷", "未知" };
  for (i = 0; i < 68; i++) {

    //Serial.println(data_content+" "+ weather_phenomena1[i]);

    if (data_content.equals(weather_phenomena1[i])) {
      return i;
    }
  }
}

String ConvertWeather(String data_content)
{
  int i;
  String weather_phenomena1[] = { "晴", "少云", "晴间多云", "多云", "阴", "有风", "平静", "微风", "和风", "清风", "强风/劲风", "疾风", "大风", "烈风", "风暴",
    "狂爆风", "飓风", "热带风暴", "阵雨", "雷阵雨", "雷阵雨并伴有冰雹", "小雨", "中雨", "大雨", "暴雨", "大暴雨", "特大暴雨",
    "强阵雨", "强雷阵雨", "极端降雨", "毛毛雨/细雨", "雨", "小雨-中雨", "中雨-大雨", "大雨-暴雨", "暴雨-大暴雨", "大暴雨-特大暴雨",
    "雨雪天气", "雨夹雪", "阵雨夹雪", "冻雨", "雪", "阵雪", "小雪", "中雪", "大雪", "暴雪", "小雪-中雪", "中雪-大雪", "大雪-暴雪",
    "浮尘", "扬沙", "沙尘暴", "强沙尘暴", "龙卷风", "雾", "浓雾", "强浓雾", "轻雾", "大雾", "特强浓雾", "霾", "中度霾", "重度霾",
    "严重霾", "热", "冷", "未知" };
  for (i = 0; i < 68; i++) {

    if (data_content.equals(weather_phenomena1[i])) {
      break;
    }
  }
  if (i == 0) {
    return "Sunny";  //晴
  }
  else if (i >= 1 && i <= 3) {
    return "Cloudy";  //多云
  }
  else if (i == 4) {
    return "Cast";   //阴
  }
  else if (i >= 5 && i <= 17) {
    return "Gale";    //风
  }
  else if (i >= 18 && i <= 40) {
    return "Rain";   //雨
  }
  else if (i >= 41 && i <= 49) {
    return "Snow";   //雪
  }
  else if (i >= 50 && i <= 54) {
    return "Dust";  //沙尘
  }
  else if (i >= 55 && i <= 60) {
    return "Fog";   //雾
  }
  else if (i >= 61 && i <= 64) {
    return "Haze";   //霾
  }
  else {
    return "Unknown";   //未知
  }
}

String ConvertWindDir(String data_content)
{
  int i;
  String direction_phenomena1[] = { "无风向", "东北", "东", "东南", "南", "西南", "西", "西北", "北", "旋转不定", "未知" };
  for (i = 0; i < 11; i++) {
    if (data_content.equals(direction_phenomena1[i])) {
      break;
    }
  }
  switch (i)
  {
  case 0:return "Breezeless";
  case 1:return "Northeast";
  case 2:return "East";
  case 3:return "Southeast";
  case 4:return "South";
  case 5:return "Southwest";
  case 6:return "West";
  case 7:return "Northwest";
  case 8:return "North";
  case 9:return "Uncertain";
  case 10:return "Unknown";
  }
}

String now_weather;    //当前天气
String now_temperature;    //当前温度
String now_temperatureunit;    //当前温度
String now_humidity;    //当前温度
String now_humidityunit;    //当前温度
String now_wind_direction;   //当前风向
String now_wind_power;   //当前风力
String now_reporttime;   //当前风力
String display_wind_power;
//****赋值
//第一天数据
String first_week;
String first_dayweather;
String first_daytemp;
String first_nighttemp;
//第二天数据
String second_week;
String second_dayweather;
String second_daytemp;
String second_nighttemp;
//第三天数据
String third_week;
String third_dayweather;
String third_daytemp;
String third_nighttemp;
String Allweathermsg;
//****获取天气子函数 
void get_weather() {
  if (WiFi.status() == WL_CONNECTED) { //如果 Wi-Fi 连接成功
    //此处往下是取得实况天气的程序
    HTTPClient http;  //开始登陆 
    //不要使用和下面相同的秘钥http://restapi.amap.com/v3/weather/weatherInfo?parameters&key=ac2f3457cc2d7928a8b4600e9759be1a&city=340207&extensions=base

    const char* HOST = "http://restapi.amap.com/v3/weather";
    String GetUrl = String(HOST) + "/weatherInfo?parameters&key=";
    GetUrl += String(Key);
    GetUrl += "&city=";
    GetUrl += String(Address);
    GetUrl += "&extensions=";

    http.begin(GetUrl + "base");  //高德开放平台提供服务 
    int httpget_now = http.GET(); //赋值
    if (httpget_now > 0) { //检查一下是否为0，应该是去检查缓存区是否为空
      /*数据解析:使用 https://arduinojson.org/assistant/ 一个工具可以直接生成程序，挑有用的复制就行*/
      const size_t capacity = JSON_ARRAY_SIZE(1) + JSON_OBJECT_SIZE(5) + JSON_OBJECT_SIZE(9) + 210;
      DynamicJsonBuffer jsonBuffer(capacity);

      JsonObject& root = jsonBuffer.parseObject(http.getString());

      JsonObject& lives_0 = root["lives"][0];
      const char* lives_0_weather = lives_0["weather"]; // "天气情况"
      const char* lives_0_temperature = lives_0["temperature"]; // "温度"
      const char* lives_0_humidity = lives_0["humidity"]; // "风力"
      const char* lives_0_winddirection = lives_0["winddirection"]; // "风向"
      const char* lives_0_windpower = lives_0["windpower"]; // "风力"
      const char* lives_0_reporttime = lives_0["reporttime"]; // "风力"

      //赋值，因为现在这些变量是在缓存区，一会将被清空
      now_weather = lives_0_weather;    //当前天气
      now_temperature = lives_0_temperature;    //当前温度
      now_temperatureunit = now_temperature + "c";
      now_humidity = lives_0_humidity;    //当前温度
      now_humidityunit = now_humidity + "%";
      now_wind_direction = lives_0_winddirection;   //当前风向
      now_wind_power = lives_0_windpower;   //当前风力
      now_reporttime = lives_0_reporttime;   //当前风力
      now_reporttime = now_reporttime.substring(10, 19);   //当前风力
      display_wind_power = now_wind_power.substring(3, 6);
    }
    http.end();
    delay(50);

    //此处往下是取得未来三天的天气的程序
    http.begin(GetUrl + "all");  //高德开放平台提供服务 
    int httpget_threedays = http.GET(); //赋值 
    if (httpget_threedays > 0) { //检查一下是否为0，应该是去检查缓存区是否为空
      const size_t capacity = JSON_ARRAY_SIZE(1) + JSON_ARRAY_SIZE(4) + 2 * JSON_OBJECT_SIZE(5) + 4 * JSON_OBJECT_SIZE(10) + 690;
      DynamicJsonBuffer jsonBuffer(capacity);
      JsonObject& root = jsonBuffer.parseObject(http.getString());
      JsonObject& forecasts_0 = root["forecasts"][0];
      JsonArray& forecasts_0_casts = forecasts_0["casts"];

      JsonObject& forecasts_0_casts_1 = forecasts_0_casts[1]; //明天天气
      const char* forecasts_0_casts_1_week = forecasts_0_casts_1["week"]; // "1"
      const char* forecasts_0_casts_1_dayweather = forecasts_0_casts_1["dayweather"]; // "多云"
      const char* forecasts_0_casts_1_daytemp = forecasts_0_casts_1["daytemp"]; // "22"
      const char* forecasts_0_casts_1_nighttemp = forecasts_0_casts_1["nighttemp"]; // "10"

      JsonObject& forecasts_0_casts_2 = forecasts_0_casts[2]; //后天天气
      const char* forecasts_0_casts_2_week = forecasts_0_casts_2["week"]; // "2"
      const char* forecasts_0_casts_2_dayweather = forecasts_0_casts_2["dayweather"]; // "晴"
      const char* forecasts_0_casts_2_daytemp = forecasts_0_casts_2["daytemp"]; // "19"
      const char* forecasts_0_casts_2_nighttemp = forecasts_0_casts_2["nighttemp"]; // "10"

      JsonObject& forecasts_0_casts_3 = forecasts_0_casts[3]; //大后天天气
      const char* forecasts_0_casts_3_week = forecasts_0_casts_3["week"]; // "3"
      const char* forecasts_0_casts_3_dayweather = forecasts_0_casts_3["dayweather"]; // "晴"
      const char* forecasts_0_casts_3_daytemp = forecasts_0_casts_3["daytemp"]; // "25"
      const char* forecasts_0_casts_3_nighttemp = forecasts_0_casts_3["nighttemp"]; // "14"

      //****赋值
      //第一天数据
      first_week = forecasts_0_casts_1_week;
      first_dayweather = forecasts_0_casts_1_dayweather;
      first_daytemp = forecasts_0_casts_1_daytemp;
      first_nighttemp = forecasts_0_casts_1_nighttemp;
      //第二天数据
      second_week = forecasts_0_casts_2_week;
      second_dayweather = forecasts_0_casts_2_dayweather;
      second_daytemp = forecasts_0_casts_2_daytemp;
      second_nighttemp = forecasts_0_casts_2_nighttemp;
      //第三天数据
      third_week = forecasts_0_casts_3_week;
      third_dayweather = forecasts_0_casts_3_dayweather;
      third_daytemp = forecasts_0_casts_3_daytemp;
      third_nighttemp = forecasts_0_casts_3_nighttemp;
    }
    http.end();   //关闭与服务器的连接
    delay(50);
  }
  Allweathermsg = "Temp:" + now_temperatureunit + " " + "Humi:" + now_humidityunit + " " + "WindDir:" + ConvertWindDir(now_wind_direction) + " "\
      + "Power:"+ display_wind_power +" "+"Update:"+ now_reporttime;
}

//****获取时间子函数
void get_time() {
    if (WiFi.status() == WL_CONNECTED) { //如果 Wi-Fi 连接成功
        HTTPClient http;  //开始登陆 
        //不要使用和下面相同的秘钥
        http.begin("http://vv.video.qq.com/checktime?otype=json");
        int httpCode = http.GET(); //赋值                               
        if (httpCode > 0) { //检查一下是否为0，应该是去检查缓存区是否为空

            char buff[300];
            sscanf(http.getString().c_str(), "QZOutputJson=%s;", buff);

            const size_t capacity = 128;
            DynamicJsonBuffer jsonBuffer(capacity);
            JsonObject& root = jsonBuffer.parseObject(buff);

            long serverTime = root["t"]; // 1610249263328
            serverTime += 8 * 60 * 60;
            tm* p;
            p = gmtime(&serverTime);
            if (p->tm_year != 0)
            {
                //char s[100]; 
                //strftime(s, sizeof(s), "%Y-%m-%d %H:%M:%S", p);  
                memcpy(&crt, &p, sizeof(p));
            }
        }
        http.end();
        delay(50);
    }
}

String fans;
void get_fans() {
  if (WiFi.status() == WL_CONNECTED) { //如果 Wi-Fi 连接成功
    HTTPClient http;  //开始登陆 
    //不要使用和下面相同的秘钥

    const char* HOST = "http://api.bilibili.com";
    String GetUrl = String(HOST) + "/x/relation/stat?vmid=";
    GetUrl += String(Bid);

    http.begin(GetUrl);
    int httpCode = http.GET();

    //    Serial.println(http.getString());
    if (httpCode == 200) {
      String resBuff = http.getString();
      DynamicJsonBuffer jsonBuffer(200);
      JsonObject& root = jsonBuffer.parseObject(resBuff);

      String msg = root["message"];
      //uint message = msg.toInt();;
      const char* fanstemp = root["data"]["follower"]; 
      fans = fanstemp;
    }
    http.end();
    delay(50);
  }
}
//void Delayms(u16 ms)
//{
//    u16 i, j;
//    u8 k;
//    for (i = 0; i < ms; i++)
//        for (j = 0; j < 0x0500; j++) k++;
//}
void keyHandle() {
  if(shake<1)
  {
    if (digitalRead(KEY_SW) == LOW)
    {
      shake = 10;
      
      if (mode == date)
          mode = bfans;
      else if (mode == bfans)
          mode = weather;
      else if (mode == weather)
          mode = date;
      MotionRun();
    }
    if (digitalRead(KEY_MENU) == LOW)
    {
      shake = 10;
    }
    if (digitalRead(KEY_MODE) == LOW)
    {
        motiontye++;
        if (motiontye > 7)
        {
            motiontye = 0;
            fonttye = 1 - fonttye;
        }

        motiontye %= 8;
      shake = 10;
    }
  }
}

char timestr[40] = "";
char datestr[40] = "";

void tickerHandle() //到时间时需要执行的任务
{
    static int blink = 0;
    if (blink++ % 2)
    {
        crt->tm_sec++;
        if (crt->tm_sec >= 60)
        {
            crt->tm_sec = 0;
            crt->tm_min++;
            if (crt->tm_min >= 60)
            {
                crt->tm_min = 0;
                get_time();
                get_fans();
                get_weather(); 
            }
        }
        strftime(timestr, sizeof(timestr), "%H:%M:%S", crt);
    }
    else
        strftime(timestr, sizeof(timestr), "%H %M %S", crt);
    strftime(datestr, sizeof(datestr), "%Y-%m-%d", crt);
}
void shakeHandle() //到时间时需要执行的任务
{
  if (shake > 0)
    shake--;
}


void configModeCallback(WiFiManager* myWiFiManager) {
  Serial.println("Entered config mode");
  Serial.println(WiFi.softAPIP());
  Serial.println(myWiFiManager->getConfigPortalSSID());
}

void saveConfigCallback() {
  Serial.println("Should save config");
  shouldSaveConfig = true;
}


void setup() {
    WiFi.mode(WIFI_STA); // explicitly set mode, esp defaults to STA+AP
    // put your setup code here, to run once:
    Serial.begin(115200);
    EEPROM.begin(1024);
    oled.init();
    //set led pin as output
    pinMode(KEY_MENU, INPUT_PULLUP);
    pinMode(KEY_SW, INPUT_PULLUP);
    pinMode(KEY_MODE, INPUT_PULLUP);
    // start ticker with 0.5 because we start in AP mode and try to connect
    oled.sendString("Initialization", 0, 0, ' ');
    oled.sendString("Succeed!", 0, 1, ' ');
    delay(1000);
    WiFiManager wm;//reset settings - for testing
    if (digitalRead(KEY_MENU) == LOW)
    {
        wm.resetSettings();
        Serial.println("Reset Config");
        oled.sendString("Reset Config", 0, 0, ' ');
        oled.sendString("", 0, 1, ' ');
        delay(1000);
    }
    wm.setConnectTimeout(60);  // 打印调试内容
    wm.setDebugOutput(false); // 设置最小信号强度
    wm.setMinimumSignalQuality(30); // 设置固定AP信息

    if (EEPROM.read(0))
    {
        for (int i = 0; i < 7; i++)
            Address[i] = EEPROM.read(i);
        for (int i = 7; i < 7 + 20; i++)
            Bid[i - 7] = EEPROM.read(i);
    }

    int Timeout = 0;
    if (wm.getWiFiSSID(false) == "")
        Timeout = 40;
    else
    {
        WiFi.begin(wm.getWiFiSSID(false), wm.getWiFiPass(false));
        Serial.println("Wait");
        oled.sendString("Connecting", 0, 0, ' ');
        oled.sendString("Please Wait", 0, 1, ' ');
        delay(1000);
    }

    while (WiFi.status() != WL_CONNECTED)//WiFi.status() ，这个函数是wifi连接状态，返回wifi链接状态
    {
        Serial.print(".");
        delay(500);
        if (Timeout++ >= 40)
        {
            oled.sendString("Please Connect", 0, 0, ' ');
            oled.sendString("WiFi:FunnyChip", 0, 1, ' ');

            wm.setAPCallback(configModeCallback);
            wm.setSaveConfigCallback(saveConfigCallback);

            WiFiManagerParameter custom_mqtt_Address("address", "WeatherAddress", Address, 7);
            WiFiManagerParameter custom_mqtt_Bid("bilibiliID", "BiliBiliID", Bid, 20);

            wm.addParameter(&custom_mqtt_Bid);
            wm.addParameter(&custom_mqtt_Address);

            Serial.println("link to AP");
            if (!wm.autoConnect("FUNNYCHIP")) {

                Serial.println("failed to connect and hit timeout");
                ESP.restart();
                delay(100);
            }

            if (shouldSaveConfig) {
                oled.sendString("Saveing Config", 0, 0, ' ');
                oled.sendString("Please Wait", 0, 1, ' ');
                strcpy(Address, custom_mqtt_Address.getValue());
                strcpy(Bid, custom_mqtt_Bid.getValue());
                for (int i = 0; i < 7; i++)
                    EEPROM.write(i, Address[i]);
                for (int i = 7; i < 7 + 20; i++)
                    EEPROM.write(i, Bid[i - 7]);

                EEPROM.commit();

                Serial.println("Save OK");
                Serial.println(Address);
                Serial.println(Bid);
            }
        }
    }
    oled.sendString("Connected!", 0, 0, ' ');
    oled.sendString("System Online", 0, 1, ' ');

    attachInterrupt(KEY_MENU, keyHandle, FALLING);
    attachInterrupt(KEY_SW, keyHandle, FALLING);
    attachInterrupt(KEY_MODE, keyHandle, FALLING);

    oled.sendString("Synchronous", 0, 0, ' ');
    oled.sendString("Please Wait!", 0, 1, ' ');
    get_time(); //开机取一次时间
    oled.sendString("Time OK!", 0, 1, ' ');
    get_weather(); //开机取一次天气
    oled.sendString("Weather OK!", 0, 1, ' ');
    get_fans();//获取数据
    oled.sendString("BiliBili OK!", 0, 1, ' ');

    ticker.attach_ms(500, tickerHandle); //初始化调度任务，每1秒执行一次tickerHandle()
    shaker.attach_ms(20, shakeHandle); //初始化调度任务，每10毫秒执行一次shakeHandle()
    oled.sendString("                ", 0, 0);  //Now includes the cursor position data (col, row)
    oled.sendString("                ", 0, 1);  //Now includes the cursor position data (col, row)
    MotionInit();
}
char tempstr[50];
char weektab[][4] = { "SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT", "SUN" };

int Current[10] = { 0 };
int Target[10] = { 0 };

void DampRun()
{
    int i;
    for (i = 0; i < 10; i++)
    {
        if (Current[i] > Target[i])
            Current[i] --;
        else if (Current[i] < Target[i])
            Current[i] ++;
        else
            Current[i] = Target[i];
    }
}

void MotionInit()
{
    Current[1] = 8;
    Current[2] = 8;
    Current[3] = -17;//BiliBili:隐藏
    Current[4] = 16;//BiliBili:隐藏
    Current[5] = -16;
    Current[6] = 5;
    Target[0] = -8;
    Target[1] = 8;
    Target[2] = 8;
    Target[3] = -17;//BiliBili:隐藏
    Target[4] = 16;//BiliBili:隐藏
    Target[5] = -16;
    Target[6] = 5;
    MotionRun();
    Current[0] = -10;
    Current[7] =-9;
    Current[2] =16;
}

void MotionRun()
{
    switch (mode)
    {
    case date:
        Current[1] = 16;
        Current[7] = 0-Current[5]-16;
        Current[2] = Current[7];
        Target[0] = 0;
        Target[1] = 0;
        Target[2] = 0;
        Target[7] = 0;
        
        Current[3] = -24;//BiliBili:隐藏
        Current[4] = 24;//BiliBili:隐藏
        Target[3] = -24;//BiliBili:隐藏
        Target[4] = 24;//BiliBili:隐藏
        Target[5] = -16; 
        Target[6] = 5;
        break;
    case bfans:
        Target[2] = 8;
        Target[3] = 0;
        Target[4] = 0;

        Current[5] = -17;
        Target[5] = -17;
        Target[7] = -17;
        break;
    case weather:
        Target[3] = -9;
        Target[4] = -9-9;
        Target[5] = 0;
        Current[6] = 13;
        Target[6] = 0;

        Target[1] = 8;
        Target[1] = 8;
        Target[2] = 8;
        break;
    }
}
//time_t now;
void loop() {
    static int count = 0;
    static long timecount = 0;
    DampRun();
    oled.clear(' ');
    switch (mode)
    {
    case date:count = 0;
        break;
    case bfans:count = 0;
        break;
    case weather:
        if (count++ > Allweathermsg.length()*8)
        {
            Target[5] = 0;
            count = 0;
        }
        if(count%128==0)
            Target[5] = count/8;
        break;
    }
    oled.display(weektab[crt->tm_mday], Current[2] + 12, 0);
    oled.display(datestr, Current[7] +1, 0);
    oled.display(now_temperatureunit.c_str(), Current[1] + 12, 1);

    oled.display("BiliBili:", Current[3] + 0, 0);
    oled.display(fans.c_str(), Current[4] + 9, 0);

    oled.display(Allweathermsg.c_str(),  0-Current[5], 0);
    oled.display(ConvertWeather(now_weather).c_str(), Current[6] + 11, 1);

    oled.animotion(timestr, motiontye, fonttye, 0);
    oled.refrash_Screen(Current[0] + 2, 1);

    delay(20);
    if (timecount++ > 50 * 600)
    {
        oled.sendString("Synchronous", 0, 0, ' ');
        oled.sendString("Please Wait!", 0, 1, ' ');
        get_time(); //时间
        get_weather(); //天气
        get_fans();//粉丝数
        timecount = 0;
    }
}