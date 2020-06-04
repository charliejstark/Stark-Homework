#include "imu.h"
#include "i2c_master_noint.h"

void imu_setup() {
    unsigned char who = 0;     
    
    who = readPin(IMU_ADDR, IMU_WHOAMI);
    
    if (who != 0b01101001) {
        while(1){}
    }

    setPin(IMU_ADDR, IMU_CTRL1_XL, 0b10000010);

    setPin(IMU_ADDR, IMU_CTRL2_G, 0b10001000);

    setPin(IMU_ADDR, IMU_CTRL3_C, 0b00000100);

}

void imu_read(unsigned char address, unsigned char reg, signed short * dataShort, int len) {
    // Order of elements in dataShort
    // 0-bit: Temperature
    // 1-bit: Gyroscopic x
    // 2-bit: Gyroscopic y
    // 3-bit: Gyroscopic z
    // 4-bit: Acceleration x
    // 5-bit: Acceleration y
    // 6-bit: Acceleration z
        
    int length = len*2;
    unsigned char dataChar[length];
    
    i2c_master_read_multiple(address, reg, dataChar, length);
    
    int jj;
    for (jj = 0; jj < len; jj++) {
        *(dataShort + jj) = (dataChar[2*jj]) | ((dataChar[2*jj + 1])<<8);   
    }
}