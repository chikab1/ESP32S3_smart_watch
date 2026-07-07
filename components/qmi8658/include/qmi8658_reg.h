#ifndef __QMI8658_REG_H__
#define __QMI8658_REG_H__

#define QMI8658_I2C_ADDR            0x6B
#define QMI8658_WHO_AM_I_VAL        0x05

#define QMI8658_WHO_AM_I            0x00
#define QMI8658_CTRL1               0x02

#define QMI8658_CTRL1_GYR_BWP_POS   6
#define QMI8658_CTRL1_ACC_BWP_POS   4
#define QMI8658_CTRL2               0x03
#define QMI8658_CTRL3               0x04
#define QMI8658_CTRL7               0x08

#define QMI8658_AX_L                0x35
#define QMI8658_AY_L                0x37
#define QMI8658_AZ_L                0x39
#define QMI8658_GX_L                0x3B
#define QMI8658_GY_L                0x3D
#define QMI8658_GZ_L                0x3F

#define QMI8658_CTRL2_ACC_RANGE_POS 5
#define QMI8658_CTRL2_ACC_ODR_POS   2

#define QMI8658_CTRL3_GYRO_RANGE_POS 5
#define QMI8658_CTRL3_GYRO_ODR_POS   2

#define QMI8658_CTRL7_ACC_EN_BIT    0
#define QMI8658_CTRL7_GYRO_EN_BIT   1

#define QMI8658_ACC_RANGE_2G        0
#define QMI8658_ACC_RANGE_4G        1
#define QMI8658_ACC_RANGE_8G        2
#define QMI8658_ACC_RANGE_16G       3

#define QMI8658_GYRO_RANGE_16DPS    0
#define QMI8658_GYRO_RANGE_32DPS    1
#define QMI8658_GYRO_RANGE_64DPS    2
#define QMI8658_GYRO_RANGE_128DPS   3
#define QMI8658_GYRO_RANGE_256DPS   4
#define QMI8658_GYRO_RANGE_512DPS   5
#define QMI8658_GYRO_RANGE_1024DPS  6

#define QMI8658_ACC_ODR_8000HZ      0
#define QMI8658_ACC_ODR_4000HZ      1
#define QMI8658_ACC_ODR_2000HZ      2
#define QMI8658_ACC_ODR_1000HZ      3
#define QMI8658_ACC_ODR_500HZ       4
#define QMI8658_ACC_ODR_250HZ       5
#define QMI8658_ACC_ODR_125HZ       6
#define QMI8658_ACC_ODR_62_5HZ      7

#define QMI8658_GYRO_ODR_8000HZ     0
#define QMI8658_GYRO_ODR_4000HZ     1
#define QMI8658_GYRO_ODR_2000HZ     2
#define QMI8658_GYRO_ODR_1000HZ     3
#define QMI8658_GYRO_ODR_500HZ      4
#define QMI8658_GYRO_ODR_250HZ      5
#define QMI8658_GYRO_ODR_125HZ      6
#define QMI8658_GYRO_ODR_62_5HZ     7

#endif