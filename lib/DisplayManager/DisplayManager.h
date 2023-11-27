#ifndef DISPLAYMANAGER_H
#define DISPLAYMANAGER_H

#include <Arduino.h>
#include <memory>

#include "DisplayManagerImpl.h"

class DisplayManager
{
public:
    DisplayManager();
    ~DisplayManager();

    void writeDisplay(const String& crypto, const String& fiat, float mainPrice, float priceOneDay, 
                      float priceOneMonth, float priceOneYear, const String& dayMonth, const String& time, 
                      int batteryPercent);
    void writeGenericText(const String& textToWrite);
    void hibernate();

    void drawCannotConnectToWifi(const String& ssid, const String& password);
    void drawWifiHasNoInternet();
    void drawLowBattery();
    void drawYesWifiNoCrypto(const String& dayMonth, const String& time);

private:
    std::unique_ptr<DisplayManagerImpl> m_impl;

};

#endif
