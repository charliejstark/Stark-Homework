#ifndef SPI__H__
#define SPI__H__

void initSPI();
unsigned char spi_io(unsigned char o);

void spiWrite(unsigned char c, unsigned short v);



#endif // SPI__H__