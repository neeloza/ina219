#ifndef __I2C_WRAPPER_
#define __I2C_WRAPPER_

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>

int write_i2c_word_data(int fd, unsigned char cmd, uint16_t reg_value);
uint16_t read_i2c_word_data(int fd, unsigned char cmd);
int i2c_dev_open(int fd, char *i2c_bus, int addr);
void i2c_dev_close(int fd);

#endif // __I2C_WRAPPER_
