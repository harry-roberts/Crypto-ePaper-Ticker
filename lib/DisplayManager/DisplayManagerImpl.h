#ifndef DISPLAYMANAGERIMPL_H
#define DISPLAYMANAGERIMPL_H

#include <Arduino.h>
#include <map>

#include <GxEPD2_BW.h>

#include <FreeSans18pt7b_edit.h>

class DisplayManagerTest_formatPrice_Test;
class DisplayManagerTest_formatPriceChange_Test;

// implementation for a 250x122 display
class DisplayManagerImpl
{
public:
    DisplayManagerImpl(int rotation = 1);

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
    void fillScreen();

private:
    friend class ::DisplayManagerTest_formatPrice_Test;
    friend class ::DisplayManagerTest_formatPriceChange_Test;

    void writeDisplayAdvanced(const String& crypto, const String& fiat, std::map<long, float>& priceData, const String& dayMonth, 
                              const String& time, const int batteryPercent);
    void writeDisplaySimple(const String& crypto, const String& fiat, std::map<long, float>& priceData, const String& dayMonth, 
                            const String& time, const int batteryPercent);

    void addLines();
    void fillCryptoBox(bool centre = false);
    void writeMainPriceAdvanced(const String& price);
    void writeMainPriceSimple(const String& price);
    void writeCrypto(const String& crypto, const bool centre = false);
    void writeDateTimeAdvanced(const String& dayMonth, const String& time);
    void writeDateTimeSimple(const String& dayMonth, const String& time);
    void writeBatteryAdvanced(const int batPct);
    void writeBatterySimple(const int batPct);
    bool writePriceChange(const float mainPrice, const float priceToCompare, const String& timeframe, const int yOffset, 
                          const bool centre = false);

    void drawArrow(const bool isPositive);

    void setCryptoBoxWidth(const String& crypto, const String& dayMonth, const String& time, bool centre = false);
    String formatPriceString(const float price);
    String formatPriceChangeString(float percentChange, const String& timeframe);
    void formatCommas(char *buf, const int price);

    GxEPD2_BW<GxEPD2_213_BN, GxEPD2_213_BN::HEIGHT> m_display;

    const int m_max_x = 249;
    const int m_max_y = 121;

    const uint16_t m_min_crypto_padding = 6;
    const uint16_t m_min_date_padding = 6;
    const uint16_t m_min_allowed_crypto_box_width = 89;
    const uint16_t m_max_allowed_crypto_box_width_advanced = 102;
    const uint16_t m_max_allowed_crypto_box_width_simple = 110;

    const int m_crypto_box_x1 = 0;
    const int m_crypto_box_y1 = 0;
    int m_crypto_box_x2 = m_min_allowed_crypto_box_width;
    const int m_crypto_box_y2 = 39;

    const int m_date_box_x1 = 15;
    const int m_date_box_y1 = 77;
    const int m_date_box_y2 = m_max_y;

    const int m_bat_box_x1 = 0;
    const int m_bat_box_y1 = 77;
    const int m_bat_box_x2 = m_date_box_x1;
    const int m_bat_box_y2 = m_max_y;

    const GFXfont* const m_default_crypto_box_font = &FreeSans18pt7b;
    const GFXfont* m_current_crypto_box_font = m_default_crypto_box_font;

    std::map<String, char> m_fiatSymbols = {{"GBP", '#'},
                                            {"USD", '$'},
                                            {"EUR", '&'}}; // as edited in original font

    
};

#endif
