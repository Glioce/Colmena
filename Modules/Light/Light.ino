// PROYECTO: ROBOT MODULAR
// CAJA LUZ v0.4 (6 nov 2017)
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
// Neopixel
#define PINPIXELS 4 //pin de datos
#define NUMPIXELS 6 //número de pixels

// Estados
#define OFF 0 //luces apagadas
#define ON  1 //luces encendidas
#define SEN 2 //enciende o apaga con foto-resistencias
//#define FLA 3 //flash para indicar que se inicia el estado SEN

#define SI 255
#define NO 0
#define LUZ_DER_PIN A7
#define LUZ_IZQ_PIN A6

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

#include <SoftWire.h> //incluir biblioteca de I2C por software (usa las definiciones anteriores)
SoftWire Swire = SoftWire(); //crear objeto de I2C por software
#include <Wire.h> //incluir biblioteca de I2C por hardware

// When we setup the NeoPixel library, we tell it how many pixels, and which pin to use to send signals.
// Note that for older NeoPixel strips you might need to change the third parameter--see the strandtest
// example for more information on possible values
#include <Adafruit_NeoPixel.h>
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, PINPIXELS, NEO_GRB + NEO_KHZ800);

#include "Gestos.h"

// ================================================================================== Variables Globales
// Estado y color
byte estado = OFF;  //estado actual (apagado)
uint32_t color = ROJO; //color de los pixels
//bool estadoIni = true; //indica el primer ciclo de un estado
int brillo = 0; //nivel de brillo usado en el flash que indica el inicio del estado SEN
int brilloDelta = 1; //cuanto cambiará el brillo en el siguiente ciclo

// Array para elegir colores
const uint32_t colores[] = {ROJO, AMARILLO, VERDE, CIAN, AZUL, MORADO, BLANCO, RANDOM};
byte colorIndex = 0; //indice para recorrer el array de colores

// I2C
byte error, address;  //
int nDevices; //contador de dispositivos I2C
char letra;
String sDir = "000"; // Texto para la direccion del esclavo
int iDir;            // Entero para la direccion del esclavo
String sDat = "000"; // Texto para el dato
int iDat;            // Entero para el dato
const int MI_DIR = CAJA_LUZ; //esta es la direccion de tu dispositivo, debe ser distinta en cada arduino
int libre;
byte datos[10];
int auxCont;
unsigned long tiempoAct;
unsigned long tiempoPas;
int tiempoEspera = 59;
byte cajaSonidoConec = NO;
byte cajaMotorConec = NO;
unsigned int distance;

// ================================================================================== Configuración
void setup() {
  // Configuracion I2C hardware A4(SDA) A5(SCL)
  // Todos los módulos son esclavos, pero pueden solicitar datos a otros esclavos
  Wire.begin(MI_DIR); //iniciar I2C como esclavo
  Wire.onRequest(requestEvent); // register event

  // Configuracion I2C software D3(SDA) D5(SCL)
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

  // Iniciar tira de pixels
  pixels.begin(); //todos los pixels se apagan

} //fin setup()

