#include <stdio.h>
#include <stdlib.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <linux/i2c.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>

#define TCS34725_I2C_ADDR 0x29
#define TCS34725_ENABLE_REG 0x03
#define TCS34725_CMD_REG 0x80
#define TCS34725_CMD_REG_RD 0xA0
#define TCS34725_COLOR_OUT 0x14

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
    fflush(stdout);
    return 1;
  }


  if(pthread_join(t1, NULL) != 0){
    printf("Error joining thread\n");
    fflush(stdout);
    return 1;
  }

  printf("ANCELAO \n");

  close(fd);

  return 0;
}


void medir(){

  struct i2c_rdwr_ioctl_data packets;
  struct i2c_msg messages[2];
  char i2cFile[15];
  int device = 1;
  int addr = TCS34725_I2C_ADDR;

  printf("Hello and welcome to the color sensor program\n");
  fflush(stdout);
  sleep(1);
  sprintf(i2cFile, "/dev/i2c-%d", device);
  fd = open(i2cFile,O_RDWR);

  if(!ioctl(fd,I2C_SLAVE,addr)){
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

  if(!ioctl(fd, I2C_RDWR, &packets)){
    printf("Error in I2C\n");
    fflush(stdout);
  }

  char reg = TCS34725_CMD_REG_RD + TCS34725_COLOR_OUT;
  char color_data[8];

  while(33){
    // Read color sensor data

    messages[0].addr = addr;
    messages[0].flags = 0;
    messages[0].len = sizeof(reg);
    messages[0].buf = &reg;

    messages[1].addr = addr;
    messages[1].flags = I2C_M_RD;
    messages[1].len = sizeof(color_data);
    messages[1].buf = color_data;

    packets.msgs = messages;
    packets.nmsgs = 2;

    if(!ioctl(fd, I2C_RDWR, &packets)){
      printf("Error in I2C\n");
      messages[0].addr = addr;
      messages[0].flags = 0;
      messages[0].len = sizeof(data);
      messages[0].buf = data;

      packets.msgs = messages;
      packets.nmsgs = 1;

      if(!ioctl(fd, I2C_RDWR, &packets)){
        printf("I2C disconnected\n");
        fflush(stdout);
      }
    }

    // Convert the color data

    __uint16_t Light = ((color_data[0] << 8) | color_data[1])/256;
    __uint16_t Red = ((color_data[2] << 8) | color_data[3])/256;
    __uint16_t Green = ((color_data[4] << 8) | color_data[5])/256;
    __uint16_t Blue = ((color_data[6] << 8) | color_data[7])/256;

    // Print the color data

    system("clear");
    printf("Light intensity: %d   Red: %d   Green: %d   Blue: %d\n", Light, Red, Green, Blue);
    printf("Color: ");
    if(Red > 200 && Green > 200 && Blue > 200){
      printf("White\n");
    }else if(Red < 50 && Green < 50 && Blue < 50){
      printf("Black\n");
    }else if(Red < 100 && Green > 175 && Blue > 175){
      printf("Cyan\n");
    }else if(Red > 175 && Green > 175 && Blue < 100){
      printf("Yellow\n");
    }else if(Red > 175 && Green < 100 && Blue > 150){
      printf("Fuchsia\n");
    }else if(Red < 100 && Green < 100 && Blue > 175){
      printf("Blue\n");
    }else if(Red < 100 && Green > 175 && Blue < 100){
      printf("Green\n");
    }else if(Red > 175 && Green < 100 && Blue < 100){
      printf("Red\n");
    }else{
      printf("Gray\n");
    }


    fflush(stdout);

    sleep(1);
  }
}
