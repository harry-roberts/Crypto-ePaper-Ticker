#ifndef DISPLAYMANAGER_H
#define DISPLAYMANAGER_H

#include <Arduino.h>
#include <memory>
#include <map>

class DisplayManagerImpl;

class DisplayManager
{
public:
    DisplayManager();
    ~DisplayManager();

    void writeDisplay(const String& crypto, const String& fiat, std::map<long, float>& priceData, const String& dayMonth, 
                      const String& time, const int batteryPercent);
    void writeGenericText(const String& textToWrite);
    void hibernate();

    void drawCannotConnectToWifi(const String& ssid, const String& password);
    void drawWifiHasNoInternet();
    void drawLowBattery();
    void drawYesWifiNoCrypto(const String& dayMonth, const String& time);
    void drawConfig(const String& ssid, const String& password, const String& crypto, const String& fiat,
                    const int refreshInterval);
    void drawAccessPoint(const String& ip);
    void drawOvernightSleep();
    void drawStartingConfigMode();
    void fillScreen();

private:
    std::unique_ptr<DisplayManagerImpl> m_impl;

};

#endif
