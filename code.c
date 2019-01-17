//#include "./GeneratedCode/Application.h"
#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <math.h>
#include "common_i2c.h"

#define INA219_I2C_DEFAULT_SLAVE_ADDRESS    0x40
#define INA219_REG_CONFIG           0x00    ///<
#define INA219_REG_SHUNTVOLTAGE     0x01    ///<
#define INA219_REG_BUSVOLTAGE       0x02    ///<
#define INA219_REG_POWER            0x03    ///<
#define INA219_REG_CURRENT          0x04    ///<
#define INA219_REG_CALIBRATION      0x05    ///<

#define CEILING_POS(X) ((X-(int)(X)) > 0 ? (int)(X+1) : (int)(X))

typedef enum ina219_range {
    INA219_RANGE_16V = 0x0000,  ///< 0-16V Range
    INA219_RANGE_32V = 0x2000,  ///< 0-32V Range
} ina219_range_t;

typedef enum ina219_gain {
    INA219_GAIN_40MV  = 0x0000, ///< Gain 1, 40mV Range
    INA219_GAIN_80MV  = 0x0800, ///< Gain 2, 80mV Range
    INA219_GAIN_160MV = 0x1000, ///< Gain 4, 160mV Range
    INA219_GAIN_320MV = 0x1800  ///< Gain 8, 320mV Range
} ina219_gain_t;

typedef enum ina219_bus_res {
    INA219_BUS_RES_9BIT  = 0x0080,  ///< 9-bit bus res = 0..511
    INA219_BUS_RES_10BIT = 0x0100,  ///< 10-bit bus res = 0..1023
    INA219_BUS_RES_11BIT = 0x0200,  ///< 11-bit bus res = 0..2047
    INA219_BUS_RES_12BIT = 0x0400   ///< 12-bit bus res = 0..4097
} ina219_bus_res_t;

typedef enum ina219_shunt_res {
    INA219_SHUNT_RES_9BIT_1S    = 0x0000,   ///< 1 x 9-bit shunt sample
    INA219_SHUNT_RES_10BIT_1S   = 0x0008,   ///< 1 x 10-bit shunt sample
    INA219_SHUNT_RES_11BIT_1S   = 0x0010,   ///< 1 x 11-bit shunt sample
    INA219_SHUNT_RES_12BIT_1S   = 0x0018,   ///< 1 x 12-bit shunt sample
    INA219_SHUNT_RES_12BIT_2S   = 0x0048,   ///< 2 x 12-bit shunt samples averaged together
    INA219_SHUNT_RES_12BIT_4S   = 0x0050,   ///< 4 x 12-bit shunt samples averaged together
    INA219_SHUNT_RES_12BIT_8S   = 0x0058,   ///< 8 x 12-bit shunt samples averaged together
    INA219_SHUNT_RES_12BIT_16S  = 0x0060,   ///< 16 x 12-bit shunt samples averaged together
    INA219_SHUNT_RES_12BIT_32S  = 0x0068,   ///< 32 x 12-bit shunt samples averaged together
    INA219_SHUNT_RES_12BIT_64S  = 0x0070,   ///< 64 x 12-bit shunt samples averaged together
    INA219_SHUNT_RES_12BIT_128S = 0x0078    ///< 128 x 12-bit shunt samples averaged together
} ina219_shunt_res_t;

typedef enum ina219_mode {
    INA219_MODE_POWER_DOWN     = 0x0000,    ///<
    INA219_MODE_SHUNT_TRIG     = 0x0001,    ///<
    INA219_MODE_BUS_TRIG       = 0x0002,    ///<
    INA219_MODE_SHUNT_BUS_TRIG = 0x0003,    ///<
    INA219_MODE_ADC_OFF        = 0x0004,    ///<
    INA219_MODE_SHUNT_CONT     = 0x0005,    ///<
    INA219_MODE_BUS_CONT       = 0x0006,    ///<
    INA219_MODE_SHUNT_BUS_CONT = 0x0007     ///<
} ina219_mode_t;

typedef struct __ina219_info {
    float current_lsb;
    float power_lsb;
    float v_shunt_max;
    float v_bus_max;
    float r_shunt;
} Ina219_info;

Ina219_info ina219_info;
char *devName = "/dev/i2c-1";
int fd;

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

void current_overflow(void) {

    uint16_t voltage, config, gain;
    voltage = read_i2c_word_data(INA219_REG_BUSVOLTAGE);

    // Overflow
    if (voltage & 1) {
        config = read_i2c_word_data(INA219_REG_CONFIG);
        gain = (config & 0x1800) >> 11;

        if (gain < 3) {
            gain = gain + 1;
        }
    }
}

float ina219_get_bus_voltage(void) {

    uint16_t voltage;
    int16_t value;

    voltage = read_i2c_word_data(INA219_REG_BUSVOLTAGE);
    voltage >>= 3;
    value = ((int16_t) voltage * (int16_t) 4);

    return ((float) value * (float) 0.001);
}

