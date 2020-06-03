#ifndef I2C_MASTER_NOINT_H__
#define I2C_MASTER_NOINT_H__
// Header file for i2c_master_noint.c
// helps implement use I2C1 as a master without using interrupts

#include <xc.h>

#define I2C_PIN_ADDR    0b01000000

void i2c_master_setup(void); // set up I2C1 as master
void i2c_master_start(void); // send a START signal
void i2c_master_restart(void); // send a RESTART signal
void i2c_master_send(unsigned char byte); // send a byte (either an address or data)
unsigned char i2c_master_recv(void); // receive a byte of data
void i2c_master_ack(int val); // send an ACK (0) or NACK (1)
void i2c_master_stop(void); // send a stop

// My functions
void setPin(unsigned char addW, unsigned char regW, unsigned char val); // Set the value of a pin
unsigned char readPin(unsigned char addW, unsigned char regR);          // Read the value of a pin

void i2c_master_read_multiple(unsigned char address, unsigned char reg, unsigned char * data, int length);


#endif