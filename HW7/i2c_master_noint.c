// I2C Master utilities, using polling rather than interrupts
// The functions must be called in the correct order as per the I2C protocol
// I2C pins need pull-up resistors, 2k-10k
#include "i2c_master_noint.h"

void i2c_master_setup(void) {
    // using a large BRG to see it on the nScope, make it smaller after verifying that code works
    // look up TPGD in the datasheet
    I2C1BRG = 50; // I2CBRG = [1/(2*Fsck) - TPGD]*Pblck - 2 (TPGD is the Pulse Gobbler Delay)
    I2C1CONbits.ON = 1; // turn on the I2C1 module
}

void i2c_master_start(void) {
    I2C1CONbits.SEN = 1; // send the start bit
    while (I2C1CONbits.SEN) {
        ;
    } // wait for the start bit to be sent
}

void i2c_master_restart(void) {
    I2C1CONbits.RSEN = 1; // send a restart 
    while (I2C1CONbits.RSEN) {
        ;
    } // wait for the restart to clear
}

void i2c_master_send(unsigned char byte) { // send a byte to slave
    I2C1TRN = byte; // if an address, bit 0 = 0 for write, 1 for read
    while (I2C1STATbits.TRSTAT) {
        ;
    } // wait for the transmission to finish
    if (I2C1STATbits.ACKSTAT) { // if this is high, slave has not acknowledged
        // ("I2C1 Master: failed to receive ACK\r\n");
        while(1){} // get stuck here if the chip does not ACK back
    }
}

unsigned char i2c_master_recv(void) { // receive a byte from the slave
    I2C1CONbits.RCEN = 1; // start receiving data
    while (!I2C1STATbits.RBF) {
        ;
    } // wait to receive the data
    return I2C1RCV; // read and return the data
}

void i2c_master_ack(int val) { // sends ACK = 0 (slave should send another byte)
    // or NACK = 1 (no more bytes requested from slave)
    I2C1CONbits.ACKDT = val; // store ACK/NACK in ACKDT
    I2C1CONbits.ACKEN = 1; // send ACKDT
    while (I2C1CONbits.ACKEN) {
        ;
    } // wait for ACK/NACK to be sent
}

void i2c_master_stop(void) { // send a STOP:
    I2C1CONbits.PEN = 1; // comm is complete and master relinquishes bus
    while (I2C1CONbits.PEN) {
        ;
    } // wait for STOP to complete
}

void setPin(unsigned char addW, unsigned char regW, unsigned char val) {
    i2c_master_start();                 // Start bit
    
    i2c_master_send(addW);              // Add - Write
    i2c_master_send(regW);              // Send data - Register
    i2c_master_send(val);               // Send data - Value
    
    i2c_master_stop();                  // Stop bit
}

unsigned char readPin(unsigned char addW, unsigned char regR) {
    unsigned char addR = addW | 1;    // Bit signifies reading
    
    i2c_master_start();                 // Start bit
    
    i2c_master_send(addW);              // Add - Write
    i2c_master_send(regR);              // Send data - Register
    i2c_master_restart();               // Restart bit
    i2c_master_send(addR);              // Add - Read
    unsigned char val = i2c_master_recv();        // Receive value
    i2c_master_ack(1);                  // Acknowledge value
    i2c_master_stop();                  // Stop bit
    return val;                         // Return pin value
}

void i2c_master_read_multiple(unsigned char address, unsigned char reg, unsigned char * data, int length) {
    // Order of bites:
    // 1: temperature low 20h
    // 2: temperature high 21h
    // 3: gyroscope x low 22h
    // 4: gyroscope x high 23h
    // 5: gyroscope y low 24h
    // 6: gyroscope y high 25h
    // 7: gyroscope z low 26h
    // 8: gyroscope z high 27h
    // 9: acceleration x low 28h
    // 10: acceleration x high 29h
    // 11: acceleration y low 2Ah
    // 12: acceleration y high 2Bh
    // 13: acceleration z low 2Ch
    // 14: acceleration z high 2Dh
    
    i2c_master_start();                 // (1) Start bit
    i2c_master_send(address);           // (2) Write bit
    i2c_master_send(reg);               // (3) Register we want to read from
    i2c_master_restart();               // (4) Restart bit
    i2c_master_send(address | 0x01);    // (5) Read bit
    
    int ii;                             // for loop
    for (ii = 0; ii < length; ii++) {   // (6 through 6+length) Receive value
        *(data + ii) = i2c_master_recv();
        if ((ii < (length - 1)) == 1) {
            i2c_master_ack(0);          // Acknowledge bit
        } else {
            i2c_master_ack(1);          // Acknowledge bit
        }
    }
    
    i2c_master_stop();                  // (7+length) Stop bit
}