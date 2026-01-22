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
const char* mqtt_topic_value = "zuerich/plant/1";
const char* mqtt_topic_status = "zuerich/plant/1/status";
const char* mqtt_topic_min = "zuerich/plant/Min_Rohwert";
const char* mqtt_topic_max = "zuerich/plant/Max_Rohwert";

// Zeit-Management
unsigned long lastMeasureTime = 0;  // Speichert den letzten Ausführungszeitpunkt
const long interval = 500;          // Intervall in Millisekunden (500ms)

WiFiClient espClient;
PubSubClient client(espClient);

// Display
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);


// State
int lastPublishedMoisture = -1;
String lastStatus = "";

// --------------------------------
void setup() {
  Serial.begin(115200);

  pinMode(GREEN, OUTPUT);
  pinMode(YELLOW, OUTPUT);
  pinMode(RED, OUTPUT);
  pinMode(SENSOR, INPUT);

  Wire.begin(21, 22);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("Display init failed");
    while (true)
      ;
  }
  display.clearDisplay();
  display.display();

  setup_wifi();

  client.setServer(mqtt_server, 1883);
  client.setCallback(mqttCallback);

  reconnectMQTT();
}
// WIFI
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

// MQTT
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
// Callback
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  // Payload in String umwandeln
  char msg[20];
  if (length >= sizeof(msg)) return;

  memcpy(msg, payload, length);
  msg[length] = '\0';

  int value = atoi(msg);

  bool limitChanged = false;

  if (strcmp(topic, mqtt_topic_min) == 0) {
    min_humid = value;
    limitChanged = true;
    Serial.print("New MIN set: ");
    Serial.println(min_humid);
  } else if (strcmp(topic, mqtt_topic_max) == 0) {
    max_humid = value;
    limitChanged = true;
    Serial.print("New MAX set: ");
    Serial.println(max_humid);
  }

  // Wenn Grenzwert geändert wurde → Status sofort neu berechnen & publishen
  if (limitChanged) {
    int moisture = readMoisture();
    String status = getStatus(moisture);

    client.publish(mqtt_topic_status, status.c_str());
    lastStatus = status;

    Serial.print("Status recalculated: ");
    Serial.println(status);
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


// return status
String getStatus(int moisture) {
  if (moisture < min_humid) return "wet";
  if (moisture > max_humid) return "dry";
  return "ok";
}


// -------------------------------
void loop() {
  // 1. MQTT Verbindung sicherstellen
  if (!client.connected()) {
    reconnectMQTT();
  }

  // 2. MQTT "atmen" lassen
  // Diese Funktion muss so oft wie möglich aufgerufen werden,
  // damit eingehende Nachrichten (MIN/MAX) sofort verarbeitet werden.
  client.loop();

  // 3. Zeitmessung für Sensor & Display
  unsigned long currentMillis = millis();

  // Prüfen, ob 500ms vergangen sind
  if (currentMillis - lastMeasureTime >= interval) {
    // Zeitstempel aktualisieren
    lastMeasureTime = currentMillis;

    // --- Ab hier läuft dein alter Code (alle 500ms) ---

    int moisture = readMoisture();
    String status = getStatus(moisture);

    // Werte publishen, wenn Änderung signifikant (Schwellwert 50)
    if (lastPublishedMoisture < 0 || abs(moisture - lastPublishedMoisture) >= 50) {
      String payload = String(moisture);
      client.publish(mqtt_topic_value, payload.c_str());
      lastPublishedMoisture = moisture;
      Serial.print("Published moisture: ");
      Serial.println(moisture);
    }

    // Status publishen, wenn er sich ändert
    if (status != lastStatus) {
      client.publish(mqtt_topic_status, status.c_str());
      lastStatus = status;
      Serial.print("Published status: ");
      Serial.println(status);
    }

    // LEDs steuern
    digitalWrite(GREEN, LOW);
    digitalWrite(YELLOW, LOW);
    digitalWrite(RED, LOW);

    if (status == "dry") {
      digitalWrite(GREEN, HIGH);
    } else if (status == "wet") {
      digitalWrite(RED, HIGH);
    } else {
      digitalWrite(YELLOW, HIGH);
    }

    // Display aktualisieren
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);

    display.setTextSize(2);
    display.setCursor(0, 10);
    display.print(moisture);

    display.setTextSize(2);
    display.setCursor(0, 40);
    display.print(status);

    display.display();

    // HIER KEIN delay(500) MEHR!
  }
}