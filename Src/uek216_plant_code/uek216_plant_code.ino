#define GREEN 23
#define YELLOW 22
#define RED 21
#define SENSOR 26

int max_humid = 10000;   // to wet
int min_humid = 50;   // to dry

void setup() {
  pinMode(GREEN, OUTPUT);
  pinMode(YELLOW, OUTPUT);
  pinMode(RED, OUTPUT);

  pinMode(SENSOR, INPUT);

  Serial.begin(115200);
}

void loop() {
  int moisture = analogRead(SENSOR);

  Serial.print("Feuchtigkeit: ");
  Serial.println(moisture);

  // LOW all LED
  digitalWrite(GREEN, LOW);
  digitalWrite(YELLOW, LOW);
  digitalWrite(RED, LOW);

  if (moisture < min_humid) {
    // to dry
    digitalWrite(YELLOW, HIGH);
  }
  else if (moisture > max_humid) {
    // to wet
    digitalWrite(RED, HIGH);
  }
  else {
    // good
    digitalWrite(GREEN, HIGH);
  }

  delay(500);
}
