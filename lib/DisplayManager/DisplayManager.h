#ifndef DISPLAYMANAGER_H
#define DISPLAYMANAGER_H

#include <Arduino.h>

#include <GxEPD2_BW.h>

class DisplayManagerTest_formatPrice_Test;

class DisplayManager
{
public:
    DisplayManager();

    void writePriceDisplay(float price, const String& crypto, const String& fiat, const String& bat);
    void hibernate();

private:
    friend class ::DisplayManagerTest_formatPrice_Test;
    void addLines();
    void writeMainPrice(const String& price);
    void writeCrypto(const String& price);
    String formatPriceString(float price);
    void formatCommas(char *buf, int price);

    GxEPD2_BW<GxEPD2_213_BN, GxEPD2_213_BN::HEIGHT> m_display;

    const int m_max_x = 249;
    const int m_max_y = 121;

    const int m_crypto_box_x = 89;
    const int m_crypto_box_y = 39;

};

#endif
