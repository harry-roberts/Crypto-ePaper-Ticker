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
        log_w("Card Mount Failed");
        return;
    }

    if(SD.cardType() == CARD_NONE) 
    {
        log_w("No SD card attached");
        return;
    }

    m_initialised = true;
}

bool SDLogger::appendFile(const String& path, const String& message)
{
    if (!m_initialised)
    {
        log_w("SD card failed init, cannot append");
        return false;
    }

    bool success = false;

    File file = SD.open(path, FILE_APPEND);
    if(!file)
    {
        log_w("Failed to open file");
        return success;
    }
    if(file.print(message))
    {
        success = true;
    } 
    else 
    {
        log_w("Failed to append");
    }

    file.close();

    return success;
}
