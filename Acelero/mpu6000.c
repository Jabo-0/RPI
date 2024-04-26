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

pthread_t t1;

int fd;

void manejador(int sig){

  signal(SIGINT, SIG_DFL);
  pthread_cancel(t1);

}


void medir();

int main(void){




  signal(SIGINT, manejador);

  if(pthread_create(&t1, NULL, (void*)medir, NULL) != 0){
    printf("Error creating thread\n");
    return 1;
  }


  if(pthread_join(t1, NULL) != 0){
    printf("Error joining thread\n");
    return 1;
  }

  printf("ANCEL \n");

  close(fd);

  return 0;
}


void medir(){

  struct i2c_rdwr_ioctl_data packets;
  struct i2c_msg messages[2];
  char i2cFile[15];
  int device = 1;
  int addr = MPU6000_I2C_ADDR;

  printf("Hello and welcome to the accelerometer program\n");
  fflush(stdout);
  sleep(1);
  sprintf(i2cFile, "/dev/i2c-%d", device);
  fd = open(i2cFile,O_RDWR);
  ioctl(fd,I2C_SLAVE,addr);

  // Wake up the MPU6000
  char data[2] = {MPU6000_PWR_MGMT_1, 0};
  messages[0].addr = addr;
  messages[0].flags = 0;
  messages[0].len = sizeof(data);
  messages[0].buf = data;

  packets.msgs = messages;
  packets.nmsgs = 1;

  ioctl(fd, I2C_RDWR, &packets);

  char reg = MPU6000_ACCEL_XOUT_H;
  char accel_data[6];

  while(33){
    // Read accelerometer data


    messages[0].addr = addr;
    messages[0].flags = 0;
    messages[0].len = sizeof(reg);
    messages[0].buf = &reg;

    messages[1].addr = addr;
    messages[1].flags = I2C_M_RD;
    messages[1].len = sizeof(accel_data);
    messages[1].buf = accel_data;

    packets.msgs = messages;
    packets.nmsgs = 2;

    ioctl(fd, I2C_RDWR, &packets);

    // Convert the accelerometer data

    short ax = (accel_data[0] << 8) | accel_data[1];
    short ay = (accel_data[2] << 8) | accel_data[3];
    short az = (accel_data[4] << 8) | accel_data[5];

    float ax_m_s2 = ((ax / MPU6000_SCALE_FACTOR) * 9.8);
    float ay_m_s2 = ((ay / MPU6000_SCALE_FACTOR) * 9.8);
    float az_m_s2 = ((az / MPU6000_SCALE_FACTOR) * 9.8)-2.4;

    // Print the accelerometer data
    system("clear");
    printf("Accelerometer data:\n ax=%.2f m/s², ay=%.2f m/s², az=%.2f m/s² \n", ax_m_s2, ay_m_s2, az_m_s2);
    if((ax_m_s2 && ay_m_s2) == 0){
      printf("Sensor disconnected");

      // Wake up the MPU6000
      char data[2] = {MPU6000_PWR_MGMT_1, 0};
      messages[0].addr = addr;
      messages[0].flags = 0;
      messages[0].len = sizeof(data);
      messages[0].buf = data;

      packets.msgs = messages;
      packets.nmsgs = 1;

      ioctl(fd, I2C_RDWR, &packets);}
    // Sleep for 3 seconds
    sleep(1);
  }
}
