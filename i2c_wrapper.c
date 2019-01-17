#include "common_i2c.h"

int write_i2c_word_data(int fd, unsigned char cmd, uint16_t reg_value) {

    uint16_t length = 3;
    unsigned char buffer[8] = {0};

    buffer[0] = cmd;
    buffer[1] = (reg_value & 0xFF00) >> 8;
    buffer[2] = (reg_value & 0x00FF);

    // Write slave device register and value
    if (write(fd, buffer, length) != length) {
        printf("Failed to write data to the i2c bus.\n");
        return -1;
    }
    return 0;
}

uint16_t read_i2c_word_data(int fd, unsigned char cmd) {

    uint16_t length = 0, value = 0;
    unsigned char buffer[8] = {0};

    buffer[0] = cmd;
    length = 1;
    // Write slave device register
    if (write(fd, buffer, length) != length) {
        printf("Failed to write to the i2c bus.\n");
        return 0;
    }

    usleep(800);

    length = 2;
    // Read slave fevice register value
    if (read(fd, buffer, length) != length) {
        printf("Failed to read from the i2c bus.\n");
    }
    else {
        value = (((uint16_t) buffer[0]) << 8) | ((uint16_t) buffer[1]);
    }
    return value;
}

int i2c_dev_open(int fd, char *i2c_bus, int addr) {

    // Open I2C bus
    fd = open(i2c_bus, O_RDWR);
    if (fd < 0) {
        printf("Failed to open i2c device, I2C Bus : %s Device Address: %x\n", i2c_bus, addr);
        return -1;
    }

    // Set working device
    if (ioctl(fd, I2C_SLAVE, addr) < 0) {
        printf("Failed to set i2c device as slave, I2C Bus : %s Device Address: %x\n", i2c_bus, addr);
        return -1;
    }

    return 0;
}

void i2c_dev_close(int fd) {

    // Close I2C bus
    if (-1 != fd) {
        close(fd);
    }
    fd = -1;
}
