
#ifdef ESP32
  #include <WiFi.h>
#else
  #include <ESP8266WiFi.h>
#endif
#include <WiFiClientSecure.h>
//#include <TelegramCertificate.h>
#include <UniversalTelegramBot.h>
// Universal Telegram Bot Library written by Brian Lough: https://github.com/witnessmenow/Universal-Arduino-Telegram-Bot
#include <ArduinoJson.h>
#include <EEPROM.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

#define pinWake 14
// Replace with your network credentials
const char* ssid = ";
const char* password = "";

// Initialize Telegram BOT
#define BOTtoken ""  // your Bot Token (Get from Botfather)

// Use @myidbot to find out the chat ID of an individual or a group
// Also note that you need to click "start" on a bot before it can
// message you
#define CHAT_ID ""  

String Time;
#ifdef ESP8266
  X509List cert(TELEGRAM_CERTIFICATE_ROOT);
#endif

WiFiClientSecure client;
UniversalTelegramBot bot(BOTtoken, client);

// Checks for new messages every 1 second.
int botRequestDelay = 500;
unsigned long lastTimeBotRan;


int SendRequestDelay = 2000;   //через скільки секунд надсилати сповіщення
unsigned long lastTimesSend=0;

bool alarm;
int State1=1;


float vout = 0.0;           //
float vin = 0.0;            //
float R1 = 390000.0;          // сопротивление R1 (10K)
float R2 = 99200.0;         // сопротивление R2 (1,5K)
int val=0;
#define analogvolt A0

int adress=1; //номер ячейки памяті
int count=0; 
bool flagVolt=false;

const long utcOffsetInSeconds = 7200;
int Hour;
int Minutes;
int timePause=2;
#define lightSensor 5
#define Switch 12
// Определение NTP-клиента для получения времени
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);


// Handle what happens when you receive new messages
void handleNewMessages(int numNewMessages) {
  Serial.println("handleNewMessages");
  Serial.println(String(numNewMessages));

  for (int i=0; i<numNewMessages; i++) {
    // Chat id of the requester
    String chat_id = String(bot.messages[i].chat_id);
    if (chat_id != CHAT_ID){
      bot.sendMessage(chat_id, "Unauthorized user", "");
      continue;
    }
    
    // Print the received message
    String text = bot.messages[i].text;
    Serial.println(text);

    String from_name = bot.messages[i].from_name;

    if (text.startsWith("/start_ms")) {
      String welcome = "Привіт, " + from_name + ".\n";
      welcome += "Чим я можу допомогти ? \n";
       welcome += "Це набір команд для керування датчика чадного газу: \n";
      welcome += "/ms_on включити сповіщення \n";
      welcome += "/ms_off виключити сповіщення \n";
      welcome += "/ms_state  статус сигналізації  \n";
      welcome += "/ms_volt  Напруга акумулятора  \n";
      welcome += "/ms_time  Пауза  \n";
      bot.sendMessage(chat_id, welcome, "");
    }
   if (text.startsWith("/ms_off")) {
      bot.sendMessage(chat_id, "Датчик руху виключено", "");
      alarm=false;
      EEPROM.put(adress, alarm);   
      EEPROM.commit();
         }
 if (text.startsWith("/ms_on")) {
      bot.sendMessage(chat_id, "Датчик руху включено", "");
      alarm=true;
      EEPROM.put(adress, alarm);   
      EEPROM.commit();
         }
  if (text.startsWith("/ms_volt")) {
      bot.sendMessage(chat_id, "Надсилаю напругу акумулятора", "");
      flagVolt=true;
      Volt();
         }
    
     if (text.startsWith("/ms_time")) {
      
      
      if (text.indexOf(':')!=-1){
          for(int i=text.indexOf(':');i<=text.length();i++)
            Time+=text[i+1];
        }
      Serial.println("Pause:");
      Serial.println(Time);
      if(Time.toInt()>0){
          timePause=Time.toInt();
          EEPROM.put(10,timePause);   
          EEPROM.commit();
          Serial.println(timePause);
          String msg="Новий час спрацьовування:"+ Time + "хв";
          bot.sendMessage(chat_id, msg, "");
        }
       else{
        bot.sendMessage(chat_id, "Новий час не записано", "");
        }
        Time="";
         }
    
    if (text.startsWith("/state_ms")) {
      if (alarm){
      bot.sendMessage(chat_id, "Сигналізація комори включена", "");
      }
      else{
          bot.sendMessage(chat_id, "Сигналізація комори виключена", "");
      }
      
     
    }
  }
}