float ina219_get_shunt_current(void) {

    int16_t value;
    value = read_i2c_word_data(INA219_REG_CURRENT);
#if 0
    return ((float) (value * ina219_info.current_lsb));
#else
    return ((float) (value * ina219_info.current_lsb * 1000));
#endif
}

float ina219_get_bus_power(void) {

    int16_t value;
    value = read_i2c_word_data(INA219_REG_POWER);
#if 0
    return ((float)(value * ina219_info.power_lsb));
#else
    return ((float)(value * ina219_info.power_lsb * 1000));
#endif
}

int ina219_configure(ina219_range_t range, ina219_gain_t gain, ina219_bus_res_t bus_res,\
        ina219_shunt_res_t shunt_res, ina219_mode_t mode)
{
    uint16_t config = range | gain| bus_res | shunt_res | mode;

    switch (range) {
        case INA219_RANGE_32V:
            ina219_info.v_bus_max = (float) 32.0;
            break;
        case INA219_RANGE_16V:
            ina219_info.v_bus_max = (float) 16.0;
            break;
    }

    switch(gain)
    {
        case INA219_GAIN_320MV:
            ina219_info.v_shunt_max = (float) 0.32;
            break;
        case INA219_GAIN_160MV:
            ina219_info.v_shunt_max = (float) 0.16;
            break;
        case INA219_GAIN_80MV:
            ina219_info.v_shunt_max = (float) 0.08;
            break;
        case INA219_GAIN_40MV:
            ina219_info.v_shunt_max = (float) 0.04;
            break;
    }

#if 1
    config = 0x219f;
#endif

    //printf("Configuration: %#x\n", config);

#if 1
    if (write_i2c_word_data(INA219_REG_CONFIG, config) < 0) {
        return -1;
    }
#endif
    int16_t value;
    value = read_i2c_word_data(INA219_REG_CONFIG);
    //printf("Return Configuration: %#x\n", value);
    return 0;
}

int  ina219_calibrate(float r_shunt_value, float i_max_expected) {
    //const float minimum_lsb = i_max_expected / 32767;

    uint16_t calibration_value;

    ina219_info.r_shunt = r_shunt_value;

#if 0
    ina219_info.current_lsb = (float) ((uint16_t) (minimum_lsb * 100000000));
    ina219_info.current_lsb /= 100000000;
    ina219_info.current_lsb /= 0.0001;
    ina219_info.current_lsb = (float) CEILING_POS(ina219_info.current_lsb);
    ina219_info.current_lsb *= 0.0001;
#else
    ina219_info.current_lsb = 0.000012;
#endif

    printf("current_lsb: %f\n", ina219_info.current_lsb);

    ina219_info.power_lsb = ina219_info.current_lsb * 20;
    printf("power_lsb: %f\n", ina219_info.power_lsb);

#if 0
    calibration_value = (uint16_t) ((0.04096) / (ina219_info.current_lsb * ina219_info.r_shunt));
#else
    calibration_value = 0x8334;
#endif
    //printf("calibration_value: %#x, %d\n", calibration_value, calibration_value);

#if 1
    if (write_i2c_word_data(INA219_REG_CALIBRATION, calibration_value) < 0) {
        return -1;
    }
#endif

    uint16_t value;
    value = read_i2c_word_data(INA219_REG_CALIBRATION);
    //printf("Return calibration_value: %#x %d\n", value, value);

    return 0;
}

int ina219_start(void) {

    if (i2c_dev_open(devName, INA219_I2C_DEFAULT_SLAVE_ADDRESS) < 0) {
        printf("Fail to initialize INA219 device at address: %x\n", INA219_I2C_DEFAULT_SLAVE_ADDRESS);
        return -1;
    }

    if (ina219_calibrate(0.1, 2) < 0) {
        printf("Fail to calibrate INA219 device\n");
        return -1;
    }

    if (ina219_configure(INA219_RANGE_32V, INA219_GAIN_320MV,\
                    INA219_BUS_RES_12BIT, INA219_SHUNT_RES_12BIT_1S , INA219_MODE_SHUNT_BUS_CONT) < 0) {
        printf("Fail to configure INA219 device\n");
        return -1;
    }


    return 0;
}


int main() {

    unsigned int i=0;
    float value = 0.0;

    if (ina219_start() < 0) {
        return -1;
    }

   // return 0;

    do {
        // Read Voltage
        value = ina219_get_bus_voltage();
        printf("Voltage: %.3f\n", value);
        value = 0.0;

        // Read Current
        for (i=0; i<5; i++) {
            value += ina219_get_shunt_current();
        }
        printf("Current: %.3f\n", value/(float)5);
        value = 0.0;

        // Read Current
        value = ina219_get_bus_power();
        printf("Power: %.3f\n", value);
        value = 0.0;

        //sleep(5);

    } while(0);

    i2c_dev_close();

    return -1;
}

