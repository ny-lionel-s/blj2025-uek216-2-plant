#include <WiFi.h>
#include <PubSubClient.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Pins
#define GREEN 23
#define YELLOW 25
#define RED 27
#define SENSOR 32

// min max
int min_humid = 1700;
int max_humid = 2800;

// WLAN
const char* ssid = "GuestWLANPortal";

// MQTT
const char* mqtt_server = "10.10.2.127";
const char* mqtt_topic_value  = "zuerich/plant/1/Humidity";
const char* mqtt_topic_status = "zuerich/plant/1/Status";
const char* mqtt_topic_min    = "zuerich/plant/1/Min_Rohwert";
const char* mqtt_topic_max    = "zuerich/plant/1/Max_Rohwert";

// Caesar key
const int CAESAR_SHIFT = 3;

// Zeit
unsigned long lastMeasureTime = 0;
const long interval = 500;

WiFiClient espClient;
PubSubClient client(espClient);

// Display
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// State
int lastPublishedMoisture = -1;
String lastStatus = "";

// ---------------- CAESAR ----------------
String caesarEncrypt(String input, int shift) {
  String result = "";
  for (int i = 0; i < input.length(); i++) {
    char c = input[i];
    if (c >= 32 && c <= 126) {
      char e = c + shift;
      if (e > 126) e = 31 + (e - 126);
      result += e;
    } else {
      result += c;
    }
  }
  return result;
}

// ---------------- WIFI ----------------
void setup_wifi() {
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");
}

// ---------------- MQTT ----------------
void reconnectMQTT() {
  while (!client.connected()) {
    Serial.print("MQTT connecting...");
    if (client.connect("ESP32_HUMID")) {
      Serial.println("connected");
      client.subscribe(mqtt_topic_min);
      client.subscribe(mqtt_topic_max);
    } else {
      Serial.println("failed, retrying...");
      delay(2000);
    }
  }
}

// ---------------- CALLBACK ----------------
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  char msg[20];
  if (length >= sizeof(msg)) return;

  memcpy(msg, payload, length);
  msg[length] = '\0';

  int value = atoi(msg);
  bool limitChanged = false;

  if (strcmp(topic, mqtt_topic_min) == 0) {
    min_humid = (value == 0) ? 1700 : value;
    limitChanged = true;
    Serial.print("New MIN: ");
    Serial.println(min_humid);
  }

  if (strcmp(topic, mqtt_topic_max) == 0) {
    max_humid = (value == 0) ? 2800 : value;
    limitChanged = true;
    Serial.print("New MAX: ");
    Serial.println(max_humid);
  }

  if (limitChanged) {
    int moisture = readMoisture();
    String status = getStatus(moisture);
    String encStatus = caesarEncrypt(status, CAESAR_SHIFT);
    client.publish(mqtt_topic_status, encStatus.c_str());
    lastStatus = status;
  }
}

// ---------------- SENSOR ----------------
int readMoisture() {
  int sum = 0;
  for (int i = 0; i < 10; i++) {
    sum += analogRead(SENSOR);
    delay(5);
  }
  return sum / 10;
}

// ---------------- STATUS ----------------
String getStatus(int moisture) {
  if (moisture < min_humid) return "wet";
  if (moisture > max_humid) return "dry";
  return "ok";
}

// ---------------- SETUP ----------------
void setup() {
  Serial.begin(115200);

  pinMode(GREEN, OUTPUT);
  pinMode(YELLOW, OUTPUT);
  pinMode(RED, OUTPUT);
  pinMode(SENSOR, INPUT);

  Wire.begin(21, 22);

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.display();

  setup_wifi();

  client.setServer(mqtt_server, 1883);
  client.setCallback(mqttCallback);
  reconnectMQTT();
}

// ---------------- LOOP ----------------
void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    setup_wifi();
  }

  if (!client.connected()) {
    reconnectMQTT();
  }
  client.loop();

  unsigned long now = millis();
  if (now - lastMeasureTime >= interval) {
    lastMeasureTime = now;

    int moisture = readMoisture();
    String status = getStatus(moisture);

    // publish moisture encrypted
    if (lastPublishedMoisture < 0 || abs(moisture - lastPublishedMoisture) >= 30) {
      String encMoisture = caesarEncrypt(String(moisture), CAESAR_SHIFT);
      client.publish(mqtt_topic_value, encMoisture.c_str());
      lastPublishedMoisture = moisture;
    }

    // publish status encrypted
    if (status != lastStatus) {
      String encStatus = caesarEncrypt(status, CAESAR_SHIFT);
      client.publish(mqtt_topic_status, encStatus.c_str());
      lastStatus = status;
    }

    digitalWrite(GREEN, LOW);
    digitalWrite(YELLOW, LOW);
    digitalWrite(RED, LOW);

    if (status == "dry") digitalWrite(GREEN, HIGH);
    else if (status == "wet") digitalWrite(RED, HIGH);
    else digitalWrite(YELLOW, HIGH);

    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(2);
    display.setCursor(0, 10);
    display.print(moisture);
    display.setCursor(0, 40);
    display.print(status);
    display.display();
  }
}
