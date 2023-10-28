#ifndef DISPLAYMANAGER_H
#define DISPLAYMANAGER_H

#include <Arduino.h>

#include <GxEPD2_BW.h>

class DisplayManagerTest_formatPrice_Test;

class DisplayManager
{
public:
    DisplayManager();

    void writeDisplay(const String& crypto, const String& fiat, float mainPrice, float pctOneDay, 
                      float pctOneWeek, float pctOneYear, const String& dayMonth, const String& time, 
                      int batteryPercent);
    void hibernate();

private:
    friend class ::DisplayManagerTest_formatPrice_Test;
    void addLines();
    void writeMainPrice(const String& price);
    void writeCrypto(const String& price);
    void writeDateTime(const String& dayMonth, const String& time);
    void writeBattery(int batPct);
    String formatPriceString(float price);
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

};

#endif
