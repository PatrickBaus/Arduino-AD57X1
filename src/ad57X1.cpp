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
#include "ad57X1.h"

// Use CPOL = 0,  CPHA = 1 for ADI DACs
AD57X1::AD57X1(uint8_t _cs_pin, SPIClass* const _spi, const uint8_t _value_offset, const uint32_t spiClockFrequency, int16_t _ldac_pin, const bool cs_polarity) :
  VALUE_OFFSET(_value_offset),
  spi(_spi), PIN_CS(_cs_pin),
  PIN_LDAC(_ldac_pin),
  CS_POLARITY(cs_polarity),
  spi_settings(SPISettings(spiClockFrequency, MSBFIRST, SPI_MODE1)) {
}

void AD57X1::writeSPI(const uint32_t value) const {
  digitalWrite(this->PIN_CS, this->CS_POLARITY);

  this->spi->beginTransaction(this->spi_settings);
  this->spi->transfer((value >> 16) & 0xFF);
  this->spi->transfer((value >> 8) & 0xFF);
  this->spi->transfer((value >> 0) & 0xFF);
  this->spi->endTransaction();

  digitalWrite(this->PIN_CS, !this->CS_POLARITY);
}

uint32_t AD57X1::readSPI(const uint32_t value) const {
  this->writeSPI(value);
  digitalWrite(this->PIN_CS, this->CS_POLARITY);

  this->spi->beginTransaction(this->spi_settings);
  uint32_t result = static_cast<uint32_t>(this->spi->transfer(0x00)) << 16;
  result |= static_cast<uint32_t>(this->spi->transfer(0x00)) << 8;
  result |= static_cast<uint32_t>(this->spi->transfer(0x00));
  this->spi->endTransaction();

  digitalWrite(this->PIN_CS, !this->CS_POLARITY);
  return result;
}

void AD57X1::updateControlRegister() const {
  this->writeSPI(AD57X1::WRITE_REGISTERS | AD57X1::CONTROL_REGISTER | AD57X1::controlRegister);
}

void AD57X1::setClearCodeValue(const uint32_t value) const {
    constexpr uint32_t command = AD57X1::WRITE_REGISTERS | AD57X1::CLEARCODE_REGISTER | ((value << this->VALUE_OFFSET) & 0xFFFFF);

    this->writeSPI(command);
}

void AD57X1::reset() {
  this->enableOutput();
}

// value is an 18 or 20 bit value
void AD57X1::setValue(const uint32_t value) const {
  constexpr uint32_t command = AD57X1::WRITE_REGISTERS | AD57X1::DAC_REGISTER | ((value << this->VALUE_OFFSET) & 0xFFFFF);

  this->writeSPI(command);

  if (this->PIN_LDAC >= 0) {
    digitalWrite(this->PIN_LDAC, HIGH);
    digitalWrite(this->PIN_LDAC, LOW);
  } else {
    this->writeSPI(AD57X1::SW_CONTROL_REGISTER | AD57X1::SW_CONTROL_LDAC);
  }
}

uint32_t AD57X1::readValue() const {
  constexpr uint32_t command = AD57X1::READ_REGISTERS | AD57X1::DAC_REGISTER;
  const uint32_t result = this->readSPI(command) >> this->VALUE_OFFSET;
  return result;
}

void AD57X1::enableOutput() {
  this->setOutputClamp(false);
  this->setTristateMode(false);
  this->updateControlRegister();
}

void AD57X1::setInternalAmplifier(const bool enable) {
  // (1 << this->RBUF_REGISTER) : internal amplifier is disabled (default)
  // (0 << this->RBUF_REGISTER) : internal amplifier is enabled
  this->controlRegister = (this->controlRegister & ~(1 << AD57X1::RBUF_REGISTER)) | (!enable << AD57X1::RBUF_REGISTER);
}

// Setting this to enabled will overrule the tristate mode and clamp the output to GND
void AD57X1::setOutputClamp(const bool enable) {
  // (1 << this->OUTPUT_CLAMP_TO_GND_REGISTER) : the output is clamped to GND (default)
  // (0 << this->OUTPUT_CLAMP_TO_GND_REGISTER) : the dac is in normal mode
  this->controlRegister = (this->controlRegister & ~(1 << AD57X1::OUTPUT_CLAMP_TO_GND_REGISTER)) | (enable << AD57X1::OUTPUT_CLAMP_TO_GND_REGISTER);
}

void AD57X1::setTristateMode(const bool enable) {
  // (1 << this->OUTPUT_TRISTATE_REGISTER) : the dac output is in tristate mode (default)
  // (0 << this->OUTPUT_TRISTATE_REGISTER) : the dac is in normal mode
  this->controlRegister = (this->controlRegister & ~(1 << AD57X1::OUTPUT_TRISTATE_REGISTER)) | (enable << AD57X1::OUTPUT_TRISTATE_REGISTER);
}

void AD57X1::setOffsetBinaryEncoding(const bool enable) {
  // (1 << this->OFFSET_BINARY_REGISTER) : the dac uses offset binary encoding, should be used when writing unsigned ints
  // (0 << this->OFFSET_BINARY_REGISTER) : the dac uses 2s complement encoding, should be used when writing signed ints (default)
  this->controlRegister = (this->controlRegister & ~(1 << AD57X1::OFFSET_BINARY_REGISTER)) | (enable << AD57X1::OFFSET_BINARY_REGISTER);
}

/*
 * Linearity error compensation
 */
// enable = 0 -> Range 0-10 V
// enable = 1 -> Range 0-20 V
void AD57X1::setReferenceInputRange(const bool enableCompensation) {
  this->controlRegister =
    (this->controlRegister & ~(0b1111 << AD57X1::LINEARITY_COMPENSATION_REGISTER))
    | ((enableCompensation ? AD57X1::REFERENCE_RANGE_20V : AD57X1::REFERENCE_RANGE_10V) << AD57X1::LINEARITY_COMPENSATION_REGISTER);
}

uint32_t AD57X1::readControlRegister() const {
  constexpr uint32_t command = AD57X1::READ_REGISTERS | AD57X1::CONTROL_REGISTER;
    const uint32_t result = this->readSPI(command);
    return result;
}

void AD57X1::begin(const bool initSpi) const {
  pinMode(this->PIN_CS, OUTPUT);
  digitalWrite(this->PIN_CS, !this->CS_POLARITY);

  if (this->PIN_LDAC >= 0) {
    pinMode(this->PIN_LDAC, OUTPUT);
    digitalWrite(this->PIN_LDAC, LOW);
  }

  if (initSpi) {
    this->spi->begin();
  }
}

