#ifndef SDLOGGER_H
#define SDLOGGER_H

#include <Arduino.h>

#include "SPI.h"

// This class is mainly for testing purposes - it can log data to an SD card attached to the board.
// Needed for longer term testing while on battery power
// Could extend this in future to be more generic and also interface with SPIFFS

class SDLogger
{
public:
    SDLogger();

    // append to a file - this will create it if it doesn't exist already
    bool appendFile(const String& path, const String& message);

private:
    SPIClass m_spi;
    bool m_initialised;
};

#endif
