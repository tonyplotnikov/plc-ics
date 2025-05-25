
extern int accel_x_OC;
extern int accel_y_OC;
extern int accel_z_OC;
extern int gyro_x_OC;
extern int gyro_y_OC;
extern int gyro_z_OC;

extern float temp_scalled;
extern float accel_x_scalled;
extern float accel_y_scalled;
extern float accel_z_scalled;
extern float gyro_x_scalled;
extern float gyro_y_scalled;
extern float gyro_z_scalled;

void MPU6050_ReadData();
void MPU6050_ResetWake();
void MPU6050_SetDLPF(int BW);
void MPU6050_SetGains(int gyro, int accel);
void MPU6050_OffsetCal();
