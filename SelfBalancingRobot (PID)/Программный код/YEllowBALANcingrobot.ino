#include <Wire.h>       // шина I2C 
#include "PID_v1.h"     // PID
#include "gyro_accel.h" // mpu6050

// вычисления угла наклона, постоянные
#define dt 20            // промежуток времени в милисикундах
#define rad2degree 57.3  // переменная для перевода из радиан в градусы
#define Filter_gain 0.95 // переменная для фильтра angle = angle_gyro*Filter_gain + angle_accel*(1-Filter_gain)

#define SND_PIN 6 // пищалка

// Глобальные переменные
unsigned long t = 0;
float y = 0;
float angle_x_gyro = 0;
float angle_y_gyro = 0;
float angle_z_gyro = 0;
float angle_x_accel = 0;
float angle_y_accel = 0;
float angle_z_accel = 0;
float angle_x = 0;
float angle_y = 0;
float angle_z = 0;

// Настройка пинов для моторов
char PWM1[] = {11, 9};      // Номера пинов для ШИМ управления
char Umotor_1[] = {10, 12}; // Подключение первого двигателя
char Umotor_2[] = {7, 8};   // Подключение первого двигателя

// Переменные для ПИД регулятора
double originalSetpoint = -0.6; // Требуемое значение стабилизации
double movingAngleOffset = 0.1;
double input, output;
double Kp = 7;  // 7 - лучшее значение, подобранное мной (предположительно - лучше не сделать)
double Kd = 0.3; // 0.3 - аналогично
double Ki = 46; // здесь 46, диапазон +-1.5
// Вызов ПИД регулятора
PID pid(&input, &output, &originalSetpoint, Kp, Ki, Kd, DIRECT);

// Функция для управления первым мотором
void MOTOR1(int PWM_1)
{
  if (PWM_1 > 0)
  {
    digitalWrite(Umotor_1[0], HIGH);
    digitalWrite(Umotor_1[1], LOW);
    analogWrite(PWM1[0], PWM_1);
  }
  else
  {
    digitalWrite(Umotor_1[0], LOW);
    digitalWrite(Umotor_1[1], HIGH);
    analogWrite(PWM1[0], abs(PWM_1));
  }
}
// Функция для управления вторым мотором
void MOTOR2(int PWM_2)
{
  if (PWM_2 > 0)
  {
    digitalWrite(Umotor_2[0], LOW);
    digitalWrite(Umotor_2[1], HIGH);
    analogWrite(PWM1[1], PWM_2);
  }
  else
  {
    digitalWrite(Umotor_2[0], HIGH);
    digitalWrite(Umotor_2[1], LOW);
    analogWrite(PWM1[1], abs(PWM_2));
  }
}

// Основной код
void setup()
{
  for (char pin = 0; pin <= 2; pin++)
    pinMode(PWM1[pin], OUTPUT);
  for (char pin1 = 0; pin1 <= 2; pin1++)
    pinMode(Umotor_1[pin1], OUTPUT);
  for (char pin2 = 0; pin2 <= 2; pin2++)
    pinMode(Umotor_2[pin2], OUTPUT);
  Serial.begin(115200);   // Общение с компьютером на частоте 115200
  Wire.begin();           // Работа с I2C шиной
  MPU6050_ResetWake();    // Сбрасывает настройки по умолчанию
  MPU6050_SetGains(0, 1); // Настраивает максимальные значения шкалы измерений гироскопа и акселерометра
  MPU6050_SetDLPF(0);     // Настройка фильтра низких частот
  MPU6050_OffsetCal();    // Калибровка гироскопа и акселерометра
  MPU6050_SetDLPF(6);     // Настройка фильтра низких частот
  Serial.print("\tangle_y_accel");
  Serial.print("\tangle_y");
  Serial.println("\tLoad");
  t = millis();
  pid.SetMode(AUTOMATIC);
  pid.SetSampleTime(10);
  pid.SetOutputLimits(-255, 255); // Ограничение крайних значений PID 

  // Настройка порта для управления пищалкой
  pinMode(SND_PIN, OUTPUT);
  digitalWrite(SND_PIN, HIGH);
  delay(30);
  digitalWrite(SND_PIN, LOW);
  delay(60);
  digitalWrite(SND_PIN, HIGH);
  delay(120);
  digitalWrite(SND_PIN, LOW);
}

void loop()
{
  t = millis();
  MPU6050_ReadData(); // Получение данных с датчика 
  // Угловая скорость относительно всех осей
  angle_y_gyro = (gyro_y_scalled * ((float)dt / 1000) + angle_y);
  // Угол поворота относительно всех осей
  angle_y_accel = -atan(accel_x_scalled / (sqrt(accel_y_scalled * accel_y_scalled + accel_z_scalled * accel_z_scalled))) * (float)rad2degree;
  // Отфильтрованные значения угла поворота относительно всех осей
  angle_y = Filter_gain * angle_y_gyro + (1 - Filter_gain) * angle_y_accel;
  Serial.print(angle_y);
  Serial.println("\t");
  // Вывод оси времени
  // Serial.println(((float)(millis()-t)/(float)dt)*100);
  while ((millis() - t) < dt)
  { 
  }
  input = angle_y;  // Угол наклона относительно оси OY является входным параметром для ПИД регулятора
  pid.Compute();    // Функция для вычисления ШИМ сигнала через полученные на вход значения текущего угла
  int PWM = output; // Передача выходного ШИМ на двигатели
  Serial.print(PWM);
  Serial.print(" ");
  MOTOR1(PWM);
  MOTOR2(PWM);
}
