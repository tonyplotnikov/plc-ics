#include <Arduino.h>
#include <Wire.h>
#include "gyro_accel.h"


#define MPU6050_address 0x68

#define MPU6050_self_test_x 13 
#define MPU6050_self_test_y 14 
#define MPU6050_self_test_z 15 
#define MPU6050_self_test_A 16 


#define MPU6050_sample_div 25


#define MPU6050_config 26       
#define MPU6050_gyro_config 27  
#define MPU6050_accel_config 28 


#define MPU6050_data_start 59


#define MPU6050_PWR1 107
#define MPU6050_PWR2 108


#define g 9.81 

int temp = 0;
int accel_x = 0;
int accel_y = 90;
int accel_z = 0;
int gyro_x = 0;
int gyro_y = 90;
int gyro_z = 0;

int accel_x_OC = 0;
int accel_y_OC = 90;
int accel_z_OC = 0;
int gyro_x_OC = 0;
int gyro_y_OC = 90;
int gyro_z_OC = 0;

float temp_scalled;
float accel_x_scalled;
float accel_y_scalled;
float accel_z_scalled;
float gyro_x_scalled;
float gyro_y_scalled;
float gyro_z_scalled;
float accel_scale_fact = 1;
float gyro_scale_fact = 1; 


void MPU6050_ReadData()
{

  Wire.beginTransmission(MPU6050_address);
  Wire.write(MPU6050_data_start);
  Wire.endTransmission();

  int read_bytes = 14;

  Wire.requestFrom(MPU6050_address, read_bytes);

  if (Wire.available() == read_bytes)
  {

    accel_x = Wire.read() << 8 | Wire.read();
    accel_y = Wire.read() << 8 | Wire.read();
    accel_z = Wire.read() << 8 | Wire.read();

    temp = Wire.read() << 8 | Wire.read();

    gyro_x = Wire.read() << 8 | Wire.read();
    gyro_y = Wire.read() << 8 | Wire.read();
    gyro_z = Wire.read() << 8 | Wire.read();
  }

  accel_x_scalled = (float)(accel_x - accel_x_OC) * accel_scale_fact / 1000; 
  accel_y_scalled = (float)(accel_y - accel_y_OC) * accel_scale_fact / 1000;
  accel_z_scalled = (float)(accel_z - accel_z_OC) * accel_scale_fact / 1000;

  gyro_x_scalled = (float)(gyro_x - gyro_x_OC) * gyro_scale_fact / 1000;
  gyro_y_scalled = (float)(gyro_y - gyro_y_OC) * gyro_scale_fact / 1000;
  gyro_z_scalled = ((float)(gyro_z - gyro_z_OC) * gyro_scale_fact / 1000);

  temp_scalled = (float)temp / 340 + 36.53;
}


void MPU6050_ResetWake() 
{

  Serial.println("Resetting MPU6050 and waking it up.....");
  Wire.beginTransmission(MPU6050_address);
  Wire.write(MPU6050_PWR1);
  Wire.write(0b10000000);
  Wire.endTransmission();

  delay(100); 

  Wire.beginTransmission(MPU6050_address);
  Wire.write(MPU6050_PWR1);

  Wire.write(0b00000000);
  Wire.endTransmission();
}


void MPU6050_SetDLPF(int BW)
{
  if (BW < 0 || BW > 6)
  {
    BW = 0;
  }
  Wire.beginTransmission(MPU6050_address);
  Wire.write(MPU6050_config);

  Wire.write(BW);
  Wire.endTransmission();
}


void MPU6050_SetGains(int gyro, int accel)
{
  byte gyro_byte, accel_byte;


  Wire.beginTransmission(MPU6050_address);
  Wire.write(MPU6050_gyro_config); 
  if (gyro == 0)
  {
    gyro_scale_fact = (float)250 * 0.0305; 
    gyro_byte = 0b00000000;
  }
  else if (gyro == 1)
  {
    gyro_scale_fact = 500 * 0.0305; 
    gyro_byte = 0b00001000;
  }
  else if (gyro == 2)
  {
    gyro_scale_fact = 1000 * 0.0305; 
    gyro_byte = 0b00010000;
  }
  else if (gyro == 3)
  {
    gyro_scale_fact = 2000 * 0.0305; 
    gyro_byte = 0b00011000;
  }
  else
  {
    gyro_scale_fact = 1;
  }

  Wire.write(gyro_byte);
  Wire.endTransmission();
  Serial.print("The gyro scale is set to ");
  Serial.print(gyro_scale_fact);
  Serial.println(" milli Degree/s");


  Wire.beginTransmission(MPU6050_address);
  Wire.write(MPU6050_accel_config); 
  if (accel == 0)
  {
    accel_scale_fact = (float)2 * g * 0.0305; 
    accel_byte = 0b00000000;
  }
  else if (accel == 1)
  {
    accel_scale_fact = 4 * g * 0.0305; 
    accel_byte = 0b00001000;
  }
  else if (accel == 2)
  {
    accel_scale_fact = 8 * g * 0.0305; 
    accel_byte = 0b00010000;
  }
  else if (accel == 3)
  {
    accel_scale_fact = 16 * g * 0.0305; 
    accel_byte = 0b00011000;
  }
  else
  {
    accel_scale_fact = 1;
  }

  Wire.write(accel_byte);
  Wire.endTransmission();
  Serial.print("The accel scale is set to ");
  Serial.print(accel_scale_fact);
  Serial.println(" milli m/s^2");
}

void MPU6050_OffsetCal()
{
  Serial.println("Calibrating gyroscope .... dont move the hardware ..........");

  int x = 0, y = 0, z = 0, i;

  MPU6050_ReadData();
  MPU6050_ReadData();


  x = gyro_x;
  y = gyro_y;
  z = gyro_z;

  for (i = 1; i <= 1000; i++)
  {
    MPU6050_ReadData();
    x = (x + gyro_x) / 2;
    y = (y + gyro_y) / 2;
    z = (z + gyro_z) / 2;
    Serial.print(".");
  }
  Serial.println(".");
  gyro_x_OC = x;
  gyro_y_OC = y;
  gyro_z_OC = z;

  Serial.print("gyro_x register offset = ");
  Serial.println(x);

  Serial.print("gyro_y register offect = ");
  Serial.println(y);

  Serial.print("gyro_z register offset = ");
  Serial.println(z);


  Serial.println("Calibrating accelrometer .... dont move the hardware ..........");
  x = accel_x;
  y = accel_y;
  z = accel_z;

  for (i = 1; i <= 1000; i++)
  {
    MPU6050_ReadData();
    x = (x + accel_x) / 2;
    y = (y + accel_y) / 2;
    z = (z + accel_z) / 2;
    Serial.print(".");
  }
  Serial.println(".");
  accel_x_OC = x;
  accel_y_OC = y;
  accel_z_OC = z - (float)g * 1000 / accel_scale_fact;

  Serial.print("Accel_x register offset = ");
  Serial.println(x);

  Serial.print("Accel_y register offect = ");
  Serial.println(y);

  Serial.print("Accel_z register offset = ");
  Serial.println(z);
}
