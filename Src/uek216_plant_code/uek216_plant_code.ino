#include <WiFi.h>
#include <PubSubClient.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Pins
#define GREEN 23
#define YELLOW 25
#define RED 27
#define SENSOR 32

// Schwellen
int max_humid = 2800;  // zu nass
int min_humid = 1200;  // zu trocken

// WLAN
const char* ssid = "GuestWLANPortal";

// MQTT
const char* mqtt_server = "10.10.2.127";
const char* mqtt_topic = "zuerich/plant/1";
const char* mqtt_topic_min = "zuerich/plant/MIN_Rohwert";
const char* mqtt_topic_max = "zuerich/plant/MAX_Rohwert";


// Display
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

//  MQTT
WiFiClient espClient;
PubSubClient client(espClient);

//  Setup
void setup() {
  pinMode(GREEN, OUTPUT);
  pinMode(YELLOW, OUTPUT);
  pinMode(RED, OUTPUT);
  pinMode(SENSOR, INPUT);

  Wire.begin(21, 22);
  Serial.begin(115200);

  // Display
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    while (1)
      ;
  }
  display.clearDisplay();
  display.display();

  // WLAN
  setup_wifi();

  // MQTT
  client.setServer(mqtt_server, 1883);
  client.setCallback(mqttCallback);

  while (!client.connected()) {
    client.connect("ESP32_HUMID");
  }
  // Subscriptions
  client.subscribe(mqtt_topic_min);
  client.subscribe(mqtt_topic_max);
}

void setup_wifi() {
  Serial.print("Connecting to ");
  Serial.print(ssid);
  WiFi.begin(ssid);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("done!");
}

void reconnectMQTT() {
  while (!client.connected()) {
    Serial.print("MQTT reconnect...");
    if (client.connect("ESP32_HUMID")) {
      Serial.println("connected");

      // erneut subscriben
      client.subscribe(mqtt_topic_min);
      client.subscribe(mqtt_topic_max);

    } else {
      Serial.println("failed");
      delay(2000);
    }
  }
}


int readMoisture() {
  int sum = 0;
  for (int i = 0; i < 10; i++) {
    sum += analogRead(SENSOR);
    delay(5);
  }
  return sum / 10;
}
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  char msg[20];
  if (length >= sizeof(msg)) return;

  memcpy(msg, payload, length);
  msg[length] = '\0';

  int value = atoi(msg);
  
  if (strcmp(topic, mqtt_topic_min) == 0) {
    min_humid = value;
    Serial.print("New MIN set: ");
    Serial.println(min_humid);
  }

  if (strcmp(topic, mqtt_topic_max) == 0) {
    max_humid = value;
    Serial.print("New MAX set: ");
    Serial.println(max_humid);
  }

  Serial.print("Topic: ");
  Serial.println(topic);
  Serial.print("Payload: ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}

void loop() {
  if (!client.connected()) {
    reconnectMQTT();
  }
  client.loop();

  int moisture = readMoisture();

  // Rohdaten
  String payload = String(moisture);

  client.publish(mqtt_topic, payload.c_str());


  Serial.print("Current humidity: ");
  Serial.println(moisture);

  // LEDs reset
  digitalWrite(GREEN, LOW);
  digitalWrite(YELLOW, LOW);
  digitalWrite(RED, LOW);

  // Ampel
  if (moisture < min_humid) {
    // zu trocken
    digitalWrite(GREEN, HIGH);
  } else if (moisture > max_humid) {
    // zu nass
    digitalWrite(RED, HIGH);
  } else {
    // normal
    digitalWrite(YELLOW, HIGH);
  }

  // Display
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(2);
  display.setCursor(0, 10);
  display.print(moisture);

  display.display();


  delay(500);
}
