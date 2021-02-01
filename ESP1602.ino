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
Ticker binker;
Ticker shaker;

tm* crt;

OLedSPI oled;
bool shouldSaveConfig = false;
bool TimeFlag = false;
bool WeatherFlag = false;
void ICACHE_RAM_ATTR keyHandle();
void ICACHE_RAM_ATTR shakeHandle();
 
#define KEY_MENU 1
#define KEY_SW 2
#define KEY_MODE 3

int keypress = 0;
int shake = 0;

char Address[7] = "110000";
char Key[33] = "ac2f3457cc2d7928a8b4600e9759be1a";
char Bid[20] = "49890113";

#define IntTime     1

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
String now_humidity;    //当前温度
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

//****获取天气子函数 
void get_weather() {
  delay(IntTime);
  if (WiFi.status() == WL_CONNECTED) { //如果 Wi-Fi 连接成功
    //此处往下是取得实况天气的程序
    HTTPClient http;  //开始登陆 
    //不要使用和下面相同的秘钥

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
      now_humidity = lives_0_humidity;    //当前温度
      now_wind_direction = lives_0_winddirection;   //当前风向
      now_wind_power = lives_0_windpower;   //当前风力
      now_reporttime = lives_0_reporttime;   //当前风力
      now_reporttime = now_reporttime.substring(5, 19);   //当前风力
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
    delay(10);
  }
}

int week(int y, int m, int d)
{
  if (m == 1 || m == 2) m += 12, y = y - 1;
  return (d + 2 * m + 3 * (m + 1) / 5 + y + y / 4 - y / 100 + y / 400 + 1) % 7;
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
      sscanf(http.getString().c_str(),"QZOutputJson=%s;",buff);

      const size_t capacity =128;
      DynamicJsonBuffer jsonBuffer(capacity);
      JsonObject& root = jsonBuffer.parseObject(buff);

      long serverTime = root["t"]; // 1610249263328
      serverTime+=8*60*60;
      tm *p;  
      p=gmtime(&serverTime);  
      if(p->tm_year!=0)
      {
        
          char s[100]; 
          strftime(s, sizeof(s), "%Y-%m-%d %H:%M:%S", p);  
          //sprintf(buff,"%d-%s",serverTime,s);
          //Serial.println(buff);
          memcpy(&crt, &p, sizeof(p));
      }
    }
    http.end();
    delay(100);
  }
}
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
      uint message = msg.toInt();;
      String fans = root["data"]["follower"];
    }
    http.end();
  }
}

void keyHandle() {
  if (shake < 1)
  {
    if (digitalRead(KEY_SW) == LOW)
    {
      shake = 15;
      keypress = 1;
    }
    if (digitalRead(KEY_MENU) == LOW)
    {
      shake = 15;
      keypress = 2;
    }
    if (digitalRead(KEY_MODE) == LOW)
    {
      shake = 15;
      keypress = 3;
    }
  }
}

void get_key()
{

  switch (keypress)
  {
  case 1:break;
  case 2:break;
  case 3:break;
  }
  keypress = 0;
}

void tickerHandle() //到时间时需要执行的任务
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
        }
    }
    Serial.println(crt->tm_sec);
}

void shakeHandle() //到时间时需要执行的任务
{
  if (shake > 0)
    shake--;
}


