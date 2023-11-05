#include "SDLogger.h"

// ESP libraries
#include "FS.h"
#include "SD.h"

// based on schematic:
#define SCK  14
#define MISO 2
#define MOSI 15
#define CS   13

SDLogger::SDLogger() :
    m_spi(SPIClass(HSPI)), // the display is using VSPI
    m_initialised(false) 
{
    m_spi.begin(SCK, MISO, MOSI, CS);
    if (!SD.begin(CS,m_spi,80000000)) 
    {
        Serial.println("Card Mount Failed");
        return;
    }

    if(SD.cardType() == CARD_NONE) 
    {
        Serial.println("No SD card attached");
        return;
    }

    m_initialised = true;
}

bool SDLogger::appendFile(const String& path, const String& message)
{
    if (!m_initialised)
    {
        Serial.printf("SD card failed init, cannot append");
        return false;
    }

    bool success = false;

    File file = SD.open(path, FILE_APPEND);
    if(!file)
    {
        Serial.println("SDLogger::appendFile - Failed to open file");
        return success;
    }
    if(file.print(message))
    {
        success = true;
    } 
    else 
    {
        Serial.println("SDLogger::appendFile - Failed to append");
    }

    file.close();

    return success;
}
