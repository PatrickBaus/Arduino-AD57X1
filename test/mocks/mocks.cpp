#include <ostream>
#include <sstream>

#include "Arduino.h"
#include "SPI.h"

SPIClass SPI;

namespace mock {
std::vector<Event> events;
std::deque<uint8_t> misoQueue;

void reset() {
  events.clear();
  misoQueue.clear();
}

std::string toString(const Event& event) {
  static const char* const names[] = {"pinMode", "digitalWrite", "SPI.begin", "beginTransaction", "transfer", "endTransaction"};
  std::ostringstream stream;
  stream << names[static_cast<int>(event.type)] << "(0x" << std::hex << event.a << ", 0x" << event.b << ")";
  return stream.str();
}

void PrintTo(const Event& event, std::ostream* os) {
  *os << toString(event);
}
}  // namespace mock

void pinMode(const uint8_t pin, const uint8_t mode) {
  mock::events.push_back({mock::EventType::PinMode, pin, mode, nullptr});
}

void digitalWrite(const uint8_t pin, const uint8_t value) {
  mock::events.push_back({mock::EventType::DigitalWrite, pin, value, nullptr});
}

void SPIClass::begin() {
  mock::events.push_back({mock::EventType::SpiBegin, 0, 0, this});
}

void SPIClass::beginTransaction(const SPISettings settings) {
  // Only MSB first makes sense for this DAC, so fold the bit order into the event to have it checked everywhere
  mock::events.push_back({mock::EventType::BeginTransaction, settings.clock, static_cast<uint32_t>(settings.dataMode) | (static_cast<uint32_t>(settings.bitOrder) << 8), this});
}

uint8_t SPIClass::transfer(const uint8_t data) {
  uint8_t result = 0x00;
  if (!mock::misoQueue.empty()) {
    result = mock::misoQueue.front();
    mock::misoQueue.pop_front();
  }
  mock::events.push_back({mock::EventType::Transfer, data, result, this});
  return result;
}

void SPIClass::endTransaction() {
  mock::events.push_back({mock::EventType::EndTransaction, 0, 0, this});
}
