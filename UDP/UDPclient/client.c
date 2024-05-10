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

pthread_t t1, t2, t3;

void manejador(int sig){

  signal(SIGINT, SIG_DFL);
  pthread_cancel(t1);
  pthread_cancel(t2);
  pthread_cancel(t3);

}

// Acelerometer

#define MPU6000_I2C_ADDR 0x68
#define MPU6000_ACCEL_XOUT_H 0x3B
#define MPU6000_PWR_MGMT_1 0x6B
#define MPU6000_SCALE_FACTOR 16384.0

int fd_mpu;

typedef struct {
	float accel_x;
	float accel_y;
	float accel_z;
} t_accel_data;


//Color sensor

#define TCS34725_I2C_ADDR 0x29
#define TCS34725_ENABLE_REG 0x03
#define TCS34725_CMD_REG 0x80
#define TCS34725_CMD_REG_RD 0xA0
#define TCS34725_COLOR_OUT 0x14

int fd_tcs;

typedef struct{
    __uint16_t Red;
    __uint16_t Green;
    __uint16_t Blue;
    __uint16_t Light;
}t_color_data;

int main(int argc, char *argv[]) {

  signal(SIGINT, manejador);

  t_accel_data mpu_udp[10];

  t_color_data tcs_udp[10];

  if(pthread_create(&t1, NULL, (void*)udp_client, NULL) != 0){
    printf("Error creating thread\n");
    fflush(stdout);
    return 1;
  }
  if(pthread_create(&t2, NULL, (void*)mpu, NULL) != 0){
    printf("Error creating thread\n");
    fflush(stdout);
    return 1;
  }
  if(pthread_create(&t3, NULL, (void*)tcs, NULL) != 0){
    printf("Error creating thread\n");
    fflush(stdout);
    return 1;
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
  if(pthread_join(t3, NULL) != 0){
    printf("Error joining thread\n");
    fflush(stdout);
    return 1;
  }

  printf("ANCELAO \n");

  close(fd_mpu);
  close(fd_tcs);



}

//hay que cambirle muchas cosas a esto para adaptarlo

int udp_client() {
    struct addrinfo hints;
    struct addrinfo *result, *rp;
    int sfd, s, j;
    size_t len;
    ssize_t nread;
    char buf[BUF_SIZE];

    if (argc < 3) {
        fprintf(stderr, "Usage: %s host port msg...\n", argv[0]);
        exit(EXIT_FAILURE);
    }

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

  while(33){

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

    for (j = 3; j < argc; j++) {
        len = strlen(argv[j]) + 1;
        /* +1 for terminating null byte */

        if (len + 1 > BUF_SIZE) {
            fprintf(stderr, "Ignoring long message in argument %d\n", j);
            continue;
        }

        if (write(sfd, argv[j], len) != len) {
            fprintf(stderr, "partial/failed write\n");
            exit(EXIT_FAILURE);
        }

        nread = read(sfd, buf, BUF_SIZE);
        if (nread == -1) {
            perror("read");
            exit(EXIT_FAILURE);
        }
    }
        printf("Received %zd bytes: %s\n", nread, buf);
        for(int i = 0;i < nread; i++){
                printf("%c", buf[i]);
                fflush(stdout);
            }
        printf("\n");   
    }
    exit(EXIT_SUCCESS);
}



//Acelerometer


void mpu(){

  struct i2c_rdwr_ioctl_data packets;
  struct i2c_msg messages[2];
  char i2cFile[15];
  int device = 1;
  int addr = MPU6000_I2C_ADDR;

  uint8_t jota;

  t_accel_data data_mpu;

  printf("Hello and welcome to the accelerometer program\n");
  fflush(stdout);
  sleep(1);
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

    short ax = (accel_data[0] << 8) | accel_data[1];
    short ay = (accel_data[2] << 8) | accel_data[3];
    short az = (accel_data[4] << 8) | accel_data[5];

    mpu_udp[jota].accel_x = ((ax / MPU6000_SCALE_FACTOR) * 9.8);
    mpu_udp[jota].accel_y = ((ay / MPU6000_SCALE_FACTOR) * 9.8);
    mpu_udp[jota].accel_z = ((az / MPU6000_SCALE_FACTOR) * 9.8)-2.4;



    // Print the accelerometer data
    system("clear");
    printf("Accelerometer data:\n ax=%.2f m/s², ay=%.2f m/s², 
      az=%.2f m/s² \n", mpu_udp[jota].accel_x, mpu_udp[jota].accel_y, mpu_udp[jota].accel_z);
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
      
      
      if(jota < 10)
        jota++;
      else jota = 0;


      
      }
    // Sleep for 3 seconds
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

  printf("Hello and welcome to the color sensor program\n");
  fflush(stdout);
  sleep(1);
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

    t_color_data color_data;

    color_data.Light = ((color_data[0] << 8) | color_data[1])/256;
    color_data.Red = ((color_data[2] << 8) | color_data[3])/256;
    color_data.Green = ((color_data[4] << 8) | color_data[5])/256;
    color_data.Blue = ((color_data[6] << 8) | color_data[7])/256;

    




    // Print the color data

    system("clear");
    printf("Light intensity: %d   Red: %d   Green: %d   Blue: %d\n", color_data.Light, color_data.Red, color_data.Green, color_data.Blue);
    printf("Color: ");
    if(color_data.Red > 200 && color_data.Green > 200 && color_data.Blue > 200){
      printf("White\n");
    }else if(color_data.Red < 50 && color_data.Green < 50 && color_data.Blue < 50){
      printf("Black\n");
    }else if(color_data.Red < 100 && color_data.Green > 175 && color_data.Blue > 175){
      printf("Cyan\n");
    }else if(color_data.Red > 175 && color_data.Green > 175 && color_data.Blue < 100){
      printf("Yellow\n");
    }else if(color_data.Red > 175 && color_data.Green < 100 && color_data.Blue > 150){
      printf("Fuchsia\n");
    }else if(color_data.Red < 100 && color_data.Green < 100 && color_data.Blue > 175){
      printf("Blue\n");
    }else if(color_data.Red < 100 && color_data.Green > 175 && color_data.Blue < 100){
      printf("Green\n");
    }else if(color_data.Red > 175 && color_data.Green < 100 && color_data.Blue < 100){
      printf("Red\n");
    }else{
      printf("Gray\n");
    }
    fflush(stdout);

    sleep(1);
  }
}


























