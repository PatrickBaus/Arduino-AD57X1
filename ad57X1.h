/*    
 *  The AD5781 library is free software: you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License as 
 *  published by the Free Software Foundation, either version 3 of the 
 *  License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.

 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef __ARDUINO_AD5762R
#define __ARDUINO_AD5762R

// #define ARDUINO_AD5762R_DEBUG

#include <Arduino.h>
// include the SPI library:
#include <SPI.h>

class AD5781 {
  public:
    AD5781(uint8_t cs_pin, uint8_t ldac_pin);
    AD5781(uint8_t cs_pin);
    void setValue(uint32_t value);
    void enableOutput();
    void setOffsetBinaryEncoding(bool enable);
    void setInternalAmplifier(bool enable);
    void setOutputClamp(bool enable);
    void setTristateMode(bool enable);
    void setReferenceInputRange(bool enable);
    void updateControlRegister();
    void setClearCodeValue(uint32_t value);
    void reset();
    void begin();

  protected:
    // DAC read/write mode
    static const uint32_t WRITE_REGISTERS = 0 << 23;
    static const uint32_t READ_REGISTERS = 1 << 23;
    // The AD5781 has got a 20-bit brethren (AD5791), with which it shares its interface, so we need to shift
    // the 18 bit value 2 bits to the left.
    static const uint32_t DAC_REGSISTER_VALUE_OFFSET = 2;

    // DAC value register (p. 21)
    static const uint32_t DAC_REGISTER = 0b001 << 20;

    // Control register (p. 22)
    static const uint32_t CONTROL_REGISTER = 0b010 << 20;
    static const uint8_t RBUF_REGISTER = 1;
    static const uint8_t OUTPUT_CLAMP_TO_GND_REGISTER = 2;
    static const uint8_t OUTPUT_TRISTATE_REGISTER = 3;
    static const uint8_t OFFSET_BINARY_REGISTER = 4;
    static const uint8_t SDO_DISABLE_REGISTER = 5;
    static const uint8_t LINEARITY_COMPENSATION_REGISTER = 6;

    static const uint32_t REFERENCE_RANGE_10V = 0b0000;
    static const uint32_t REFERENCE_RANGE_20V = 0b1100;

    // Clearcode register (p. 22)
    static const uint32_t CLEARCODE_REGISTER = 0b011 <<20;

    // Software control register (p. 23)
    static const uint32_t SW_CONTROL_REGISTER = 0b100 << 20;
    static const uint8_t SW_CONTROL_LDAC = 0b001;

    void writeSPI(uint32_t value);

    uint8_t cs_pin;   // The ~Chip select pin used to address the DAC
    int16_t ldac_pin;   // The ~LDAC select pin used to address the DAC
    uint32_t controlRegister =
        (1 << RBUF_REGISTER)
      | (1 << OUTPUT_CLAMP_TO_GND_REGISTER)
      | (1 << OUTPUT_TRISTATE_REGISTER)
      | (1 << OFFSET_BINARY_REGISTER)
      | (0 << SDO_DISABLE_REGISTER)
      | (REFERENCE_RANGE_10V << LINEARITY_COMPENSATION_REGISTER);    // This is the default register after reset (see p. 22 of the datasheet)
    SPISettings spi_settings;
};

#endif
