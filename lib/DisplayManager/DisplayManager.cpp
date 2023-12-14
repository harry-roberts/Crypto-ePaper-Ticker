#include "DisplayManager.h"
#include "DisplayManagerImpl.h"

DisplayManager::DisplayManager() :
    m_impl(new DisplayManagerImpl())
{
}

DisplayManager::~DisplayManager() = default;

void DisplayManager::writeDisplay(const String& crypto, const String& fiat, std::map<long, float>& priceData, const String& dayMonth, 
                                  const String& time, int batteryPercent)
{    
    m_impl->writeDisplay(crypto, fiat, priceData, dayMonth, time, batteryPercent);
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

void DisplayManager::drawConfig(const String& ssid, const String& password, const String& crypto, const String& fiat,
                                int refreshInterval)
{
    m_impl->drawConfig(ssid, password, crypto, fiat, refreshInterval);
}

void DisplayManager::drawAccessPoint(const String& ip)
{
    m_impl->drawAccessPoint(ip);
}
