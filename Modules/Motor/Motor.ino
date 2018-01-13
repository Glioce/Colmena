// PROYECTO: ROBOT MODULAR
// CAJA MOTOR v0.4 (6 nov 2017)
//
// Referencias
// MPU-6050 Datasheet
// http://www.instructables.com/id/Magic-Wand-TV-Remote/
// https://playground.arduino.cc/Main/SoftwareI2CLibrary
// https://www.arduino.cc/en/Hacking/Atmega168Hardware
// http://playground.arduino.cc/Main/MPU-6050
// https://learn.adafruit.com/adafruit-neopixel-uberguide/arduino-library-use

//======================================================================================== Direcciones i2c
#define CAJA_LUZ    1
#define CAJA_MOTOR  2
#define CAJA_SONIDO 3

// ======================================================================================= Definiciones
// Pines para motores (Puente H)
// Estos pines pueden funcionar como PWM
#define MOT_DA  9  //motor derecho A
#define MOT_DB  6  //motor derecho B
#define MOT_IA  11 //motor izquierdo A
#define MOT_IB  10 //motor izquierdo B
#define ADELANTE 0
#define DERECHA 1
#define IZQUIERDA 2
#define ATRAS 3
#define DETENIDO 4

// Estados
#define ON  1 //encendido
#define OFF 0 //apagado
#define SI 1
#define NO 0

// Pines para SoftWire (comunicación con acelerómetro)
// Estas definiciones se deben hacer antes de incluir la biblioteca
// En port D, el número de pin en el puerto corresponde con el número de pin en Arduino
#define SDA_PORT PORTD
#define SDA_PIN 3 //Digital 3
#define SCL_PORT PORTD
#define SCL_PIN 5 //Digital 5
#define LUZ_DER_PIN A6
#define LUZ_IZQ_PIN A7

#include <SoftWire.h> //incluir biblioteca de I2C por software (usa las definiciones anteriores)
SoftWire Swire = SoftWire(); //crear objeto de I2C por software
#include <Wire.h> //incluir biblioteca de I2C por hardware

#include "Gestos.h"

// =============================================================================== Variables Globales
// Estado y color/modo
byte estado = OFF;  //estado actual (apagado)
byte color = ADELANTE; //modo
//bool estadoIni = true; //indica el primer ciclo de un estado

// I2C
byte error, address;  //
int nDevices; //contador de dispositivos I2C
char letra;
String sDir = "000"; // Texto para la direccion del esclavo
int iDir;            // Entero para la direccion del esclavo
String sDat = "000"; // Texto para el dato
int iDat;            // Entero para el dato
const int MI_DIR = CAJA_MOTOR; //esta es la direccion de tu dispositivo, debe ser distinta en cada arduino
int libre;
byte datos[10];
byte luz_der, luz_izq; //intensidad de luz
byte faro = ADELANTE;
unsigned long tiempoAct;
unsigned long tiempoPas;
int tiempoEspera = 1009; //número primo

// dispositivos conectados
bool cajaSonidoConec = false;
bool cajaLuzConec = false;

// lado al que debe girar cuando está conectada la caja de sonido y detecta un objeto cercano
bool ladoGiro = DERECHA;

// ================================================================================== Configuración
void setup() {
  // Iniciar comunicación serial
  // (esto tal vez no es necesario en la versión final)
  Serial.begin(9600); //inicar puerto serie (para depurar programa)
  Serial.print("Caja Motor"); //imprimir presentación
  Serial.println(MI_DIR, HEX); //y dirección I2C

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

  // Pines de motores como salidas
  pinMode(MOT_DA, OUTPUT); //motor derecha A
  pinMode(MOT_DB, OUTPUT); //motor derecha B
  pinMode(MOT_IA, OUTPUT); //motor izquierda A
  pinMode(MOT_IB, OUTPUT); //motor izquierda B

  //adelante
  digitalWrite(MOT_DB, HIGH); //activar B
  digitalWrite(MOT_IB, HIGH);
  digitalWrite(MOT_IA, LOW); //desactivar A
  digitalWrite(MOT_DA, LOW);
  delay(100);
  digitalWrite(MOT_DB, LOW);
  digitalWrite(MOT_IB, LOW);
  digitalWrite(MOT_IA, LOW);
  digitalWrite(MOT_DA, LOW);

} //fin setup()

// ================================================================================== Principal
void loop()
{
  buscarI2C();
  delay(7);
} //fin void loop()

