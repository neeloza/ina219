#ifndef __I2C_WRAPPER_
#define __I2C_WRAPPER_

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>

int access_i2c_device(char flag_rw, unsigned char cmd,\
                                    int data_size, union i2c_smbus_data *i2c_data);
int write_i2c_word_data(unsigned char cmd, uint16_t reg_value);
int read_i2c_word_data(unsigned char cmd);
int i2c_dev_open(char *i2c_bus, int addr);
void i2c_dev_close(void);

#endif // __I2C_WRAPPER_
