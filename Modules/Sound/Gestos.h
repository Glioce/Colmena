//

// ======================================== Definiciones
// Tipos de gestos
#define NINGUNO 0 //no hay gesto
#define GOLPE_V 1 //golpe vertical
#define GOLPE_H 2 //golpe horizontal
#define GIRO_D  3 //giro derecha
#define GIRO_I  4 //giro izquierda
#define MUERTO  5 //muertito

// Periodos de espera
#define T_ESP_PICOS  200; //tiempo de espera entre picos
#define T_ESP_GESTOS 300; //tiempo de espera entre reconocmiento de gestos

// Detección de gestos
unsigned long tFin = 0; //tiempo final (se usa para cancelar un gesto)
unsigned long tGesto = 0; //momento de reconocer otro gesto
byte gestoPicoNum = 1; //número de pico a detectar
byte gestoEnCurso = NINGUNO; //todavía no se inica la detección de un gesto
byte gestoReco = NINGUNO; //gesto reconocido

// Acelerómetro
const int G = 16384; //en configuración por defecto, la aceleración de 1 g se aproxima a ese valor
const float G2 = pow(16384.0 , 2); //g al cuadrado (se usa para reconocer golpe horizontal)
const int SATP = 32767; //saturación positiva
const int SATN = -32768; //saturación negativa

// MPU-6050
const int MPU = 0x68;  //I2C address
int16_t AcX, AcY, AcZ; //acelerómetro
int16_t Tmp;           //termómetro
int16_t GyX, GyY, GyZ; //giroscopio


// ====================================================================== Leer Acelerómetro
void leerAc()
{
  //Serial.println("leerAc()");
  // En esta función solo se leen los valores del
  // MPU-6050 que se utilizan en la función gestos()
  Swire.beginTransmission(MPU);
  Swire.write(0x3B); //starting with register 0x3B (ACCEL_XOUT_H)
  Swire.endTransmission(false);

  Swire.requestFrom(MPU, 14, true); //request a total of 14 registers
  AcX = Swire.read() << 8 | Swire.read(); //0x3B (ACCEL_XOUT_H) & 0x3C (ACCEL_XOUT_L)
  AcY = Swire.read() << 8 | Swire.read(); //0x3D (ACCEL_YOUT_H) & 0x3E (ACCEL_YOUT_L)
  AcZ = Swire.read() << 8 | Swire.read(); //0x3F (ACCEL_ZOUT_H) & 0x40 (ACCEL_ZOUT_L)
  Tmp = Swire.read() << 8 | Swire.read(); //0x41 (TEMP_OUT_H)   & 0x42 (TEMP_OUT_L)
  GyX = Swire.read() << 8 | Swire.read(); //0x43 (GYRO_XOUT_H)  & 0x44 (GYRO_XOUT_L)
  GyY = Swire.read() << 8 | Swire.read(); //0x45 (GYRO_YOUT_H)  & 0x46 (GYRO_YOUT_L)
  GyZ = Swire.read() << 8 | Swire.read(); //0x47 (GYRO_ZOUT_H)  & 0x48 (GYRO_ZOUT_L)

  // Imprimir valores para depurar
  //  Serial.print(AcX);
  //  Serial.print("\t");
  //  Serial.print(AcY);
  //  Serial.print("\t");
  //  Serial.println(AcZ);

} //fin void leerAc()


