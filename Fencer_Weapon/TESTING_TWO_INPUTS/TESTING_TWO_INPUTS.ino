const int TIP1 = 27;
const int TIP2 = 14;

void setup() {
  Serial.begin(115200);
  pinMode(TIP1, INPUT_PULLUP);
  pinMode(TIP2, INPUT_PULLUP);
}

void loop() {
  Serial.print("TIP1=");
  Serial.print(digitalRead(TIP1));
  Serial.print("  TIP2=");
  Serial.println(digitalRead(TIP2));
  delay(100);
}
