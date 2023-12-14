#ifndef DISPLAYMANAGERIMPL_H
#define DISPLAYMANAGERIMPL_H

#include <Arduino.h>
#include <map>

#include <GxEPD2_BW.h>

class DisplayManagerTest_formatPrice_Test;
class DisplayManagerTest_formatPriceChange_Test;

class DisplayManagerImpl
{
public:
    DisplayManagerImpl(int rotation = 1);

    void writeDisplay(const String& crypto, const String& fiat, std::map<long, float>& priceData, const String& dayMonth, 
                      const String& time, int batteryPercent);

    void writeGenericText(const String& textToWrite);
    void hibernate();

    void drawCannotConnectToWifi(const String& ssid, const String& password);
    void drawWifiHasNoInternet();
    void drawLowBattery();
    void drawYesWifiNoCrypto(const String& dayMonth, const String& time);
    void drawConfig(const String& ssid, const String& password, const String& crypto, const String& fiat,
                    int refreshInterval);
    void drawAccessPoint(const String& ip);

private:
    friend class ::DisplayManagerTest_formatPrice_Test;
    friend class ::DisplayManagerTest_formatPriceChange_Test;

    void addLines();
    void writeMainPrice(const String& price);
    void writeCrypto(const String& crypto);
    void writeDateTime(const String& dayMonth, const String& time);
    void writeBattery(int batPct);
    bool writePriceChange(float mainPrice, float priceToCompare, const String& timeframe, int yOffset);

    void drawArrow(bool isPositive);

    String formatPriceString(float price);
    String formatPriceChangeString(float percentChange, const String& timeframe);
    void formatCommas(char *buf, int price);

    GxEPD2_BW<GxEPD2_213_BN, GxEPD2_213_BN::HEIGHT> m_display;

    const int m_max_x = 249;
    const int m_max_y = 121;

    const int m_crypto_box_x1 = 0;
    const int m_crypto_box_y1 = 0;
    const int m_crypto_box_x2 = 89;
    const int m_crypto_box_y2 = 39;

    const int m_date_box_x1 = 15;
    const int m_date_box_y1 = 77;
    const int m_date_box_x2 = m_crypto_box_x2;
    const int m_date_box_y2 = m_max_y;

    const int m_bat_box_x1 = 0;
    const int m_bat_box_y1 = 77;
    const int m_bat_box_x2 = m_date_box_x1;
    const int m_bat_box_y2 = m_max_y;

    std::map<String, char> m_fiatSymbols = {{"GBP", '#'},
                                            {"USD", '$'},
                                            {"EUR", '&'}}; // as edited in original font

    
};

#endif