// ======================================================================================= Gestos
void gestos() {
  gestoReco = NINGUNO; //suponer que no se reconocerá un nuevo gesto
  if (millis() < tGesto) return; //si no es tiempo de reconocer otro gesto, salir
  //Serial.println("gestos()");
  leerAc(); //obtener valores de aceleración y giro
  // A partir de los valores obtenidos se determinan los gestos

  if (gestoEnCurso != NINGUNO) { //si hay un gesto en curso
    if (millis() > tFin)        //y se acaba el tiempo de espera
      gestoEnCurso = NINGUNO;   //terminar reconocimiento
  }

  switch (gestoEnCurso)
  {
    case NINGUNO: //si no hay un gesto en curso, reconocer uno nuevo

      if (AcZ >= SATP) { //si hay saturación de aceleración del eje Z (lado positivo)
        Serial.print(AcZ); Serial.println(" AcZ >= SATP");
        gestoEnCurso = GOLPE_V; //inicia el reconocimiento de "golpe vertical"
        tFin = millis() + T_ESP_PICOS; //asignar tiempo límite
        gestoPicoNum = 2; //se va a detectar el segundo pico
      }
      if (GyZ >= SATP) { //si hay saturación de giro en el eje Z (positivo)
        Serial.print(GyZ); Serial.println(" GyZ >= SATP");
        gestoEnCurso = GIRO_D; //inicia el reconocimiento de "giro derecha"
        tFin = millis() + T_ESP_PICOS; //asignar tiempo límite
        gestoPicoNum = 2; //se va a detectar el segundo pico
      }
      if (GyZ <= SATN) { //si hay saturación de giro en el eje Z (negativo)
        Serial.print(GyZ); Serial.println(" GyZ <= SATN");
        gestoEnCurso = GIRO_I; //inicia el reconocimiento de "giro derecha"
        tFin = millis() + T_ESP_PICOS; //asignar tiempo límite
        gestoPicoNum = 2; //se va a detectar el segundo pico
      }
      break; //case NINGUNO

    case GOLPE_V: //se está reconiciendo un golpe vertical
      //Serial.println(gestoEnCurso);
      if (gestoPicoNum == 2) { //se va a reconocer el segundo pico
        if (AcZ < -G) { //si la acel en Z es menor a -1 G
          Serial.print(AcZ); Serial.println(" AcZ < -G");
          tFin = millis() + T_ESP_PICOS; //nuevo tiempo límite
          gestoPicoNum = 3; //sigue el tercer pico
        }
      }
      else if (gestoPicoNum == 3) { //se va a reconocer el tercer pico
        if (AcZ >= SATP) { //si la acel en Z llega a saturación positiva
          Serial.println(AcZ);
          gestoReco = GOLPE_V; //se ha reconicido un golpe vertical
          gestoPicoNum = 1; //reinicar número
          gestoEnCurso = NINGUNO;
          tGesto = millis() + T_ESP_GESTOS; //esperar antes de reconocer otro gesto
        }
      }
      break; //case GOLPE_V

    case GIRO_D: //se está reconiciendo giro derecha
      if (gestoPicoNum == 2) { //se va a reconocer el segundo pico
        if (GyZ <= SATN) { //
          Serial.print(GyZ); Serial.println(" GyZ <= SATN");
          tFin = millis() + T_ESP_PICOS; //nuevo tiempo límite
          gestoPicoNum = 3; //sigue el tercer pico
        }
      }
      else if (gestoPicoNum == 3) { //se va a reconocer el tercer pico
        if (GyZ >= SATP) { //
          Serial.println(GyZ);
          gestoReco = GIRO_D; //se ha reconicido un giro derecha
          gestoPicoNum = 1; //reinicar número
          gestoEnCurso = NINGUNO;
          tGesto = millis() + T_ESP_GESTOS; //esperar antes de reconocer otro gesto
        }
      }
      break; //case GIRO_D

    case GIRO_I: //se está reconiciendo giro izquierda
      if (gestoPicoNum == 2) { //se va a reconocer el segundo pico
        if (GyZ >= SATP) { //
          Serial.print(GyZ); Serial.println(" GyZ >= SATP");
          tFin = millis() + T_ESP_PICOS; //nuevo tiempo límite
          gestoPicoNum = 3; //sigue el tercer pico
        }
      }
      else if (gestoPicoNum == 3) { //se va a reconocer el tercer pico
        if (GyZ <= SATN) { //
          Serial.println(GyZ);
          gestoReco = GIRO_I; //se ha reconicido un giro izquierda
          gestoPicoNum = 1; //reinicar número
          gestoEnCurso = NINGUNO;
          tGesto = millis() + T_ESP_GESTOS; //esperar antes de reconocer otro gesto
        }
      }
      break; //case GIRO_I

  } //fin switch (gestoEnCurso)
} //fin void gestos()
