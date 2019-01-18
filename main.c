/***********************************************/
/* @file   code.c
 *
 * @brief  Main source file of this project
 */
/***********************************************/

/***********************************************
 *                Header Files
 **********************************************/
//#include "./GeneratedCode/Application.h"
#include <dirent.h>
#include <pthread.h>
#include <math.h>
#include "i2c_wrapper.h"

/***********************************************
 *                Macros
 **********************************************/
#define INA219_I2C_HEATER_ADDRESS   0x40
#define INA219_I2C_PUMP_IN_ADDRESS  0x41
#define INA219_I2C_PUMP_OUT_ADDRESS 0x42
#define INA219_REG_CONFIG           0x00    ///<
#define INA219_REG_SHUNTVOLTAGE     0x01    ///<
#define INA219_REG_BUSVOLTAGE       0x02    ///<
#define INA219_REG_POWER            0x03    ///<
#define INA219_REG_CURRENT          0x04    ///<
#define INA219_REG_CALIBRATION      0x05    ///<
#define INA219_SHUNT_VALUE          0.01
#define INA219_CURRENT_LSB_VALUE    0.000012
#define INA219_CALIBRATION_VALUE    0x8334
#define INA219_CONFIGURATION_VALUE  0x219f
#define NUMBER_OF_GAIN_RANGE        4

/***********************************************
 *          Structure Declaration
 **********************************************/
typedef struct __ina219_info {
    float current_lsb;
    float power_lsb;
    float v_shunt_max;
    float v_bus_max;
    float r_shunt;
} Ina219_info;

/***********************************************
 *             Global Variables
 ***********************************************/
float gain_range[NUMBER_OF_GAIN_RANGE] = {0.04, 0.08, 0.16, 0.32};
char *devName = "/dev/i2c-1";
Ina219_info ina219_info;

/***********************************************
 *          Function Declaration
 **********************************************/
int   ina219_calibrate(int fd, float r_shunt_value);
void  *current_sensor(void *x_void_ptr);
void  check_current_overflow(int fd);
float ina219_get_bus_voltage(int fd);
float ina219_get_shunt_current(int fd);
float ina219_get_bus_power(int fd);
int   ina219_configure(int fd);
int   ina219_start(int *fd, int address);
void  read_sensor_data(int i2c_slave_address);

/***********************************************
 *             Function Defination
 **********************************************/
void *current_sensor(void *x_void_ptr) {
	char buffer[100];

    int heater_voltage, heater_current, heater_power;
    int pump_in_voltage, pump_in_current, pump_in_power;
    int pump_out_voltage, pump_out_current, pump_out_power;

	while (1) {
		/* in bash: python /home/pi/share/RaspberryPi/Application/current.py | sed -e "s/(//" -e "s/)//" -e "s/ //g" -e "s/'//g" | tr -d '.' | tr -d '\n' */

		FILE* file = popen("python /home/pi/oxycooldemo/current.py | sed -e \"s/(//\" -e \"s/)//\" -e \"s/ //g\" -e \"s/\'//g\" | tr -d '.' | tr -d '\n' ", "r");
		if (file) {
			fgets(buffer, 100, file);
			sscanf(buffer, "%d,%d,%d,%d,%d,%d,%d,%d,%d", &heater_voltage, &heater_current, &heater_power, &pump_in_voltage, &pump_in_current, &pump_in_power, &pump_out_voltage, &pump_out_current, &pump_out_power);

			//printf("%d,%d,%d,%d,%d,%d\n", heater_voltage, heater_current,	heater_power, pump_in_voltage, pump_in_current, pump_in_power);
		} else {
			heater_voltage = 0;
			heater_current = 0;
			heater_power = 0;

			pump_in_voltage = 0;
			pump_in_current = 0;
			pump_in_power = 0;

			pump_out_voltage = 0;
			pump_out_current = 0;
			pump_out_power = 0;
		}
		pclose(file);

		usleep(50000);

	}
	return NULL;

}

