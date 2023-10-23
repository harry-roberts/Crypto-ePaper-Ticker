#include <gtest/gtest.h>
#include <Arduino.h>
#include "compile_time.h"

// ------------------------------------------------------------------------
// Note for running tests on ESP8266 (probably other Arduino boards also):
//      It seems any test needs to have some form of serial output for the
//      test to be recognised by PlatformIO when running without verbose. 
//      With no output, the test will either silently pass without being 
//      counted, or only give information when it fails. In verbose mode,
//      the test will show info regardless, but still not show in the final
//      count/report at the end.
// ------------------------------------------------------------------------

// Same main() issue as described in src:
// The Arduino framework internally defines the main() function, which
// calls setup() once and loop() on repeat
// Tricky to remove this behaviour, so just treat setup() as this 
// program's main() and leave loop() blank

void setup()
{
    Serial.begin(115200); // test_speed in platformio.ini
    delay(200);
    Serial.println();
    Serial.print("Using: ");
    Serial.print(UNIX_TIMESTAMP); // see compile_time.h
    Serial.print(" as test timestamp (");
    Serial.print(__DATE__);
    Serial.print(" at ");
    Serial.print(__TIME__);
    Serial.println(")");
    Serial.println();

    ::testing::InitGoogleTest();

    // Run tests
    if (RUN_ALL_TESTS());

    // sleep for 1 sec
    delay(1000);
}

void loop() {}
