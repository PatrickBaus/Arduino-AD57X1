// Minimal host-side stand-in for the Arduino core. Only what the AD57X1 library needs.
#ifndef MOCK_ARDUINO_H
#define MOCK_ARDUINO_H

#include <stdint.h>

#include <deque>
#include <string>
#include <vector>

#define LOW 0x0
#define HIGH 0x1
#define INPUT 0x0
#define OUTPUT 0x1

void pinMode(uint8_t pin, uint8_t mode);
void digitalWrite(uint8_t pin, uint8_t value);

class SPIClass;

namespace mock {
enum class EventType { PinMode, DigitalWrite, SpiBegin, BeginTransaction, Transfer, EndTransaction };

struct Event {
  EventType type;
  uint32_t a;                  // pin / byte written (MOSI) / clock frequency
  uint32_t b;                  // pin mode / pin level / byte read (MISO) / SPI data mode
  const SPIClass* bus;         // the SPI bus used, nullptr for GPIO events

  bool operator==(const Event& other) const {
    return type == other.type && a == other.a && b == other.b && bus == other.bus;
  }
};

std::string toString(const Event& event);
void PrintTo(const Event& event, std::ostream* os);

// Everything the library did to the "hardware" in chronological order
extern std::vector<Event> events;
// Bytes returned by SPIClass::transfer(). Returns 0x00 when empty.
extern std::deque<uint8_t> misoQueue;

void reset();
}  // namespace mock

#endif
