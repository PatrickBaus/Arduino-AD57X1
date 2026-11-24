// Host-side unit tests for the AD57X1 library. The Arduino core and the SPI library are replaced by the mocks in
// test/mocks, which record every pin and bus access, so the tests can compare against the exact sequence on the wire.
#include <gtest/gtest.h>

#include <initializer_list>
#include <vector>

#include "ad57X1.h"

using mock::Event;
using mock::EventType;

namespace {
constexpr uint8_t PIN_CS = 7;
constexpr int16_t PIN_LDAC = 9;
constexpr uint32_t DEFAULT_CLOCK = 1000000;
// The library must always use SPI mode 1 (CPOL = 0, CPHA = 1), MSB first
constexpr uint32_t EXPECTED_SPI_MODE = SPI_MODE1 | (MSBFIRST << 8);

// Control register content, see p. 22 of the datasheet
constexpr uint32_t CTRL_WRITE = 0x200000;
constexpr uint32_t RBUF = 1 << 1;
constexpr uint32_t OPGND = 1 << 2;
constexpr uint32_t DACTRI = 1 << 3;
constexpr uint32_t BIN2SC = 1 << 4;
constexpr uint32_t LIN_COMP_20V = 0b1100 << 6;
constexpr uint32_t CTRL_DEFAULT = RBUF | OPGND | DACTRI | BIN2SC;

constexpr uint32_t SOFTWARE_LDAC = 0x400001;
constexpr uint32_t NOP = 0x000000;

typedef std::vector<Event> Events;

Event pinModeEvent(const uint8_t pin, const uint8_t mode) {
  return {EventType::PinMode, pin, mode, nullptr};
}

Event writeEvent(const uint8_t pin, const uint8_t level) {
  return {EventType::DigitalWrite, pin, level, nullptr};
}

Event spiBeginEvent(const SPIClass* bus = &SPI) {
  return {EventType::SpiBegin, 0, 0, bus};
}

// A complete 24 bit frame: claim the bus, assert CS, 3 bytes MSB first, release CS, release the bus
Events frame(const uint32_t mosi, const uint32_t miso = 0, const bool csActiveLevel = HIGH, const SPIClass* bus = &SPI, const uint32_t clock = DEFAULT_CLOCK) {
  return {
    {EventType::BeginTransaction, clock, EXPECTED_SPI_MODE, bus},
    writeEvent(PIN_CS, csActiveLevel),
    {EventType::Transfer, (mosi >> 16) & 0xFF, (miso >> 16) & 0xFF, bus},
    {EventType::Transfer, (mosi >> 8) & 0xFF, (miso >> 8) & 0xFF, bus},
    {EventType::Transfer, mosi & 0xFF, miso & 0xFF, bus},
    writeEvent(PIN_CS, !csActiveLevel),
    {EventType::EndTransaction, 0, 0, bus},
  };
}

Events concat(const std::initializer_list<Events> parts) {
  Events result;
  for (const Events& part : parts) {
    result.insert(result.end(), part.begin(), part.end());
  }
  return result;
}

// Queue the answer of the DAC to a readback. The DAC is silent during the first frame (the read command) and clocks
// out the register during the second frame.
void queueReadback(const uint32_t value) {
  mock::misoQueue = {0x00, 0x00, 0x00, static_cast<uint8_t>(value >> 16), static_cast<uint8_t>(value >> 8), static_cast<uint8_t>(value)};
}

class AD57X1Test : public ::testing::Test {
  protected:
    void SetUp() override { mock::reset(); }
    void TearDown() override { mock::reset(); }
};

/*
 * begin()
 */
TEST_F(AD57X1Test, BeginConfiguresChipSelectAndStartsSpi) {
  const AD5781 dac(PIN_CS, &SPI);
  dac.begin();

  // CS is active high by default, so it must idle low
  EXPECT_EQ(mock::events, (Events{pinModeEvent(PIN_CS, OUTPUT), writeEvent(PIN_CS, LOW), spiBeginEvent()}));
}

TEST_F(AD57X1Test, BeginWithoutSpiInitDoesNotTouchTheBus) {
  const AD5781 dac(PIN_CS, &SPI);
  dac.begin(false);

  EXPECT_EQ(mock::events, (Events{pinModeEvent(PIN_CS, OUTPUT), writeEvent(PIN_CS, LOW)}));
}

TEST_F(AD57X1Test, BeginConfiguresLdacPin) {
  const AD5781 dac(PIN_CS, &SPI, DEFAULT_CLOCK, PIN_LDAC);
  dac.begin();

  EXPECT_EQ(mock::events, (Events{
    pinModeEvent(PIN_CS, OUTPUT), writeEvent(PIN_CS, LOW),
    pinModeEvent(PIN_LDAC, OUTPUT), writeEvent(PIN_LDAC, LOW),
    spiBeginEvent(),
  }));
}

TEST_F(AD57X1Test, BeginWithActiveLowChipSelectIdlesHigh) {
  const AD5791 dac(PIN_CS, &SPI, DEFAULT_CLOCK, -1, false);
  dac.begin();

  EXPECT_EQ(mock::events, (Events{pinModeEvent(PIN_CS, OUTPUT), writeEvent(PIN_CS, HIGH), spiBeginEvent()}));
}

TEST_F(AD57X1Test, BeginUsesTheGivenSpiBus) {
  SPIClass otherBus;
  const AD5791 dac(PIN_CS, &otherBus);
  dac.begin();

  EXPECT_EQ(mock::events.back(), spiBeginEvent(&otherBus));
}

TEST_F(AD57X1Test, ConstructorDoesNotTouchTheHardware) {
  // Global objects are constructed before the Arduino core is initialized, so the constructor must stay passive
  const AD5781 dac18(PIN_CS, &SPI, DEFAULT_CLOCK, PIN_LDAC);
  const AD5791 dac20(PIN_CS, &SPI, DEFAULT_CLOCK, PIN_LDAC);
  const AD57X1 dacGeneric(PIN_CS, &SPI, 0, DEFAULT_CLOCK, PIN_LDAC, true);

  EXPECT_TRUE(mock::events.empty());
}

/*
 * setValue()
 */
TEST_F(AD57X1Test, SetValueAD5781ShiftsTheValueAndLatchesViaSoftwareLdac) {
  const AD5781 dac(PIN_CS, &SPI);
  dac.setValue(0x3FFFF);

  // The 18 bit value is left aligned in the 20 bit data field
  EXPECT_EQ(mock::events, concat({frame(0x1FFFFC), frame(SOFTWARE_LDAC)}));
}

TEST_F(AD57X1Test, SetValueAD5791UsesAll20Bits) {
  const AD5791 dac(PIN_CS, &SPI);
  dac.setValue(0xFFFFF);

  EXPECT_EQ(mock::events, concat({frame(0x1FFFFF), frame(SOFTWARE_LDAC)}));
}

TEST_F(AD57X1Test, SetValueZero) {
  const AD5781 dac(PIN_CS, &SPI);
  dac.setValue(0);

  EXPECT_EQ(mock::events, concat({frame(0x100000), frame(SOFTWARE_LDAC)}));
}

TEST_F(AD57X1Test, SetValueKeepsBitPattern) {
  const AD5781 dac18(PIN_CS, &SPI);
  dac18.setValue(0x2A5A5);
  const AD5791 dac20(PIN_CS, &SPI);
  dac20.setValue(0xA5A5A);

  EXPECT_EQ(mock::events, concat({frame(0x100000 | (0x2A5A5 << 2)), frame(SOFTWARE_LDAC), frame(0x1A5A5A), frame(SOFTWARE_LDAC)}));
}

TEST_F(AD57X1Test, SetValueOutOfRangeMustNotCorruptTheRegisterAddress) {
  const AD5781 dac18(PIN_CS, &SPI);
  dac18.setValue(0xFFFFFFFF);
  const AD5791 dac20(PIN_CS, &SPI);
  dac20.setValue(0xFFFFFFFF);

  // Excess bits are dropped. They must never leak into the R/W and address bits.
  EXPECT_EQ(mock::events, concat({frame(0x1FFFFC), frame(SOFTWARE_LDAC), frame(0x1FFFFF), frame(SOFTWARE_LDAC)}));
}

TEST_F(AD57X1Test, SetValueWithLdacPinPulsesThePinInsteadOfSoftwareLdac) {
  const AD5781 dac(PIN_CS, &SPI, DEFAULT_CLOCK, PIN_LDAC);
  dac.setValue(0x20000);

  EXPECT_EQ(mock::events, concat({frame(0x180000), {writeEvent(PIN_LDAC, HIGH), writeEvent(PIN_LDAC, LOW)}}));
}

TEST_F(AD57X1Test, SetValueWithLdacOnPinZero) {
  // Pin 0 is a valid pin. Only negative numbers disable the LDAC pin.
  const AD5791 dac(PIN_CS, &SPI, DEFAULT_CLOCK, 0);
  dac.setValue(1);

  EXPECT_EQ(mock::events, concat({frame(0x100001), {writeEvent(0, HIGH), writeEvent(0, LOW)}}));
}

TEST_F(AD57X1Test, SetValueWithActiveLowChipSelect) {
  const AD5791 dac(PIN_CS, &SPI, DEFAULT_CLOCK, -1, false);
  dac.setValue(0x80000);

  EXPECT_EQ(mock::events, concat({frame(0x180000, 0, LOW), frame(SOFTWARE_LDAC, 0, LOW)}));
}

TEST_F(AD57X1Test, SetValueUsesTheGivenBusAndClockFrequency) {
  SPIClass otherBus;
  const AD5781 dac(PIN_CS, &otherBus, 30UL * 1000 * 1000);
  dac.setValue(1);

  EXPECT_EQ(mock::events, concat({frame(0x100004, 0, HIGH, &otherBus, 30000000), frame(SOFTWARE_LDAC, 0, HIGH, &otherBus, 30000000)}));
}

/*
 * setClearCodeValue()
 */
TEST_F(AD57X1Test, SetClearCodeValueAD5781) {
  const AD5781 dac(PIN_CS, &SPI);
  dac.setClearCodeValue(0x20000);

  EXPECT_EQ(mock::events, frame(0x380000));
}

TEST_F(AD57X1Test, SetClearCodeValueAD5791) {
  const AD5791 dac(PIN_CS, &SPI);
  dac.setClearCodeValue(0x12345);

  EXPECT_EQ(mock::events, frame(0x312345));
}

TEST_F(AD57X1Test, SetClearCodeValueOutOfRangeMustNotCorruptTheRegisterAddress) {
  const AD5781 dac(PIN_CS, &SPI);
  dac.setClearCodeValue(0xFFFFFFFF);

  EXPECT_EQ(mock::events, frame(0x3FFFFC));
}

/*
 * Readback
 */
TEST_F(AD57X1Test, ReadValueAD5781) {
  const AD5781 dac(PIN_CS, &SPI);
  queueReadback(0x0FFFFC);

  EXPECT_EQ(dac.readValue(), 0x3FFFFu);
  // Read command for the DAC register, then a NOP to clock out the result
  EXPECT_EQ(mock::events, concat({frame(0x900000), frame(NOP, 0x0FFFFC)}));
}

TEST_F(AD57X1Test, ReadValueAD5791) {
  const AD5791 dac(PIN_CS, &SPI);
  queueReadback(0x0ABCDE);

  EXPECT_EQ(dac.readValue(), 0xABCDEu);
  EXPECT_EQ(mock::events, concat({frame(0x900000), frame(NOP, 0x0ABCDE)}));
}

TEST_F(AD57X1Test, ReadValueStripsTheAddressBits) {
  // The upper 4 bits of the frame are not part of the value, whatever the DAC or a floating SDO line puts there
  const AD5781 dac18(PIN_CS, &SPI);
  queueReadback(0x9FFFFC);
  EXPECT_EQ(dac18.readValue(), 0x3FFFFu);

  const AD5791 dac20(PIN_CS, &SPI);
  queueReadback(0xF00001);
  EXPECT_EQ(dac20.readValue(), 0x00001u);
}

TEST_F(AD57X1Test, ReadValueWithActiveLowChipSelect) {
  const AD5791 dac(PIN_CS, &SPI, DEFAULT_CLOCK, -1, false);
  queueReadback(0x000042);

  EXPECT_EQ(dac.readValue(), 0x42u);
  EXPECT_EQ(mock::events, concat({frame(0x900000, 0, LOW), frame(NOP, 0x000042, LOW)}));
}

TEST_F(AD57X1Test, ReadControlRegister) {
  const AD5781 dac(PIN_CS, &SPI);
  queueReadback(0x000312);

  EXPECT_EQ(dac.readControlRegister(), 0x312u);
  EXPECT_EQ(mock::events, concat({frame(0xA00000), frame(NOP, 0x000312)}));
}

TEST_F(AD57X1Test, ReadControlRegisterStripsTheAddressBits) {
  const AD5791 dac(PIN_CS, &SPI);
  queueReadback(0xA0001E);

  EXPECT_EQ(dac.readControlRegister(), 0x1Eu);
}

/*
 * Control register
 */
TEST_F(AD57X1Test, UpdateControlRegisterWritesTheDefaults) {
  const AD5781 dac(PIN_CS, &SPI);
  dac.updateControlRegister();

  // Output clamped and tristated, internal amplifier off, offset binary encoding, no linearity compensation
  EXPECT_EQ(mock::events, frame(CTRL_WRITE | CTRL_DEFAULT));
}

TEST_F(AD57X1Test, SettersOnlyChangeTheShadowRegister) {
  AD5781 dac(PIN_CS, &SPI);
  dac.setInternalAmplifier(true);
  dac.setOutputClamp(false);
  dac.setTristateMode(false);
  dac.setOffsetBinaryEncoding(false);
  dac.setReferenceInputRange(true);

  // Nothing is sent until updateControlRegister() is called
  EXPECT_TRUE(mock::events.empty());

  dac.updateControlRegister();
  EXPECT_EQ(mock::events, frame(CTRL_WRITE | LIN_COMP_20V));
}

TEST_F(AD57X1Test, EnableOutputReleasesClampAndTristate) {
  AD5781 dac(PIN_CS, &SPI);
  dac.enableOutput();

  EXPECT_EQ(mock::events, frame(CTRL_WRITE | RBUF | BIN2SC));
}

TEST_F(AD57X1Test, EnableOutputKeepsOtherSettings) {
  AD5791 dac(PIN_CS, &SPI);
  dac.setInternalAmplifier(true);
  dac.setOffsetBinaryEncoding(false);
  dac.setReferenceInputRange(true);
  dac.enableOutput();

  EXPECT_EQ(mock::events, frame(CTRL_WRITE | LIN_COMP_20V));
}

TEST_F(AD57X1Test, ResetEnablesTheOutput) {
  AD5781 dac(PIN_CS, &SPI);
  dac.reset();

  EXPECT_EQ(mock::events, frame(CTRL_WRITE | RBUF | BIN2SC));
}

TEST_F(AD57X1Test, SetInternalAmplifier) {
  AD5781 dac(PIN_CS, &SPI);
  // RBUF is inverted: 0 enables the amplifier
  dac.setInternalAmplifier(true);
  dac.updateControlRegister();
  dac.setInternalAmplifier(false);
  dac.updateControlRegister();

  EXPECT_EQ(mock::events, concat({frame(CTRL_WRITE | (CTRL_DEFAULT & ~RBUF)), frame(CTRL_WRITE | CTRL_DEFAULT)}));
}

TEST_F(AD57X1Test, SetOutputClamp) {
  AD5781 dac(PIN_CS, &SPI);
  dac.setOutputClamp(false);
  dac.updateControlRegister();
  dac.setOutputClamp(true);
  dac.updateControlRegister();

  EXPECT_EQ(mock::events, concat({frame(CTRL_WRITE | (CTRL_DEFAULT & ~OPGND)), frame(CTRL_WRITE | CTRL_DEFAULT)}));
}

TEST_F(AD57X1Test, SetTristateMode) {
  AD5781 dac(PIN_CS, &SPI);
  dac.setTristateMode(false);
  dac.updateControlRegister();
  dac.setTristateMode(true);
  dac.updateControlRegister();

  EXPECT_EQ(mock::events, concat({frame(CTRL_WRITE | (CTRL_DEFAULT & ~DACTRI)), frame(CTRL_WRITE | CTRL_DEFAULT)}));
}

TEST_F(AD57X1Test, SetOffsetBinaryEncoding) {
  AD5781 dac(PIN_CS, &SPI);
  dac.setOffsetBinaryEncoding(false);
  dac.updateControlRegister();
  dac.setOffsetBinaryEncoding(true);
  dac.updateControlRegister();

  EXPECT_EQ(mock::events, concat({frame(CTRL_WRITE | (CTRL_DEFAULT & ~BIN2SC)), frame(CTRL_WRITE | CTRL_DEFAULT)}));
}

TEST_F(AD57X1Test, SetReferenceInputRange) {
  AD5781 dac(PIN_CS, &SPI);
  dac.setReferenceInputRange(true);
  dac.updateControlRegister();
  dac.setReferenceInputRange(false);
  dac.updateControlRegister();

  EXPECT_EQ(mock::events, concat({frame(CTRL_WRITE | CTRL_DEFAULT | LIN_COMP_20V), frame(CTRL_WRITE | CTRL_DEFAULT)}));
}

TEST_F(AD57X1Test, SettersAreIdempotent) {
  AD5781 dac(PIN_CS, &SPI);
  dac.setInternalAmplifier(true);
  dac.setInternalAmplifier(true);
  dac.setOutputClamp(true);
  dac.setTristateMode(true);
  dac.setOffsetBinaryEncoding(true);
  dac.setReferenceInputRange(true);
  dac.setReferenceInputRange(true);
  dac.updateControlRegister();

  EXPECT_EQ(mock::events, frame(CTRL_WRITE | OPGND | DACTRI | BIN2SC | LIN_COMP_20V));
}

TEST_F(AD57X1Test, InstancesDoNotShareTheControlRegister) {
  AD5781 dacA(PIN_CS, &SPI);
  const AD5781 dacB(PIN_CS, &SPI);
  dacA.enableOutput();
  dacB.updateControlRegister();

  EXPECT_EQ(mock::events, concat({frame(CTRL_WRITE | RBUF | BIN2SC), frame(CTRL_WRITE | CTRL_DEFAULT)}));
}

/*
 * The mocks themselves
 */
TEST_F(AD57X1Test, MockEventsArePrintable) {
  EXPECT_EQ(mock::toString(writeEvent(7, HIGH)), "digitalWrite(0x7, 0x1)");
  EXPECT_EQ(::testing::PrintToString(pinModeEvent(10, OUTPUT)), "pinMode(0xa, 0x1)");
}
}  // namespace
