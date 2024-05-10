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

int TCS(void);

typedef struct{
    __uint16_t Red;
    __uint16_t Green;
    __uint16_t Blue;
    __uint16_t Light;
}t_color_data;