//gets called when WiFiManager enters configuration mode
void configModeCallback(WiFiManager* myWiFiManager) {
  Serial.println("Entered config mode");
  Serial.println(WiFi.softAPIP());
  //if you used auto generated SSID, print it
  Serial.println(myWiFiManager->getConfigPortalSSID());
  //entered config mode, make led toggle faster
}
/**
 * 功能描述：设置点击保存的回调
 */
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
  oled.sendString("Initialization",0,0,' ');
  oled.sendString("Succeed!",0,1,' ');
  delay(1000);
  WiFiManager wm;//reset settings - for testing
  if (digitalRead(KEY_MENU) == LOW)
  {
    wm.resetSettings();
    Serial.println("Reset Config");
    oled.sendString("Reset Config",0,0,' ');
    oled.sendString("",0,1,' ');
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
  if(wm.getWiFiSSID(false)=="")
    Timeout=40;
  else
  {
    WiFi.begin(wm.getWiFiSSID(false), wm.getWiFiPass(false));
    Serial.println("Wait");
    oled.sendString("Connecting",0,0,' ');
    oled.sendString("Please Wait",0,1,' ');
    delay(1000);
  }

  while (WiFi.status() != WL_CONNECTED)//WiFi.status() ，这个函数是wifi连接状态，返回wifi链接状态
  {
    Serial.print(".");
    delay(500);
    if (Timeout++ >= 40)
    {
      oled.sendString("Please Connect",0,0,' ');
      oled.sendString("WiFi:FunnyChip",0,1,' ');

      wm.setAPCallback(configModeCallback);
      wm.setSaveConfigCallback(saveConfigCallback);

      WiFiManagerParameter custom_mqtt_Address("address", "WeatherAddress", Address, 7);
      WiFiManagerParameter custom_mqtt_Bid("bilibiliID", "BiliBiliID", Bid, 20);

      wm.addParameter(&custom_mqtt_Bid);
      wm.addParameter(&custom_mqtt_Address);

      //fetches ssid and pass and tries to connect
      //if it does not connect it starts an access point with the specified name
      //here  "AutoConnectAP"
      //and goes into a blocking loop awaiting configuration
      Serial.println("link to AP");
      if (!wm.autoConnect("FUNNYCHIP")) {

        Serial.println("failed to connect and hit timeout");
        //reset and try again, or maybe put it to deep sleep
        ESP.restart();
        delay(100);
      }

      if (shouldSaveConfig) {
        // 读取配置页面配置好的信息
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
  oled.sendString("Connected!",0,0,' ');
  oled.sendString("System Online",0,1,' ');

  //if you get here you have connected to the WiFi
  ticker.detach();

  attachInterrupt(KEY_MENU, keyHandle, CHANGE);
  attachInterrupt(KEY_SW, keyHandle, CHANGE);
  attachInterrupt(KEY_MODE, keyHandle, CHANGE);

  oled.sendString("Get Time", 0, 0, ' ');
  oled.sendString("NTP", 0, 1, ' ');
  get_time(); //开机取一次时间
  oled.sendString("Get Weather", 0, 0, ' ');
  oled.sendString("Addr:", 0, 1, ' ');
  oled.sendString(Address, 5, 1, ' ');
  get_weather(); //开机取一次天气
  oled.sendString("Get Bilibili", 0, 0, ' ');
  oled.sendString("Bid:", 0, 1, ' ');
  oled.sendString(Bid, 4, 1, ' ');
  get_fans();//获取数据

  binker.attach(1, tickerHandle); //初始化调度任务，每1秒执行一次tickerHandle()
  shaker.attach_ms(10, shakeHandle); //初始化调度任务，每10毫秒执行一次shakeHandle()
  oled.sendString("                ", 0, 0);  //Now includes the cursor position data (col, row)
  oled.sendString("                ", 0, 1);  //Now includes the cursor position data (col, row)
}

char tempstr[50];
char weektab[][4] = { "SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT", "SUN" };
//time_t now;
void loop() {
  // put your main code here, to run repeatedly
  get_key();//检测按键按下情况
  //oled.sendString(weektab[crt->tm_mday],12,0);  //Now includes the cursor position data (col, row)
  //strftime(tempstr, sizeof(tempstr), "%Y-%m-%d",crt);
  //oled.sendString(tempstr, 1, 0);  //Now includes the cursor position data (col, row)

  oled.sendString(ConvertWeather(now_weather).c_str(), 0, 0);
  oled.sendString(now_temperature.c_str(), 8, 0);

  strftime(tempstr, sizeof(tempstr), "%H:%M:%S",crt);
  oled.animotion(tempstr,7,0,0);     
  oled.refrash_Screen();
  delay(30);
}