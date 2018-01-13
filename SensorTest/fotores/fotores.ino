// PRUEBA PARA LAS FOTO-RESISTENACIAS

void setup() {
  // Primera lectura analógica
  analogRead(A6);
  analogRead(A7);
  // nota: en Arduino Nano los pines A6 y A7 NO se pueden usar como pines digitales

  // Inicar puerto serie
  Serial.begin(9600);
  Serial.println("FOTO-RESISTENCIAS");
}

void loop() {
  // Leer valores analógicos
  int a = analogRead(A6);
  int b = analogRead(A7);
  
  Serial.print(a);
  Serial.print('\t');
  Serial.println(b);

  delay(100);
}
