// PROYECTO: ROBOT MODULAR
// CAJA LUZ v0.3 (5 nov 2017)
//
// Referencias
// MPU-6050 Datasheet
// http://www.instructables.com/id/Magic-Wand-TV-Remote/
// https://playground.arduino.cc/Main/SoftwareI2CLibrary
// https://www.arduino.cc/en/Hacking/Atmega168Hardware
// http://playground.arduino.cc/Main/MPU-6050
// https://learn.adafruit.com/adafruit-neopixel-uberguide/arduino-library-use

//======================================================================================== Diresciones i2c
#define CAJA_LUZ    1
#define CAJA_MOTOR  2
#define CAJA_SONIDO 3


// ======================================================================================= Definiciones

// Estados y colores
#define ON  1 //luces encendidas
#define OFF 0 //luces apagadas
#define SI  255
#define NO  0
#define SEN 3 //enciende o apaga con foto-resistencias
#define DO          1
#define RE          2
#define MI          3
#define FA          4

#define TEREMIT     0
#define NOTA_DO   523
#define NOTA_RE   587
#define NOTA_MI   659
#define NOTA_FA   698
#define NOTA_SOL   784
#define NOTA_LA   880
#define NOTA_SI   988

// Neopixel
#define PINPIXELS 6 //pin de datos
#define NUMPIXELS 4 //número de pixels

// Colores
#define ROJO     0xFF0000
#define AMARILLO 0xFFFF00
#define VERDE    0x00FF00
#define CIAN     0x00FFFF
#define AZUL     0x0000FF
#define MORADO   0xFF00FF
#define BLANCO   0XFFFFFF
#define RANDOM   0x1000000
#define NUMCOLORES 8

// Pines para SoftWire
// Estas definiciones se deben hacer antes de incluir la biblioteca
// En port D, el número de pin en el puerto corresponde con el número de pin en Arduino
#define SDA_PORT PORTD
#define SDA_PIN 3 //Digital 3
#define SCL_PORT PORTD
#define SCL_PIN 5 //Digital 5
#define bocina  4
const byte sensorPin = A7;         // medidor de distancia en A0
const int knockSensor = 2;         // Piezo sensor on pin A2
const int MI_DIR = CAJA_SONIDO; //esta es la direccion de tu dispositivo, debe ser distinta en cada arduino
const int NOTA = NOTA_SOL;

#include "pitches.h"
#include <Adafruit_NeoPixel.h>
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, PINPIXELS, NEO_GRB + NEO_KHZ800);

#include <SharpDistSensor.h>
#include <SoftWire.h> //incluir biblioteca de I2C por software (usa las definiciones anteriores)
SoftWire Swire = SoftWire(); //crear objeto de I2C por software
#include <Wire.h> //incluir biblioteca de I2C por hardware



#include "Gestos.h"

// ================================================================================== Variables Globales
// Estado y color
byte estado = OFF;  //estado actual
uint32_t nota = TEREMIT; //color de los pixels
//bool estadoIni = true; //indica el primer ciclo de un estado

// I2C
byte error, address;  //
int nDevices; //contador de dispositivos I2C
char letra;
String sDir = "000"; // Texto para la direccion del esclavo
int iDir;            // Entero para la direccion del esclavo
String sDat = "000"; // Texto para el dato
int iDat;            // Entero para el dato
int libre;
byte datos[10];
int auxCont;

// Window size of the median filter (odd number, 1 = no filtering)
const byte mediumFilterWindowSize = 5; //5 (debe ser impar)
SharpDistSensor sensor(sensorPin, mediumFilterWindowSize);