// ================================================================================== Principal
void loop() {
  gestos(); //reconocer gestos
  if (cambio() == SI)
  {
    buscarI2C();
  }

  if ((cajaSonidoConec == SI) && (cajaMotorConec == NO)) // Si esta conectada la caja de sonido
  {
    if (distance > 0)
    {
      estado = ON;
    }
    else
    {
      estado = OFF;
    }
    Serial.println(datos[0]);
  }// fin de cajaSonido

  if ((nDevices == 0) || (cajaSonidoConec == SI)) // Si no hay otra cosa conectada
  {
    if (estado == OFF) //------------------------------si apagado
    {
      ponerColor(0); //apagar leds

      if (gestoReco == GOLPE_V) { //si se hace un golpe vertical
        estado = ON; //cambiar a estado encendido
      }
      if (gestoReco == GOLPE_H) { //si se hace un golpe horizontal
        estado = SEN; //cambiar a estado sensor
      }
    }
    else if (estado == ON) //-----------------------------si encendido
    {
      ponerColor(color); //mostrar color actual

      if (gestoReco == GOLPE_V) { //si se hace un golpe vertical
        estado = OFF; //cambiar a estado apagado
      }
      if (gestoReco == GOLPE_H) { //si se hace un golpe horizontal
        estado = SEN; //cambiar a estado sensor
      }
      if (gestoReco == GIRO_D or gestoReco == GIRO_I) { //si se realiza un giro
        colorIndex ++; //incrementar index
        if (colorIndex >= NUMCOLORES) colorIndex = 0;
        color = colores[ colorIndex]; //asignar siguiente color
      }
    }
    else //-------------------------------------si sensor
    {
      // Promedio de los sensores
      int luz = (analogRead(LUZ_DER_PIN) + analogRead(LUZ_IZQ_PIN)) / 2;

      if (luz < 400) { //si el nivel de luz ambiente es bajo
        ponerColor(color); //encender pixels
      }
      else if (luz > 500)
      {
        for (byte i = 1; i < NUMPIXELS; i++) {
          pixels.setPixelColor(i, 0);
        }

        if (color == RANDOM) {
          pixels.setPixelColor(0, random(BLANCO)); //encender solo un pixel
          pixels.show();
        }
        else { //si el color no es random, mostrar latido
          brillo += brilloDelta; //cambiar brillo
          if (brillo >= 255) { //si se alcanza el límite superior
            brillo = 255; //no rebasar límite
            brilloDelta *= -1; //cambiar signo para disminuir el brillo
          }
          if (brillo < 0) { //si se alcanza el límite inferior
            brillo = 0; //no rebasar límite
            brilloDelta *= -1; //cambiar signo para incrementar el brillo
          }
          //pixels.setPixelColor(0, brillo, brillo, brillo); //encender solo un pixel
          pixels.setPixelColor(0, (color >> 16) & brillo, (color >> 8) & brillo, color & brillo);
          pixels.show();
        }
      }

      if (gestoReco == GOLPE_H) { //si se hace un golpe horizontal
        estado = OFF;
      }
      if (gestoReco == GIRO_D or gestoReco == GIRO_I) { //si se realiza un giro
        colorIndex ++; //incrementar index
        if (colorIndex >= NUMCOLORES) colorIndex = 0;
        color = colores[ colorIndex]; //asignar siguiente color
      }
    } //fin if (estado ...)
  }// fin de nDevices

  if (cajaMotorConec == SI)
  {
    farosLuz();
  }
} //fin void loop()

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

//------------------------------------------------- buscar I2C
void buscarI2C()
{
  nDevices = 0;
  cajaSonidoConec = NO;
  cajaMotorConec = NO;
  {
    Serial.println("Busca i2c");
    for (address = 1; address < 12; address++ )
    {
      // The i2c_scanner uses the return value of
      // the Write.endTransmisstion to see if
      // a device did acknowledge to the address.
      Wire.beginTransmission(address);
      error = Wire.endTransmission();

      if (error == 0)
      {
        //Serial.println("Hay i2c");
        nDevices++;
        switch (address)
        {
          case CAJA_MOTOR:  //Caja de motor conectada
            //leerEsclavo(CAJA_MOTOR, 1);
            cajaMotorConec = SI;
            break;

          case CAJA_SONIDO:  //Sonido
            leerEsclavo(CAJA_SONIDO, 1);
            cajaSonidoConec = SI;
            distance = datos[0];
            Serial.println("Caja Sonido");
            break;
        }
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
  Wire.write(map(analogRead(LUZ_DER_PIN), 0, 1023, 0, 255));
  Wire.write(map(analogRead(LUZ_IZQ_PIN), 0, 1023, 0, 255));
  Serial.println("SEND");
}

void farosLuz()
{
  pixels.setPixelColor(0, 255, 255, 255);
  pixels.setPixelColor(1, 255, 255, 255);
  pixels.show();
  delay(500);
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

