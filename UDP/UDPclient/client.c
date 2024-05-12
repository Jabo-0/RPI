/* client.c */
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <linux/i2c.h>
#include <pthread.h>
#include <signal.h>


#define BUF_SIZE 500
//#define PORT 60000

int sfd;
int bucle = 33;

pthread_t t1, t2;

void handler(int sig){

  signal(SIGINT, SIG_DFL);
  pthread_cancel(t1);
  pthread_cancel(t2);
  
  bucle = 0;

}

// Acelerometer

void mpu();

#define MPU6000_I2C_ADDR 0x68
#define MPU6000_ACCEL_XOUT_H 0x3B
#define MPU6000_PWR_MGMT_1 0x6B
#define MPU6000_SCALE_FACTOR 16384.0

int fd_mpu;

typedef struct {
	__uint16_t accel_x;
	__uint16_t accel_y;
	__uint16_t accel_z;
} t_accel_data;


//Color sensor

void tcs();

#define TCS34725_I2C_ADDR 0x29
#define TCS34725_ENABLE_REG 0x03
#define TCS34725_CMD_REG 0x80
#define TCS34725_CMD_REG_RD 0xA0
#define TCS34725_COLOR_OUT 0x14

int fd_tcs;

typedef struct{
    __uint8_t Red;
    __uint8_t Green;
    __uint8_t Blue;
    __uint8_t Light;
}t_color_data;

//-----------------

typedef struct{
    char data[10];
}t_send_data;


t_accel_data mpu_udp[10];

t_color_data tcs_udp[10];

t_send_data send_data[10];

int main(int argc, char *argv[]) {

  struct addrinfo hints;
  struct addrinfo *result, *rp;
  int s, j, x;
  size_t len;
  ssize_t nread;
  char buf[BUF_SIZE];
  char bufOut[100];

  signal(SIGINT, handler);

  printf("Client start\n");
  fflush(stdout);

  if(pthread_create(&t1, NULL, (void*)mpu, NULL) != 0){
    printf("Error creating thread\n");
    fflush(stdout);
    return 1;
  }

  if(pthread_create(&t2, NULL, (void*)tcs, NULL) != 0){
    printf("Error creating thread\n");
    fflush(stdout);
    return 1;
  }

  if (argc < 3) {
      fprintf(stderr, "Usage: %s host port msg...\n", argv[0]);
      exit(EXIT_FAILURE);
  }

  while(bucle){

    /* Obtain address(es) matching host/port */

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;    /* Allow IPv4 or IPv6 */
    hints.ai_socktype = SOCK_DGRAM; /* Datagram socket */
    hints.ai_flags = 0;
    hints.ai_protocol = 0;          /* Any protocol */

    s = getaddrinfo(argv[1], argv[2], &hints, &result);
    if (s != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
        exit(EXIT_FAILURE);
    }

    /* getaddrinfo() returns a list of address structures.
    Try each address until we successfully connect(2).
    If socket(2) (or connect(2)) fails, we (close the socket
    and) try the next address. */

    for (rp = result; rp != NULL; rp = rp->ai_next) {
        sfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (sfd == -1)
            continue;

        if (connect(sfd, rp->ai_addr, rp->ai_addrlen) != -1)
            break;                  /* Success */

        close(sfd);
    }

    if (rp == NULL) {               /* No address succeeded */
        fprintf(stderr, "Could not connect\n");
        exit(EXIT_FAILURE);
    }

    freeaddrinfo(result);           /* No longer needed */

    /* Send remaining command-line arguments as separate
    datagrams, and read responses from server */
    for(int i = 0; i < 10; i++){

      send_data[i].data[0] = tcs_udp[i].Red;
      send_data[i].data[1] = tcs_udp[i].Green;
      send_data[i].data[2] = tcs_udp[i].Blue;
      send_data[i].data[3] = tcs_udp[i].Light;
      send_data[i].data[4] = mpu_udp[i].accel_x >> 8;
      send_data[i].data[5] = mpu_udp[i].accel_x;
      send_data[i].data[6] = mpu_udp[i].accel_y >> 8;
      send_data[i].data[7] = mpu_udp[i].accel_y;
      send_data[i].data[8] = mpu_udp[i].accel_z >> 8;
      send_data[i].data[9] = mpu_udp[i].accel_z;

      ///*
      float accel_x_data = (mpu_udp[i].accel_x/MPU6000_SCALE_FACTOR)*9.8;
      printf("ite: %d green:%d  red: %d ax: %.2f \n", i,tcs_udp[i].Green, tcs_udp[i].Red, accel_x_data);
      fflush(stdout);
      //*/
    }

    x = 0;

    for(int i = 0; i < 10; i++){
      for(int j = 0; j < 10; j++){
        bufOut[x] = send_data[i].data[j];
        x++;
      }
    }

    x = write(sfd, bufOut, 100);

    if (x != 100) {
          fprintf(stderr, "partial/failed write\n");
          fflush(stdout);
          //exit(EXIT_FAILURE);
    }

    printf("Send %d bytes\n", x);
    fflush(stdout);

        //nread = read(sfd, buf, BUF_SIZE);
        //if (nread == -1) {
            //perror("read");
            //exit(EXIT_FAILURE);
        //}

    //recepcion

        //printf("Received %zd bytes: %s\n", nread, buf);
        //for(int i = 0;i < nread; i++){
        //        printf("%c", buf[i]);
        //        fflush(stdout);
        //    }
    printf("mimimimimimi\n");
    fflush(stdout);
    sleep(10);
    printf("I wake up\n");
    fflush(stdout);
    }
    

  if(pthread_join(t1, NULL) != 0){
    printf("Error joining thread\n");
    fflush(stdout);
    return 1;
  }
  
  if(pthread_join(t2, NULL) != 0){
    printf("Error joining thread\n");
    fflush(stdout);
    return 1;
  }

  printf("\n Program terminated.\n");

  close(fd_mpu);
  close(fd_tcs);

  exit(EXIT_SUCCESS);

}

