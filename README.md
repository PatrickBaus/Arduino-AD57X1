# Arduino-AD57X1
[![CI](https://github.com/PatrickBaus/Arduino-AD57X1/actions/workflows/ci.yml/badge.svg)](https://github.com/PatrickBaus/Arduino-AD57X1/actions/workflows/ci.yml)

This library is still work in progress and not all features of the DACs are supported. Basic functionality is implemented. More documentation will be added soon.

#### Examples

This is a simple example, to get going. Check out the more in depth examples in [/examples/](./examples/)

```C++
#include <ad57X1.h>

#define CS_AD5781              7
AD5781 ad5781(CS_AD5781, &SPI);   // Use AD5791 if using the 20 bit version

void setup() {
  ad5781.begin();   // Set the pin modes
  SPI.begin();
  ad5781.setOffsetBinaryEncoding(true);   // Set the input encoding to offset binary. Default is 2s complement (false).

  ad5781.enableOutput();    // Turn on the DAC. After startup the output will be clamped to GND and disconnected (tri-state mode)
  ad5781.setValue(0x3FFFF); // Set to full scale output
}

void loop() {

}
```

#### Tests

The library has a unit test suite that runs on the host, no hardware required. The Arduino core and the SPI library are
replaced by mocks (see [/test/mocks/](./test/mocks/)), which record every pin and bus access. The tests need CMake, a C++17
compiler and [GoogleTest](https://github.com/google/googletest) (downloaded automatically, if it is not installed).

```bash
cmake -S test -B build -DAD57X1_COVERAGE=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

To create the coverage report, install [gcovr](https://gcovr.com/) and run

```bash
gcovr --root . --filter src/ --exclude-throw-branches build
```

The CI requires 100 % line, branch and function coverage of the library.