int frec;
unsigned int distance;
unsigned long tiempoAct;
unsigned long tiempoPas;
int tiempoEspera = 31;
byte presionado = NO;
byte  cajaLuzConec = NO;
byte  cajaMotorConec = NO;
// Tuning constants.  Could be made vars and hoooked to potentiometers for soft configuration, etc.
const int threshold = 500;           // Minimum signal from the piezo to register as a knock
const int rejectValue = 25;        // If an individual knock is off by this percentage of a knock we don't unlock..
const int averageRejectValue = 15; // If the average timing of the knocks is off by this percent we don't unlock.
const int knockFadeTime = 150;     // milliseconds we allow a knock to fade before we listen for another one. (Debounce timer.)
const int lockTurnTime = 650;      // milliseconds that we run the motor to get it to go a half turn.
const int maximumKnocks = 20;       // Maximum number of knocks to listen for.
const int knockComplete = 1200;     // Longest time to wait for a knock before we assume that it's finished.
// Variables.
int secretCode[maximumKnocks] = {50, 25, 25, 50, 100, 50, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};  // Initial setup: "Shave and a Hair Cut, two bits."
int knockReadings[maximumKnocks];   // When someone knocks this array fills with delays between knocks.
int knockSensorValue = 0;
// Array para elegir notas
const int notas[] = {TEREMIT, NOTA_DO, NOTA_RE, NOTA_MI, NOTA_FA, NOTA_SOL, NOTA_LA, NOTA_SI};
byte notasIndex = 0; //para recorrer el array
int melody[] = {
  NOTE_C4, NOTE_G3, NOTE_G3, NOTE_A3, NOTE_G3, 0, NOTE_B3, NOTE_C4
};

int noteDurations[] = {
  4, 8, 8, 4, 4, 4, 4, 4
};

// ================================================================================== Configuración
void setup() {
  tone(bocina, NOTA);
  delay(600);
  noTone(bocina);
  pixels.begin();
  ponerColor(AMARILLO);
  // Configuracion I2C hardware A4(SDA) A5(SCL)
  // Todos los módulos son esclavos, pero pueden solicitar datos a otros esclavos
  Wire.begin(MI_DIR); //iniciar I2C como esclavo
  Wire.onRequest(requestEvent); // register event

  // Configuracion I2C software D3(SDA) D5(SCL)
  // NOTA: En la caja de luz se podrían usar D4 y D5
  Swire.begin(); //solo se puede iniciar como maestro
  Swire.beginTransmission(MPU); //iniciar MPU-6050
  Swire.write(0x6B); //PWR_MGMT_1 register
  Swire.write(0); //set to zero (wakes up the MPU-6050)
  Swire.endTransmission(true);

  // Iniciar comunicación serial
  // (esto tal vez no es necesario en la versión final)
  Serial.begin(9600); //inicar puerto serie (para depurar programa)
  Serial.print("Caja de Luz 0x"); //imprimir saludo
  Serial.println(MI_DIR, HEX); //y dirección I2C


} //fin setup()

// ================================================================================== Principal
void loop() {
  gestos(); //reconocer gestos
  if (cambio() == SI)
  {
    buscarI2C();
  }

  if (estado == OFF) //si apagado
  {
    ponerColor(AMARILLO);
    noTone(bocina); //apagar leds
    if (gestoReco == GOLPE_V) { //si se hace un golpe vertical
      tone(bocina,NOTA,100);
      delay(200);
      tone(bocina,NOTA,100);
      delay(500);
      estado = ON; //cambiar a estado encendido
      Serial.println("estado = ON");
    }
  }
  else if (estado == ON) //si encendido
  {
    ponerColor(CIAN);
    if (gestoReco == GOLPE_V) { //si se hace un golpe vertical
      tone(bocina,NOTA,100);
      delay(200);
      tone(bocina,NOTA,100);
      delay(500);
      estado = OFF; //apagar
    }

    if (gestoReco == GIRO_D or gestoReco == GIRO_I) { //si se hace un golpe horizontal
      tone(bocina,NOTA,100);
      delay(200);
      tone(bocina,NOTA,100);
      delay(500);
      noTone(bocina);
      estado = SEN; //cambiar a estado sensor
      Serial.println("SEN");
    }

    if (((nDevices == 0) || (cajaLuzConec == SI)) && (cajaMotorConec == NO)) // Si no encontro ningun otro dispositivo
    {
      switch (nota)
      {
        case TEREMIT:
          ponerColor(RANDOM);
          teremitSound();
          break;

        case DO:
          piano(NOTA);
          break;
      }
    }
    else
    {
      noTone(bocina); //apagar leds
    }
  } //fin if (estado == ON)
  else  //estado == SEN
  {
    //    if (gestoReco == GOLPE_H) { //si se hace un golpe vertical
    //      estado = OFF; //cambiar a estado apagado
    //    }
    if (gestoReco == GIRO_D or gestoReco == GIRO_I) { //si se hace un golpe horizontal
      estado = ON; //cambiar a estado sensor
      Serial.println("ON");
    }
    
    noTone(bocina);
    ponerColor(BLANCO);
    golpeador();
  }
  delay(random(5, 15));
} //fin void loop()