void check_current_overflow(int fd) {

    uint16_t voltage, config, gain;
    voltage = read_i2c_word_data(fd, INA219_REG_BUSVOLTAGE);

    // Overflow
    if (voltage & 1) {
        config = read_i2c_word_data(fd, INA219_REG_CONFIG);
        gain = (config & 0x1800) >> 11;

       if (gain < (NUMBER_OF_GAIN_RANGE - 1)) {
          gain = gain + 1;

          // Write gain voltage value on calibrate register
          ina219_calibrate(fd, INA219_SHUNT_VALUE);

          // Write gain voltage value on configuration register
          config = config & 0xE7FF;
          write_i2c_word_data(fd, INA219_REG_CONFIG, config | (gain << 11));
          printf("gain set to: %.2fV", gain_range[gain]);
       }
    }
}

float ina219_get_bus_voltage(int fd) {

    uint16_t voltage;
    int16_t value;

    voltage = read_i2c_word_data(fd, INA219_REG_BUSVOLTAGE);
    voltage >>= 3;
    value = ((int16_t) voltage * (int16_t) 4);

    return ((float) value * (float) 0.001);
}

float ina219_get_shunt_current(int fd) {

    int16_t value;

    check_current_overflow(fd);
    value = read_i2c_word_data(fd, INA219_REG_CURRENT);
    return ((float) (value * ina219_info.current_lsb * 1000));
}

float ina219_get_bus_power(int fd) {

    int16_t value;

    check_current_overflow(fd);
    value = read_i2c_word_data(fd, INA219_REG_POWER);
    return ((float)(value * ina219_info.power_lsb * 1000));
}

int ina219_configure(int fd)
{
    // Configuration Value = voltage range << 13 | gain << 11 | bus adc << 7 | shunt adc << 3 | 7
    uint16_t config = INA219_CONFIGURATION_VALUE;

    // Write sensor configuration value
    if (write_i2c_word_data(fd, INA219_REG_CONFIG, config) < 0) {
        return -1;
    }

    return 0;
}

int ina219_calibrate(int fd, float r_shunt_value) {

    uint16_t calibration_value;

    ina219_info.r_shunt     = r_shunt_value;
    // Max Possible Amps = Shunt Volts Max(21) / r_shunt_value
    // current_lsb = Max Possible Amps / Current_LSB_Factor(32800)
    ina219_info.current_lsb = INA219_CURRENT_LSB_VALUE;
    // Power LSB = Current LSB * 20
    ina219_info.power_lsb   = ina219_info.current_lsb * 20;
    // trunc(CALIBRATION_FACTOR(0.04096) / (current_lsb * r_shunt_value))
    calibration_value       = INA219_CALIBRATION_VALUE;

    // Write sensor calibration Value
    if (write_i2c_word_data(fd, INA219_REG_CALIBRATION, calibration_value) < 0) {
        return -1;
    }

    return 0;
}

int ina219_start(int *fd, int address) {

    // Initialize I2C slave device
    if (i2c_dev_open(fd, devName, address) < 0) {
        printf("Fail to initialize INA219 device at address: %x\n", address);
        return -1;
    }

    // Calibration I2C slave device
    if (ina219_calibrate(*fd, INA219_SHUNT_VALUE) < 0) {
        printf("Fail to calibrate INA219 device\n");
        return -1;
    }

    // Configure I2C slave device
    if (ina219_configure(*fd) < 0) {
        printf("Fail to configure INA219 device\n");
        return -1;
    }

    return 0;
}

void read_sensor_data(int i2c_slave_address) {

    unsigned int fd = -1, index = 0;
    float value= 0.0, current = 0.0, voltage = 0.0, power = 0.0;

    // Initialize Sensor
    if (ina219_start(&fd, i2c_slave_address) < 0) {
        return;
    }

    // Read Voltage
    voltage = ina219_get_bus_voltage(fd);

    // Read Current
    for (index=0; index<5; index++) {
        value += ina219_get_shunt_current(fd);
    }
    current = value/(float)5;

    // Read Power
    power = ina219_get_bus_power(fd);

    printf("Address: %d, Voltage: %.3lf, Current: %.3lf, Power: %.3lf\n", i2c_slave_address, voltage, current, power);

    // DeInitialize Sensor
    i2c_dev_close(fd);
}

int main(int argc, char *argv[]) {

    do {

        // Read Heater
        read_sensor_data(INA219_I2C_HEATER_ADDRESS);
        sleep(1);

        // Read Pump In
        read_sensor_data(INA219_I2C_PUMP_IN_ADDRESS);
        sleep(1);

        // Read Pump Out
        read_sensor_data(INA219_I2C_PUMP_OUT_ADDRESS);
        sleep(10);

    } while(1);

    return -1;
}

