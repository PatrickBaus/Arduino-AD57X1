// Minimal host-side stand-in for the Arduino SPI library.
#ifndef MOCK_SPI_H
#define MOCK_SPI_H

#include "Arduino.h"

#define LSBFIRST 0
#define MSBFIRST 1

#define SPI_MODE0 0x00
#define SPI_MODE1 0x04
#define SPI_MODE2 0x08
#define SPI_MODE3 0x0C

class SPISettings {
  public:
    SPISettings(uint32_t _clock, uint8_t _bitOrder, uint8_t _dataMode) : clock(_clock), bitOrder(_bitOrder), dataMode(_dataMode) {}
    uint32_t clock;
    uint8_t bitOrder;
    uint8_t dataMode;
};

class SPIClass {
  public:
    void begin();
    void beginTransaction(SPISettings settings);
    uint8_t transfer(uint8_t data);
    void endTransaction();
};

extern SPIClass SPI;

#endif
