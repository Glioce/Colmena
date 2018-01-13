// MPU-6050 Short Example Sketch
// By Arduino User JohnChi
// August 17, 2014
// Public Domain

// Pines para SoftSwire
// En port D, el número de pin en el puerto corresponde con el número de pin en Arduino
#define SDA_PORT PORTD
#define SDA_PIN 3 //Digital 3
#define SCL_PORT PORTD
#define SCL_PIN 5 //Digital 5

#include <SoftWire.h> //incluir biblioteca de I2C por software (usa las definiciones anteriores)
SoftWire Swire = SoftWire(); //crear objeto de I2C por software

const int MPU_addr = 0x68;  // I2C address of the MPU-6050
int16_t AcX, AcY, AcZ, Tmp, GyX, GyY, GyZ;

void setup() {
  Swire.begin();
  Swire.beginTransmission(MPU_addr);
  Swire.write(0x6B); // PWR_MGMT_1 register
  Swire.write(0); // set to zero (wakes up the MPU-6050)
  Swire.endTransmission(true);
  Serial.begin(9600);
}

void loop() {
  Swire.beginTransmission(MPU_addr);
  Swire.write(0x3B); // starting with register 0x3B (ACCEL_XOUT_H)
  Swire.endTransmission(false);

  Swire.requestFrom(MPU_addr, 14, true); // request a total of 14 registers
  AcX = Swire.read() << 8 | Swire.read(); // 0x3B (ACCEL_XOUT_H) & 0x3C (ACCEL_XOUT_L)
  AcY = Swire.read() << 8 | Swire.read(); // 0x3D (ACCEL_YOUT_H) & 0x3E (ACCEL_YOUT_L)
  AcZ = Swire.read() << 8 | Swire.read(); // 0x3F (ACCEL_ZOUT_H) & 0x40 (ACCEL_ZOUT_L)
  Tmp = Swire.read() << 8 | Swire.read(); // 0x41 (TEMP_OUT_H) & 0x42 (TEMP_OUT_L)
  GyX = Swire.read() << 8 | Swire.read(); // 0x43 (GYRO_XOUT_H) & 0x44 (GYRO_XOUT_L)
  GyY = Swire.read() << 8 | Swire.read(); // 0x45 (GYRO_YOUT_H) & 0x46 (GYRO_YOUT_L)
  GyZ = Swire.read() << 8 | Swire.read(); // 0x47 (GYRO_ZOUT_H) & 0x48 (GYRO_ZOUT_L)

//  Serial.print("AcX = "); Serial.print(AcX);
//  Serial.print(" | AcY = "); Serial.print(AcY);
//  Serial.print(" | AcZ = "); Serial.print(AcZ);
//  Serial.print(" | Tmp = "); Serial.print(Tmp / 340.00 + 36.53);  //equation for temperature in degrees C from datasheet
//  Serial.print(" | GyX = "); Serial.print(GyX);
//  Serial.print(" | GyY = "); Serial.print(GyY);
//  Serial.print(" | GyZ = "); Serial.println(GyZ);

//  Serial.print("AcX = "); Serial.print(AcX);
//  Serial.print("\tAcY = "); Serial.print(AcY);
//  Serial.print("\tAcZ = "); Serial.print(AcZ);
//  Serial.print("\tTmp = "); Serial.print(Tmp / 340.00 + 36.53);  //equation for temperature in degrees C from datasheet
  Serial.print("\tGyX = "); Serial.print(GyX);
  Serial.print("\tGyY = "); Serial.print(GyY);
  Serial.print("\tGyZ = "); Serial.print(GyZ);
  Serial.println();
  delay(10);
}