//------------------------------------------------- buscar I2C
void buscarI2C()
{
  //Serial.println("Busca i2c");
  nDevices = 0;
  cajaLuzConec = NO;
  cajaMotorConec = NO;

  for (address = 1; address < 12; address++ )
  {
    // The i2c_scanner uses the return value of
    // the Write.endTransmisstion to see if
    // a device did acknowledge to the address.
    Wire.beginTransmission(address);
    error = Wire.endTransmission();

    if (error == 0)
    {
      Serial.println("Hay i2c");
      nDevices++;
      switch (address)
      {
        case CAJA_MOTOR:  //Caja de luz conectada
          //leerEsclavo(CAJA_MOTOR, 1);
          cajaMotorConec = SI;
          Serial.println("CAJA_MOTOR");
          break;

        case CAJA_LUZ:  //Sonido
          //leerEsclavo(CAJA_LUZ, 4);
          cajaLuzConec = SI;
          Serial.println("CAJA_LUZ");
          break;
      }

    }
  }
}

//---------------------------------------------- leerEsclavos
void leerEsclavo(int dir, int bytes)
{
  int i = 0;
  Wire.requestFrom(dir, bytes);    // request 5 bytes from slave device #4
  while (Wire.available())
  { // slave may send less than requested
    datos[i] = Wire.read(); // receive a byte as character
    // Serial.print(char(datos[i]));         // print the character
    i++;
  }
  //Serial.println();
}

//---------------------------------------------- requestEvent

void requestEvent()
{
  distance = sensor.getDist(); //volver a leer la distancia
  if (distance > 500) //si rebasa el rango que interesa
    distance = 500; //limitar valor
  byte d = map(distance, 60, 1500, 0, 255); //mapear a un byte
  Wire.write(presionado);
  Wire.write(d); //enviar byte
  Serial.println("SEND");
}

//================================================teremitSound
void teremitSound()
{
  distance = sensor.getDist();
  Serial.println(distance);
  // Print distance to Seria
  if ((distance > 80) && (distance < 500))
  {
    frec = map(distance, 80, 500, 31, 4978);
    presionado = map(distance, 80, 500, 0, 255);
    tone(bocina, frec);
    //delay(50);
  }
  else
  {
    noTone(bocina);
    presionado = NO;
  }

}

//=========================================== cambio
byte cambio()
{
  tiempoAct = millis(); // actualizo el segundo tiempo
  //----------------------------------- reinicia espera
  if ((tiempoAct - tiempoPas) > tiempoEspera) // Si han pasado menos de 2 segundos del primer golpe
  {
    tiempoPas = tiempoAct;
    return  SI;
  }
  else
  {
    return  NO;
  }
}


//============================================ piano
void piano(int frec)
{
  distance = sensor.getDist();
  if ((distance > 80) && (distance < 500))
  {
    tone(bocina, frec);
    presionado = SI;
  }
  else
  {
    noTone(bocina);
    presionado = NO;
  }
}

//=========================================== golpeador
void golpeador()
{
  //Serial.println("golpes");
  knockSensorValue = analogRead(knockSensor);
  if (knockSensorValue <= threshold)
  {
    listenToSecretKnock();
  }
}

