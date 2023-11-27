#include "DisplayManager.h"

DisplayManager::DisplayManager() :
    m_impl(new DisplayManagerImpl())
{
}

DisplayManager::~DisplayManager() = default;

void DisplayManager::writeDisplay(const String& crypto, const String& fiat, float mainPrice, float priceOneDay, 
                                  float priceOneMonth, float priceOneYear, const String& dayMonth, const String& time, 
                                  int batteryPercent)
{    
    m_impl->writeDisplay(crypto, fiat, mainPrice, priceOneDay, priceOneMonth, priceOneYear, dayMonth, time, batteryPercent);
}

void DisplayManager::writeGenericText(const String& textToWrite)
{
    m_impl->writeGenericText(textToWrite);
}


void DisplayManager::hibernate()
{
    m_impl->hibernate();
}

void DisplayManager::drawCannotConnectToWifi(const String& ssid, const String& password)
{
    m_impl->drawCannotConnectToWifi(ssid, password);
}

void DisplayManager::drawWifiHasNoInternet()
{
    m_impl->drawWifiHasNoInternet();
}

void DisplayManager::drawLowBattery()
{
    m_impl->drawLowBattery();
}

void DisplayManager::drawYesWifiNoCrypto(const String& dayMonth, const String& time)
{
    m_impl->drawYesWifiNoCrypto(dayMonth, time);
}
