#include <WiFi.h>
#include <PubSubClient.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Pins
#define GREEN 23
#define YELLOW 25
#define RED 26
#define SENSOR 32

// Schwellen
int max_humid = 2800;  // zu nass
int min_humid = 1200;  // zu trocken

// WLAN
const char* ssid = "GuestWLANPortal";

// MQTT
const char* mqtt_server = "10.10.2.127";
const char* mqtt_topic = "zuerich/plant/1";

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
  while (!client.connected()) {
    client.connect("ESP32_HUMID");
  }
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
void loop() {
  Serial.println("Loop alive");
  if (!client.connected()) {
    reconnectMQTT();
  }
  client.loop();

  int moisture = readMoisture();

  // Rohdaten
  String payload =  String(moisture);

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
  display.setCursor(0, 0);
  display.print("Moisture:");

  display.setTextSize(2);
  display.setCursor(0, 16);
  display.print(moisture);

  display.display();


  delay(500);
}