//================== listenToSecretKnock Records the timing of knocks.
void listenToSecretKnock() {
  Serial.println("knock starting");

  int i = 0;
  // First lets reset the listening array.
  for (i = 0; i < maximumKnocks; i++)
  {
    knockReadings[i] = 0;
  }

  int currentKnockNumber = 0;             // Incrementer for the array.
  int startTime = millis();               // Reference for when this knock started.
  int now = millis();


  delay(knockFadeTime);
  do {
    //listen for the next knock or wait for it to timeout.
    knockSensorValue = analogRead(knockSensor);
    if (knockSensorValue <= threshold) {                 //got another knock...
      //record the delay time.
      Serial.println("knock.");
      now = millis();
      knockReadings[currentKnockNumber] = now - startTime;
      currentKnockNumber ++;                             //increment the counter
      startTime = now;
      // and reset our timer for the next knock
      delay(knockFadeTime);                              // again, a little delay to let the knock decay.

    }

    now = millis();

    //did we timeout or run out of knocks?
  } while ((now - startTime < knockComplete) && (currentKnockNumber < maximumKnocks));

  //we've got our knock recorded, lets see if it's valid
  { // only if we're not in progrmaing mode.
    if (validateKnock() == true)
    {
      triggerDoorUnlock();
    }
    else
    {
      ponerColor(ROJO);
      Serial.println("Secret knock failed.");
      tone(bocina,NOTE_C3,500);
      delay(500);
      noTone(bocina);
    }
  }
}

void triggerDoorUnlock() 
{
  ponerColor(VERDE);
  Serial.println("Door unlocked!");
    // iterate over the notes of the melody:
  for (int thisNote = 0; thisNote < 8; thisNote++) 
  {

    // to calculate the note duration, take one second divided by the note type.
    //e.g. quarter note = 1000 / 4, eighth note = 1000/8, etc.
    int noteDuration = 1000 / noteDurations[thisNote];
    tone(bocina, melody[thisNote], noteDuration);

    // to distinguish the notes, set a minimum time between them.
    // the note's duration + 30% seems to work well:
    int pauseBetweenNotes = noteDuration * 1.30;
    delay(pauseBetweenNotes);
    // stop the tone playing:
    noTone(bocina);
  }
  int i = 0;
}

// Sees if our knock matches the secret.
// returns true if it's a good knock, false if it's not.
// todo: break it into smaller functions for readability.
boolean validateKnock() {
  int i = 0;

  // simplest check first: Did we get the right number of knocks?
  int currentKnockCount = 0;
  int secretKnockCount = 0;
  int maxKnockInterval = 0;               // We use this later to normalize the times.

  for (i = 0; i < maximumKnocks; i++) {
    if (knockReadings[i] > 0) {
      currentKnockCount++;
    }
    if (secretCode[i] > 0) {          //todo: precalculate this.
      secretKnockCount++;
    }

    if (knockReadings[i] > maxKnockInterval) {  // collect normalization data while we're looping.
      maxKnockInterval = knockReadings[i];
    }
  }

  if (currentKnockCount != secretKnockCount) {
    return false;
  }

  /*  Now we compare the relative intervals of our knocks, not the absolute time between them.
      (ie: if you do the same pattern slow or fast it should still open the door.)
      This makes it less picky, which while making it less secure can also make it
      less of a pain to use if you're tempo is a little slow or fast.
  */
  int totaltimeDifferences = 0;
  int timeDiff = 0;
  for (i = 0; i < maximumKnocks; i++) { // Normalize the times
    knockReadings[i] = map(knockReadings[i], 0, maxKnockInterval, 0, 100);
    timeDiff = abs(knockReadings[i] - secretCode[i]);
    if (timeDiff > rejectValue) { // Individual value too far out of whack
      return false;
    }
    totaltimeDifferences += timeDiff;
  }
  // It can also fail if the whole thing is too inaccurate.
  if (totaltimeDifferences / secretKnockCount > averageRejectValue) {
    return false;
  }

  return true;

}

// ===================================================== Poner Color
void ponerColor(byte r, byte g, byte b) { //forma con 3 argumentos
  for (int i = 0; i < NUMPIXELS; i++) {
    pixels.setPixelColor(i, r, g, b);
  }
  pixels.show();
}
void ponerColor(uint32_t c) { //forma sobrecargada, con 1 argumento
  if (c == RANDOM) {
    pixels.setPixelColor(random(NUMCOLORES), random(BLANCO));
  }
  else {
    for (int i = 0; i < NUMPIXELS; i++) {
      pixels.setPixelColor(i, c);
    }
  }
  pixels.show();
}