//hay que cambirle muchas cosas a esto para adaptarlo

//Acelerometer

void mpu(){

  struct i2c_rdwr_ioctl_data packets;
  struct i2c_msg messages[2];
  char i2cFile[15];
  int device = 1;
  int addr = MPU6000_I2C_ADDR;

  uint8_t jota;

  printf("Accelerometer started\n");
  fflush(stdout);
  
  sprintf(i2cFile, "/dev/i2c-%d", device);
  fd_mpu = open(i2cFile,O_RDWR);
  ioctl(fd_mpu,I2C_SLAVE,addr);

  // Wake up the MPU6000
  char data[2] = {MPU6000_PWR_MGMT_1, 0};
  messages[0].addr = addr;
  messages[0].flags = 0;
  messages[0].len = sizeof(data);
  messages[0].buf = data;

  packets.msgs = messages;
  packets.nmsgs = 1;

  ioctl(fd_mpu, I2C_RDWR, &packets);

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

    ioctl(fd_mpu, I2C_RDWR, &packets);

    // Convert the accelerometer data

    mpu_udp[jota].accel_x = (accel_data[0] << 8) | accel_data[1];
    mpu_udp[jota].accel_y = (accel_data[2] << 8) | accel_data[3];
    mpu_udp[jota].accel_z = (accel_data[4] << 8) | accel_data[5];

    // Print the accelerometer data
    ///*
    float accel_x_data = (mpu_udp[jota].accel_x/MPU6000_SCALE_FACTOR)*9.8;
    float accel_y_data = (mpu_udp[jota].accel_y/MPU6000_SCALE_FACTOR)*9.8;
    float accel_z_data = (mpu_udp[jota].accel_z/MPU6000_SCALE_FACTOR)*9.8;
    printf("ite: %d x: %.2f   y: %.2f  z: %.2f \n", jota, accel_x_data, accel_y_data, accel_z_data);
    fflush(stdout);
    //*/
    if((mpu_udp[jota].accel_x && mpu_udp[jota].accel_y) == 0){
      printf("Sensor disconnected");

      // Wake up the MPU6000
      char data[2] = {MPU6000_PWR_MGMT_1, 0};
      messages[0].addr = addr;
      messages[0].flags = 0;
      messages[0].len = sizeof(data);
      messages[0].buf = data;

      packets.msgs = messages;
      packets.nmsgs = 1;

      ioctl(fd_mpu, I2C_RDWR, &packets);

      }
            
      if(jota < 9)
        jota++;
      else jota = 0;
    // Sleep for 1 seconds
    sleep(1);
  }
}

//Color Sensor

void tcs(){

  struct i2c_rdwr_ioctl_data packets;
  struct i2c_msg messages[2];
  char i2cFile[15];
  int device = 1;
  int addr = TCS34725_I2C_ADDR;

  uint8_t ca;

  printf("Color sensor start\n");
  fflush(stdout);

  sprintf(i2cFile, "/dev/i2c-%d", device);
  fd_tcs = open(i2cFile,O_RDWR);

  if(!ioctl(fd_tcs,I2C_SLAVE,addr)){
    printf("Error in I2C\n");
    fflush(stdout);
  }
  // Wake up the TCS34725
  char data[2];
  data[0] = TCS34725_CMD_REG; 
  data[1]= TCS34725_ENABLE_REG;
  messages[0].addr = addr;
  messages[0].flags = 0;
  messages[0].len = sizeof(data);
  messages[0].buf = data;

  packets.msgs = messages;
  packets.nmsgs = 1;

  if(!ioctl(fd_tcs, I2C_RDWR, &packets)){
    printf("Error in I2C\n");
    fflush(stdout);
  }

  char reg = TCS34725_CMD_REG_RD + TCS34725_COLOR_OUT;
  char raw_color_data[8];

  while(33){
    // Read color sensor data

    messages[0].addr = addr;
    messages[0].flags = 0;
    messages[0].len = sizeof(reg);
    messages[0].buf = &reg;

    messages[1].addr = addr;
    messages[1].flags = I2C_M_RD;
    messages[1].len = sizeof(raw_color_data);
    messages[1].buf = raw_color_data;

    packets.msgs = messages;
    packets.nmsgs = 2;

    if(!ioctl(fd_tcs, I2C_RDWR, &packets)){
      printf("Error in I2C\n");
      messages[0].addr = addr;
      messages[0].flags = 0;
      messages[0].len = sizeof(data);
      messages[0].buf = data;

      packets.msgs = messages;
      packets.nmsgs = 1;

      if(!ioctl(fd_tcs, I2C_RDWR, &packets)){
        printf("I2C disconnected\n");
        fflush(stdout);
      }
    }

    // Convert the color data

    tcs_udp[ca].Light = ((raw_color_data[0] << 8) | raw_color_data[1])/256;
    tcs_udp[ca].Red = ((raw_color_data[2] << 8) | raw_color_data[3])/256;
    tcs_udp[ca].Green = ((raw_color_data[4] << 8) | raw_color_data[5])/256;
    tcs_udp[ca].Blue = ((raw_color_data[6] << 8) | raw_color_data[7])/256;

    ///*
    printf("Light intensity: %d   Red: %d   Green: %d   Blue: %d   ite: %d\n", tcs_udp[ca].Light, tcs_udp[ca].Red, tcs_udp[ca].Green, tcs_udp[ca].Blue, ca);    
    fflush(stdout);
    //*/
    
    if(ca < 9)
        ca++;
      else ca = 0;

    sleep(1);

  }
}


























