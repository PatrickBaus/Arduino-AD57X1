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
#ifndef ARDUINO_AD57X1
#define ARDUINO_AD57X1

#include <cstdint>    // uint8_t, etc.
// include the SPI library:
#include <SPI.h>

class AD57X1 {
  public:
    AD57X1(uint8_t cs_pin, SPIClass* _spi, uint8_t VALUE_OFFSET, uint32_t spiClockFrequency, int16_t ldac_pin, bool cs_polarity);
    void setValue(uint32_t value) const;
    [[nodiscard]] uint32_t readValue() const;
    [[nodiscard]] uint32_t readControlRegister() const;
    void enableOutput();
    void setOffsetBinaryEncoding(bool enable);
    void setInternalAmplifier(bool enable);
    void setOutputClamp(bool enable);
    void setTristateMode(bool enable);
    void setReferenceInputRange(bool enable);
    void updateControlRegister() const;
    void setClearCodeValue(uint32_t value) const;
    void reset();
    void begin(bool initSpi=true) const;

  private:
    const uint8_t VALUE_OFFSET;
    SPIClass* const spi;
    
  protected:
    // DAC read/write mode
    static constexpr uint32_t WRITE_REGISTERS = 0UL << 23;
    static constexpr uint32_t READ_REGISTERS = 1UL << 23;

    // DAC value register (p. 21)
    static constexpr uint32_t DAC_REGISTER = 0b001UL << 20;

    // Control register (p. 22)
    static constexpr uint32_t CONTROL_REGISTER = 0b010UL << 20;
    static constexpr uint8_t RBUF_REGISTER = 1;
    static constexpr uint8_t OUTPUT_CLAMP_TO_GND_REGISTER = 2;
    static constexpr uint8_t OUTPUT_TRISTATE_REGISTER = 3;
    static constexpr uint8_t OFFSET_BINARY_REGISTER = 4;
    static constexpr uint8_t SDO_DISABLE_REGISTER = 5;
    static constexpr uint8_t LINEARITY_COMPENSATION_REGISTER = 6;

    static constexpr uint32_t REFERENCE_RANGE_10V = 0b0000;
    static constexpr uint32_t REFERENCE_RANGE_20V = 0b1100;

    // Clearcode register (p. 22)
    static constexpr uint32_t CLEARCODE_REGISTER = 0b011UL << 20;

    // Software control register (p. 23)
    static constexpr uint32_t SW_CONTROL_REGISTER = 0b100UL << 20;
    static constexpr uint8_t SW_CONTROL_LDAC = 0b001;

    void writeSPI(uint32_t value) const;
    [[nodiscard]] uint32_t readSPI(uint32_t value) const;

    const uint8_t PIN_CS;   // The ~Chip select pin used to address the DAC
    const int16_t PIN_LDAC;   // The ~LDAC select pin used to address the DAC
    const bool CS_POLARITY;
    uint32_t controlRegister =
        (1 << RBUF_REGISTER)
      | (1 << OUTPUT_CLAMP_TO_GND_REGISTER)
      | (1 << OUTPUT_TRISTATE_REGISTER)
      | (1 << OFFSET_BINARY_REGISTER)
      | (0 << SDO_DISABLE_REGISTER)
      | (REFERENCE_RANGE_10V << LINEARITY_COMPENSATION_REGISTER);    // This is the default register after reset (see p. 22 of the datasheet)
    SPISettings spi_settings;
};

class AD5781: public AD57X1 {
  public:
    AD5781(const uint8_t cs_pin, SPIClass* spi, const uint32_t spiClockFrequency=1UL*1000*1000, const int16_t ldac_pin=-1, const bool cs_polarity=true) : AD57X1(cs_pin, spi, 2, spiClockFrequency, ldac_pin, cs_polarity) {}
};

class AD5791: public AD57X1 {
  public:
    AD5791(const uint8_t cs_pin, SPIClass* spi, const uint32_t spiClockFrequency=1UL*1000*1000, const int16_t ldac_pin=-1, const bool cs_polarity=true) : AD57X1(cs_pin, spi, 0, spiClockFrequency, ldac_pin, cs_polarity) {}
};
#endif
