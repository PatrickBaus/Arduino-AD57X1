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

#define AD5781_SPI_CLOCK_FREQ (1*1000*1000)   // 30 MHz SPI clock is the maximum specified
// Use CPOL = 0,  CPHA =1 for ADI DACs

AD5781::AD5781(uint8_t cs_pin, uint8_t ldac_pin, double referenceVoltage) : cs_pin(cs_pin), ldac_pin(ldac_pin), referenceVoltage(referenceVoltage), spi_settings(SPISettings(AD5781_SPI_CLOCK_FREQ, MSBFIRST, SPI_MODE1)) {
}

AD5781::AD5781(uint8_t cs_pin, double referenceVoltage) : cs_pin(cs_pin), ldac_pin(-1), referenceVoltage(referenceVoltage), spi_settings(SPISettings(AD5781_SPI_CLOCK_FREQ, MSBFIRST, SPI_MODE1)) {
}

// TODO: use an SPI object to select the SPI bus
void AD5781::writeSPI(uint32_t value) {
  SPI1.beginTransaction(this->spi_settings);
  digitalWrite (this->cs_pin, LOW);

  #ifdef ARDUINO_AD5762R_DEBUG
  Serial.print("Writing to SPI:");
  Serial.println(value, HEX);
  #endif

  SPI1.transfer((value >> 16) & 0xFF);
  SPI1.transfer((value >> 8) & 0xFF);
  SPI1.transfer((value >> 0) & 0xFF);

  digitalWrite(this->cs_pin, HIGH);
  SPI1.endTransaction();
}

void AD5781::updateControlRegister() {
  this->writeSPI(this->WRITE_REGISTERS | this->CONTROL_REGISTER | this->controlRegister);
}

void AD5781::reset() {
  this->enableOutput();
}

// value is an 18-bit value
void AD5781::setValue(uint32_t value) {
  uint32_t command = this->WRITE_REGISTERS | this->DAC_REGISTER | ((value << this->DAC_REGSISTER_VALUE_OFFSET) & 0xFFFFF);

  this->writeSPI(command);
  this->writeSPI(this->SW_CONTROL_REGISTER | this->SW_CONTROL_LDAC);
}

void AD5781::enableOutput() {
  this->setInternalAmplifier(false);
  this->setOutputClamp(false);
  this->setTristateMode(false);
  this->setOffsetBinaryEncoding(false);
  // If the reference voltage range is larger than 10V
  // some linearity compensation needs to be applied.
  this->setReferenceInputRange(this->referenceVoltage > 10.0);
  this->updateControlRegister();
}

void AD5781::setInternalAmplifier(bool enable) {
  // (1 << this->RBUF_REGISTER) : internal amplifier is disabled (default)
  // (0 << this->RBUF_REGISTER) : internal amplifier is enabled
  this->controlRegister = (this->controlRegister & ~(1 << this->RBUF_REGISTER)) | (!enable << this->RBUF_REGISTER);
}

// Setting this to enabled will overrule the tristate mode and clamp the output to GND
void AD5781::setOutputClamp(bool enable) {
  // (1 << this->OUTPUT_CLAMP_TO_GND_REGISTER) : the output is clamped to GND (default)
  // (0 << this->OUTPUT_CLAMP_TO_GND_REGISTER) : the dac is in normal mode
  this->controlRegister = (this->controlRegister & ~(1 << this->OUTPUT_CLAMP_TO_GND_REGISTER)) | (enable << this->OUTPUT_CLAMP_TO_GND_REGISTER);
}

void AD5781::setTristateMode(bool enable) {
  // (1 << this->OUTPUT_TRISTATE_REGISTER) : the dac output is in tristate mode (default)
  // (0 << this->OUTPUT_TRISTATE_REGISTER) : the dac is in normal mode
  this->controlRegister = (this->controlRegister & ~(1 << this->OUTPUT_TRISTATE_REGISTER)) | (enable << this->OUTPUT_TRISTATE_REGISTER);
}

void AD5781::setOffsetBinaryEncoding(bool enable) {
  // (1 << this->OFFSET_BINARY_REGISTER) : the dac uses offset binary encoding, should be used when writing unsigned ints
  // (0 << this->OFFSET_BINARY_REGISTER) : the dac uses 2s complement encoding, should be used when writing signed ints (default)
  this->controlRegister = (this->controlRegister & ~(1 << this->OFFSET_BINARY_REGISTER)) | (enable << this->OFFSET_BINARY_REGISTER);
}

/* Linearity error compensation
 * 
 */
// enable = 0 -> Range 0-10 V
// enable = 1 -> Range 0-20 V
void AD5781::setReferenceInputRange(bool enableCompensation) {
  this->controlRegister = (this->controlRegister & ~(0b1111 << this->LINEARITY_COMPENSATION_REGISTER)) | ((enableCompensation ? this->REFERENCE_RANGE_20V : this->REFERENCE_RANGE_10V) << this->LINEARITY_COMPENSATION_REGISTER);
}

void AD5781::begin() {
  pinMode(this->cs_pin, OUTPUT);
  digitalWrite(this->cs_pin, HIGH);

  if (this->ldac_pin >= 0) {
    pinMode(this->ldac_pin, OUTPUT);
    digitalWrite(this->ldac_pin, LOW);
  }
  //this->reset();
}