void setup() {
   pinMode(pinWake,OUTPUT);
  digitalWrite(pinWake,HIGH);
  pinMode(lightSensor,INPUT);
  pinMode(Switch,INPUT);
  Serial.begin(115200);
  if (digitalRead(Switch)==HIGH){   //коли вкл режим нічний
      Serial.println("Mode switch night!");
      if(digitalRead(lightSensor)==LOW){  //коли надворі день
          sleep1();
        }
    }
    
  #ifdef ESP8266
    configTime(0, 0, "pool.ntp.org");      // get UTC time via NTP
    client.setTrustAnchors(&cert); // Add root certificate for api.telegram.org
  #endif

  pinMode(LED_BUILTIN, OUTPUT); 
  digitalWrite(LED_BUILTIN, LOW);
  delay(10);
  EEPROM.begin(512);
  //EEPROM.put(adress, 1);   //запишем спочатку true в коміру памяті потім закомкентуєм
  //EEPROM.commit();
  timePause= EEPROM.read(10);
  alarm= EEPROM.read(adress);
  Serial.print(" ");
  Serial.println(alarm);
  delay(100);
  // Connect to Wi-Fi
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  #ifdef ESP32
    client.setCACert(TELEGRAM_CERTIFICATE_ROOT); // Add root certificate for api.telegram.org
  #endif
  int count=0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi..");
    count++;
    if (count>10){
      sleep1();
      }
    
  }
  timeClient.begin();
  // Print ESP32 Local IP Address
  Serial.println(WiFi.localIP());
  client.setInsecure();
  
  
  if(digitalRead(lightSensor)==HIGH){  // коли ніч пауза між спрацьовуванням 2 хв
     timePause=2; 
     Serial.println("night"); 
    }
  timeClient.update();
  
  
  Hour= EEPROM.read(4);
  Minutes=EEPROM.read(6);
  delay(1000);
 
   int NewHour=timeClient.getHours();
   int NewMinutes=timeClient.getMinutes();
   
 // Serial.println(timeClient.getFormattedTime());
 if(timeClient.getHours()==2 and timeClient.getMinutes()==0){
    delay(1000);
    timeClient.update();
    NewHour=timeClient.getHours();
    NewMinutes=timeClient.getMinutes();
    Serial.println("tut");
    }
  Serial.print(Hour);
  Serial.print(":");
  Serial.println(Minutes);
  Serial.print("New");
  Serial.print(NewHour);
  Serial.print(":");
  Serial.println(NewMinutes);
  Serial.print("difference in min:");
  int differ=(NewHour*60+NewMinutes)-(Hour*60+Minutes);
  Serial.println(differ);
  if(differ>=timePause or differ<0){
    Serial.println("Write new hour and minutes");
   EEPROM.put(4, NewHour);
   EEPROM.put(6, NewMinutes);     
   EEPROM.commit();
   delay(500);
  }
  else{
    sleep1();
  }
 

}

void loop() {
  
  if(alarm){
      bot.sendMessage(CHAT_ID, "Виявлено рух в коморі!", "");
      Serial.println("тут2");
      alarm=false;
  }
  
  if (millis() > lastTimeBotRan + botRequestDelay)  {
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);

    while(numNewMessages) {
      Serial.println("got response");
      handleNewMessages(numNewMessages);
      numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    }
    lastTimeBotRan = millis();
  }
 
    Volt();
    delay(500);
    sleep1();
  
}
void sleep1(){
   Serial.println("Sleep###");
   digitalWrite(pinWake,LOW);
   delay(500);
   ESP.deepSleep(0);
}
void Volt(){
  delay(100);
  val = analogRead(analogvolt);
  Serial.println(val);
  if (val>0){
     vout = (val * 1.0) / 1024.0;
     vin = (vout / (R2 / (R1 + R2)))*0.980;
    }
   else{
    vin=0.0;
    } 
  
  Serial.println(vin);
  if (vin<3.30){
     bot.sendMessage(CHAT_ID, "Акумулятор розряджений!", "");
    }
  if (flagVolt){
    String text="Напруга U=" + String(vin, 2);;
    bot.sendMessage(CHAT_ID, text, "");
    Serial.println("SENDVolt...");
    flagVolt=false;
  }
}
