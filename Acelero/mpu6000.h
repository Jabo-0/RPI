#include <stdio.h>
#include <stdlib.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <linux/i2c.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>

#define MPU6000_I2C_ADDR 0x68
#define MPU6000_ACCEL_XOUT_H 0x3B
#define MPU6000_PWR_MGMT_1 0x6B
#define MPU6000_SCALE_FACTOR 16384.0

int mpu(t_accel_data *data);

typedef struct {
	float accel_x;
	float accel_y;
	float accel_z;
} t_accel_data;
