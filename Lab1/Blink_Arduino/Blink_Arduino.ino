const int LED = 25;
float frecuencia = 1.0;
unsigned long ultimoCambio = 0;

void setup() {
  pinMode(LED, OUTPUT);
  Serial.begin(115200);
  while (!Serial) {
    delay(10); // espera hasta que el host abra el puerto USB CDC
  }
  Serial.println("Ingrese frecuencia (Hz):");
}

void loop() {
  if (Serial.available() > 0) {
    float f = Serial.parseFloat();
    if (f > 0) {
      frecuencia = f;
      Serial.print("Frecuencia actual: ");
      Serial.print(frecuencia);
      Serial.println(" Hz");
    }
  }

  if (millis() - ultimoCambio >= (500.0 / frecuencia)) {
    ultimoCambio = millis();
    digitalWrite(LED, !digitalRead(LED));
  }
}