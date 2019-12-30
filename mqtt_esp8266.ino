#include <time.h>
#include <sys/time.h>
#include <CronAlarms.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

#define activity_indicator D0
#define saida_comando_valvula D1
#define activity_indicator_led D2

const char* ssid = "";
const char* password = "";
const char* mqtt_server = "broker.mqtt-dashboard.com";
const char* topic_house_garden = "house/garden";

const long utcOffsetInSeconds = 3600;

WiFiClient espClient;
PubSubClient client(espClient);
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "a.st1.ntp.br", utcOffsetInSeconds);

long lastMsg = 0;
char msg[50];
int value = 0;
bool isStart = false;
bool allowCommandsMQTT = true;

void setup() {
  Serial.begin(115200);

  pinMode(activity_indicator, OUTPUT);
  pinMode(saida_comando_valvula, OUTPUT);
  pinMode(activity_indicator_led, OUTPUT);

  setupWifi();

  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  digitalWrite(activity_indicator, HIGH);

  timeClient.begin();

  configureDateTime();

  //Cron.create("0 30 8 * * *", startx, false);
  //Cron.create("*/120 * * * * *", startx, false);
  //Cron.create("0 2 20 * * *", startx, false);
}

void startx() {
  startStop(true, "Cron");
  Cron.delay();
  delay(5000);
  startStop(false, "Cron");
}

void setupWifi() {
  if (WiFi.status() == WL_CONNECTED)
    return;

  delay(10);

  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  printData("--------------");
  printData("WiFi connected");
  printData("IP address: ");
  printData(WiFi.localIP().toString());
}

void reconnectMQTT() {
  if (client.connected())
    return;

  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");

    String clientId = "ESP8266Client-";

    clientId += String(random(0xffff), HEX);

    if (client.connect(clientId.c_str())) {
      Serial.println("connected");

      client.subscribe(topic_house_garden);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void loop() {
  Cron.delay();

  reconnectMQTT();

  client.loop();
}

void callback(char* topic, byte * payload, unsigned int length) {
  digitalWrite(activity_indicator, LOW);
  delay(100);
  digitalWrite(activity_indicator, HIGH);

  printData("Message arrived [" + String(topic) + "] - " + String((char)payload[0]));

  String topicx(topic);
  String str1(topic_house_garden);

  if (topicx.equals(str1) && ((char)payload[0] == '0' || (char)payload[0] == '1')) {
    startStop((char)payload[0] == '1', "MQTT");
  }
}

void startStop(bool startProcess, String source) {
  if (startProcess == true) {
    digitalWrite(saida_comando_valvula, HIGH);
    digitalWrite(activity_indicator_led, HIGH);

    printData("Irrigate(" + source + "): start");
  }
  else {
    digitalWrite(saida_comando_valvula, LOW);
    digitalWrite(activity_indicator_led, LOW);

    printData("Irrigate(" + source + "): stop");
  }
}

void configureDateTime() {
  timeClient.update();

  struct tm tm_newtime;
  tm_newtime.tm_year = 2011 - 1900;
  tm_newtime.tm_mon = 1 - 1;
  tm_newtime.tm_mday = timeClient.getDay();
  tm_newtime.tm_hour = timeClient.getHours();
  tm_newtime.tm_min = timeClient.getMinutes();
  tm_newtime.tm_sec = timeClient.getSeconds();
  tm_newtime.tm_isdst = 0;
  timeval tv = { mktime(&tm_newtime), 0 };
  timezone tz = { 0, 0};
  settimeofday(&tv, &tz);

  time_t tnow = time(nullptr);
  Serial.println(asctime(gmtime(&tnow)));
}

void printData(String data) {
  time_t tnow = time(nullptr);
  Serial.println("");
  Serial.print(asctime(gmtime(&tnow)));
  Serial.print(" -> ");
  Serial.print(data);
  Serial.println("");
}
