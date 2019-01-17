#include "common_i2c.h"

extern int fd;

int access_i2c_device(char flag_rw, unsigned char cmd,\
                            int data_size, union i2c_smbus_data *i2c_data) {

    struct i2c_smbus_ioctl_data ioctl_data;
    int status = 0;

    ioctl_data.read_write   = flag_rw;
    ioctl_data.command      = cmd;
    ioctl_data.size         = data_size;
    ioctl_data.data         = i2c_data;

    status = ioctl(fd, I2C_SMBUS, &ioctl_data);
    if (status < 0) {
        printf("Failed to access I2C device status: %d\n", status);
    }

    return status;
}

int write_i2c_word_data(unsigned char cmd, uint16_t reg_value) {
#if 0
    union i2c_smbus_data i2c_data;
    int status = 0;

    i2c_data.word = reg_value;
    status = access_i2c_device(I2C_SMBUS_WRITE, cmd, I2C_SMBUS_WORD_DATA, &i2c_data);
    if (status < 0) {
        printf("Failed to write word data on I2C status: %d\n", status);
    }
 #else
	uint16_t length = 0;
	unsigned char buffer[8] = {0};
	buffer[0] = cmd;
	buffer[1] = (reg_value & 0xFF00) >> 8;
	buffer[2] = (reg_value & 0x00FF);
	//----- WRITE BYTES -----
	length = 3;			//<<< Number of bytes to write
	if (write(fd, buffer, length) != length) {
		/* ERROR HANDLING: i2c transaction failed */
		printf("Failed to write to the i2c bus.\n");
	}
#endif

    return 0;
}

uint16_t read_i2c_word_data(unsigned char cmd) {

#if 0
    union i2c_smbus_data i2c_data;
    int status = 0;
    unsigned char lsb, msb; 

    status = access_i2c_device(I2C_SMBUS_WRITE, cmd, I2C_SMBUS_BYTE, NULL);
    if (status < 0) {
	printf("Failed to write byte data on I2C status: %d\n", status);
        return -1;
    } 

    status = access_i2c_device(I2C_SMBUS_READ, 0, I2C_SMBUS_BYTE, &i2c_data);
    if (status < 0) {
        printf("Failed to read word data from I2C status: %d\n", status);
        return -1;
    } else {
        lsb = i2c_data.byte;
    }

    status = access_i2c_device(I2C_SMBUS_READ, 0, I2C_SMBUS_BYTE, &i2c_data);
    if (status < 0) {
        printf("Failed to read word data from I2C status: %d\n", status);
        return -1;
    } else {
        msb = i2c_data.byte;
    }
    

    printf("LSB: %d\n", lsb);
    printf("MSB: %d\n", msb);

    return ((((uint16_t) msb) << 8) | ((uint16_t) lsb));
}
#else
	uint16_t length = 0, value = 0;
	unsigned char buffer[8] = {0};
	buffer[0] = cmd;
	//----- WRITE BYTES -----
	length = 1;			//<<< Number of bytes to write
	if (write(fd, buffer, length) != length) {
		/* ERROR HANDLING: i2c transaction failed */
		printf("Failed to write to the i2c bus.\n");
	}

	usleep(800);

	length = 2;
	if (read(fd, buffer, length) != length) {
		printf("Failed to read from the i2c bus.\n");
	}
	else {
		value = (((uint16_t) buffer[0]) << 8) | ((uint16_t) buffer[1]);
		//printf("Data read: %d %d %d %x\n", buffer[1], buffer[0], value, value);
	}
	return value;
}
#endif

int i2c_dev_open(char *i2c_bus, int addr)
{
    int status = 0;

    fd = open(i2c_bus, O_RDWR);
    if (fd < 0) {
        printf("Failed to open i2c device, I2C Bus : %s Device Address: %x\n", i2c_bus, addr);
        return -1;
    }

    // set working device
    if (ioctl(fd, I2C_SLAVE, addr) < 0) {
        printf("Failed to set i2c device as slave, I2C Bus : %s Device Address: %x\n", i2c_bus, addr);
        return -1;
    }

    return status;
}

void i2c_dev_close(void)
{
    if (-1 != fd) {
        close(fd);
    }
    fd = -1;
}