// =================================================================== buscar I2C
void buscarI2C()
{
  //Serial.println("Scanning...");
  nDevices = 0; // suponer que no hay disposirivos conectados
  cajaSonidoConec = false;
  cajaLuzConec = false;

  for (address = 1; address < 12; address++) //recorres todas las direcciones existentes
  {
    // The i2c_scanner uses the return value of the Write.endTransmisstion
    // to see if a device did acknowledge to the address.
    Wire.beginTransmission(address);
    error = Wire.endTransmission();

    if (error == 0) //si no hay error
    {
      //Serial.println("Hay i2c");
      nDevices ++; //incrementar contador
      switch (address) { //¿qué se conectó?
        case CAJA_SONIDO:
          cajaSonidoConec = true;
          color = ADELANTE;
          break;
        case CAJA_LUZ:
          cajaLuzConec = true;
          break;
      }
    }
  }

  if (cajaSonidoConec) {
     leerEsclavo(CAJA_SONIDO, 2); //pedir distancia
        Serial.println(datos[1]);
        // el valor va de 0 a 255 y equivale a 0 y 500 milímetros
        if (datos[1] < 50) 
        {
          color = random(1, 3); //1 o 2, es decir, DERECHA o IZQUIERDA
          //color = DERECHA;
        }
        else
        {
          color = ADELANTE;
        }
    // Si la caja de sonido está conectada no se pide información de caja luz
    switch (color)
    {
      case ADELANTE:
        digitalWrite(MOT_DB, HIGH); //activar B
        digitalWrite(MOT_IB, HIGH);
        digitalWrite(MOT_IA, LOW); //desactivar A
        digitalWrite(MOT_DA, LOW);
        break; //fin case ADELANTE

      case DERECHA:
        digitalWrite(MOT_DB, HIGH);
        digitalWrite(MOT_IB, LOW);
        digitalWrite(MOT_IA, HIGH);
        digitalWrite(MOT_DA, LOW);
        delay(250);
        //if (cambio()) color = ADELANTE;
        break; //fin case DERECHA

      case IZQUIERDA:
        digitalWrite(MOT_DB, LOW);
        digitalWrite(MOT_IB, HIGH);
        digitalWrite(MOT_IA, LOW);
        digitalWrite(MOT_DA, HIGH);
        delay(250);
        //if (cambio()) color = ADELANTE;
        break; //fin case IZQUIERDA

    } //fin switch (random)

  }
  else if (cajaLuzConec) {
    // Si la caja de sonido está conectada no se pide información de caja luz
    leerEsclavo(CAJA_LUZ, 2);
    seguirLuz();
    Serial.print(datos[0]);
    Serial.print("\t");
    Serial.println(datos[1]);
  }

  if ((nDevices == 0) && (error != 0))
  {
    Serial.println("No hay i2C");
    gestos();
    if (estado == OFF) //si apagado
    {
      digitalWrite(MOT_DB, LOW); //apagar motores
      digitalWrite(MOT_IB, LOW);
      digitalWrite(MOT_IA, LOW);
      digitalWrite(MOT_DA, LOW);
      if (gestoReco == GOLPE_V) { //si se hace un golpe vertical
        estado = ON; //cambiar a estado encendido
        Serial.println("estado = ON");
      }
    }
    else //si encendido
    {
      if (gestoReco == GOLPE_V) { //si se hace un golpe vertical
        estado = OFF; //apagar
        Serial.println("estado = OFF");
      }

      if (cambio())
        // cuando no hay nada conectado se mueve de forma aleatoria)
      {
        switch (random(ADELANTE, DETENIDO))
        {
          case ADELANTE:
            digitalWrite(MOT_DB, HIGH); //activar B
            digitalWrite(MOT_IB, HIGH);
            digitalWrite(MOT_IA, LOW); //desactivar A
            digitalWrite(MOT_DA, LOW);
            break; //fin case ROJO

          case ATRAS:
            digitalWrite(MOT_DB, LOW); //activar B
            digitalWrite(MOT_IB, LOW);
            digitalWrite(MOT_IA, HIGH); //desactivar A
            digitalWrite(MOT_DA, HIGH);
            break; //fin case AZUL

          case DERECHA:
            digitalWrite(MOT_DB, HIGH); //activar B
            digitalWrite(MOT_IB, LOW);
            digitalWrite(MOT_IA, HIGH); //desactivar A
            digitalWrite(MOT_DA, LOW);
            break; //fin case VERDE

          case IZQUIERDA:
            digitalWrite(MOT_DB, LOW); //activar B
            digitalWrite(MOT_IB, HIGH);
            digitalWrite(MOT_IA, LOW); //desactivar A
            digitalWrite(MOT_DA, HIGH);
            break; //fin case AMARILLO

        } //fin switch (random)
      } //fin if (cambio())
    } //fin if (estado == OFF) else
  } //fin if ((nDevices ...
} //fin void buscarI2C()

//---------------------------------------------- leerEsclavos
void leerEsclavo(int dir, int bytes)
{
  int i = 0;
  Wire.requestFrom(dir, bytes);    // request 5 bytes from slave device #4
  while (Wire.available())
  { // slave may send less than requested
    datos[i] = Wire.read(); // receive a byte as character
    //Serial.println(datos[i]);         // print the character
    i++;
  }
  //Serial.println();
}

//---------------------------------------------- requestEvent

void requestEvent()
{
  Wire.write(faro);
}

//================================================ seguirLuz
void seguirLuz()
{
  luz_der = datos[0];
  luz_izq = datos[1];

  //Si hay suficiente luz en ambos sensores se debe mover
  if ((luz_der > 200) || (luz_izq > 200))
  {

    if ((luz_izq - luz_der) > 50) // debo girar a la izquierda
    {
      digitalWrite(MOT_DB, LOW); //activar B
      digitalWrite(MOT_IB, HIGH);
      digitalWrite(MOT_IA, LOW); //desactivar A
      digitalWrite(MOT_DA, HIGH);
      faro = IZQUIERDA;
    }

    else if ((luz_der - luz_izq) > 50) // debe girar a la derecha
    {
      digitalWrite(MOT_DB, HIGH); //activar B
      digitalWrite(MOT_IB, LOW);
      digitalWrite(MOT_IA, HIGH); //desactivar A
      digitalWrite(MOT_DA, LOW);
      faro = DERECHA;
    }
    else                            // debe acvanzar
    {
      digitalWrite(MOT_DB, HIGH); //activar B
      digitalWrite(MOT_IB, HIGH);
      digitalWrite(MOT_IA, LOW); //desactivar A
      digitalWrite(MOT_DA, LOW);
      faro = ADELANTE;
    }
  }
  else                            // se detiene
  {
    digitalWrite(MOT_DB, LOW);
    digitalWrite(MOT_IB, LOW);
    digitalWrite(MOT_IA, LOW);
    digitalWrite(MOT_DA, LOW);
    faro = DETENIDO;
  }
  delay(110);
}

//=========================================== cambio
bool cambio()
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

