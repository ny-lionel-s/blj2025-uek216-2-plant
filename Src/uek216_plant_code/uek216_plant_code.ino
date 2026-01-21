#include <WiFi.h>
#include <PubSubClient.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Pins 
#define GREEN 23
#define YELLOW 19
#define RED 18
#define SENSOR 26

Wire.begin();

// Schwellen
int max_humid = 10000;   // zu nass
int min_humid = 50;      // zu trocken

// WLAN 
const char* ssid = "GuestWLANPortal";


// MQTT
const char* mqtt_server = "10.10.2.127";
const char* mqtt_topic = "zuerich/plant/myplant";

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

  Serial.begin(115200);

  // Display
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();

  // WLAN
  WiFi.begin(ssid);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }

  // MQTT
  client.setServer(mqtt_server, 1883);
  while (!client.connected()) {
    client.connect("ESP32_HUMID");
  }
}

// Loop
void loop() {
  client.loop();

  int moisture = analogRead(SENSOR);

  // Rohdaten
  client.publish(mqtt_topic, String(moisture).c_str());

  Serial.print("Feuchtigkeit: ");
  Serial.println(moisture);

  // LEDs reset
  digitalWrite(GREEN, LOW);
  digitalWrite(YELLOW, LOW);
  digitalWrite(RED, LOW);

  // Ampel
  if (moisture < min_humid) {
    // zu trocken
    digitalWrite(GREEN, HIGH);
  }
  else if (moisture > max_humid) {
    // zu nass
    digitalWrite(RED, HIGH);
  }
  else {
    // normal
    digitalWrite(YELLOW, HIGH);
  }

  // Display
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(0, 20);
  display.print("Current Humidity:");
  display.print(moisture);
  display.display();

  delay(500);
}
