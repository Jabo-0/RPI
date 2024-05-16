/* UDP code is from https://gist.github.com/saxbophone/f770e86ceff9d488396c0c32d47b757e */
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

#define MEAS_US 1000000   //1000000 every 10 seconds 10 measures are send 
#define MEAS (MEAS_US*10)
#define BUF_SIZE 500
//#define PORT 60000

int sfd;
int bucle = 33;

pthread_t t1, t2;

void handler(int sig){    // When control + C is used the program close itself closing all file descriptors (in the end of the program)

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
#define MPU6000_SCALE_FACTOR 16384

int fd_mpu;

typedef struct {
	int16_t accel_x;
	int16_t accel_y;
	int16_t accel_z;
} t_accel_data;


//Color sensor

void tcs();

#define TCS34725_I2C_ADDR 0x29
#define TCS34725_ENABLE_REG 0x03
#define TCS34725_CMD_REG 0x80
#define TCS34725_CMD_REG_RD 0xA0
#define TCS34725_COLOR_OUT 0x13

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

//the arguments needed are the ip of the server and the port used

int main(int argc, char *argv[]) {

  struct addrinfo hints;
  struct addrinfo *result, *rp;
  int s, j, x;
  size_t len;
  ssize_t nread;
  char buf[BUF_SIZE];
  char bufOut[100];

  signal(SIGINT, handler);

  system("clear");

  printf("Client start\n");
  fflush(stdout);

  if(pthread_create(&t1, NULL, (void*)mpu, NULL) != 0){     //threads for the color and the acelerometer
    printf("Error creating thread\n");
    fflush(stdout);
    return 1;
  }

  if(pthread_create(&t2, NULL, (void*)tcs, NULL) != 0){
    printf("Error creating thread\n");
    fflush(stdout);
    return 1;
  }

  if (argc < 3) {  //if not enough arguments the program ends 
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
    sleep(1);
    
    system("clear");

    while(bucle){

    usleep(MEAS); //waits the time of 10 measures

    system("clear");

    for(int i = 0; i < 10; i++){  // organice the information for the transmission

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

      /*//code used to know if data is send properly
      float accel_x_data = (mpu_udp[i].accel_x/MPU6000_SCALE_FACTOR)*9.8;
      uint16_t aux = send_data[i].data[4] << 8 | send_data[i].data[5];
      float accel_x_data2 = (aux/MPU6000_SCALE_FACTOR)*9.8;
      printf("ite: %d green:%d  red: %d ax: %.2f ax2: %.2f  \n", i,tcs_udp[i].Green, tcs_udp[i].Red, accel_x_data, accel_x_data2);
      fflush(stdout);
      //*/
    }

    x = 0;
    //data unification to be sent
    for(int i = 0; i < 10; i++){
      for(int j = 0; j < 10; j++){
        bufOut[x] = send_data[i].data[j];
        x++;
      }
    }

    x = write(sfd, bufOut, 100);  //transmission

    if (x != 100) {
          fprintf(stderr, "partial/failed write\n");
          fflush(stdout);
    }

    printf("Send %d bytes\n", x);
    fflush(stdout);

    buf[0] = 0;

    nread = read(sfd, buf, BUF_SIZE); //read of the ACK, if the data receive matches the number of bytes send the transmision is considered a success
    if (nread == -1) {
      printf("Transmission failed\n");
      fflush(stdout); 
    }else{
      printf("Received %zd bytes\n", nread);
      fflush(stdout);
      nread = 0;
     if(buf[0] == x){
          printf("Transmission correct\n");
          fflush(stdout);
      }else{
          printf("Transmission incomplete\n");
          fflush(stdout); 
      }
    }
    }

	//when code reach this part is because we want to close it, we wait to threads to be finish and close the file descriptors

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


  close(sfd);
  close(fd_mpu);
  close(fd_tcs);

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
    /*//code used to know if data is send properly
    float accel_x_data = (mpu_udp[jota].accel_x*9.8)/MPU6000_SCALE_FACTOR;
    float accel_y_data = (mpu_udp[jota].accel_y*9.8)/MPU6000_SCALE_FACTOR;
    float accel_z_data = (mpu_udp[jota].accel_z*9.8)/MPU6000_SCALE_FACTOR;
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
            
      if(jota < 9) //iterator for the array in which we save the data of ten different measures
        jota++;
      else jota = 0;
    // Sleep for 1 seconds
    usleep(MEAS_US);
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
  char raw_color_data[9];

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

  do{
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
    /*//code used to know the status
    printf("Status: %d \n", raw_color_data[0]);    
    fflush(stdout);
    //*/
  }while((raw_color_data[0] & 0x01) == 0);

    // Convert the color data

    tcs_udp[ca].Light = ((raw_color_data[1] << 8) | raw_color_data[2])/256;
    tcs_udp[ca].Red = ((raw_color_data[3] << 8) | raw_color_data[4])/256;
    tcs_udp[ca].Green = ((raw_color_data[5] << 8) | raw_color_data[6])/256;
    tcs_udp[ca].Blue = ((raw_color_data[7] << 8) | raw_color_data[8])/256;

    /*//code used to know if data is send properly
    printf("Light intensity: %d   Red: %d   Green: %d   Blue: %d   ite: %d\n", tcs_udp[ca].Light, tcs_udp[ca].Red, tcs_udp[ca].Green, tcs_udp[ca].Blue, ca);    
    fflush(stdout);
    //*/
    
    if(ca < 9) //iterator for the array in which we save the data of ten different measures
        ca++;
      else ca = 0;

    usleep(MEAS_US);

  }
}


























