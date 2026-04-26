const int A_PIN = 27;
const int B_PIN = 18;

void setup() {
  Serial.begin(115200);
  pinMode(A_PIN, INPUT);
  pinMode(B_PIN, INPUT_PULLUP);
}

void loop() {
  int A = digitalRead(A_PIN);
  int B = digitalRead(B_PIN);

  Serial.print("A = ");
  Serial.print(A);
  Serial.print(" | B = ");
  Serial.println(B);

  delay(100);
}
