#include "DisplayManager.h"

#include <Fonts/FreeSans18pt7b.h>

DisplayManager::DisplayManager() :
    m_display(GxEPD2_213_BN(/*CS=5*/ SS, /*DC=*/ 17, /*RST=*/ 16, /*BUSY=*/ 4))
{
    m_display.init(115200, true, 2, false); // USE THIS for Waveshare boards with "clever" reset circuit, 2ms reset pulse
    m_display.setRotation(1);
}

void DisplayManager::writePriceDisplay(float price, String crypto, String fiat)
{    
    m_display.setFullWindow();
    m_display.firstPage();
    do
    {
        m_display.fillScreen(GxEPD_WHITE);
        addLines();
        writeMainPrice(fiat + formatPriceString(price));
        writeCrypto(crypto);
    }
    while (m_display.nextPage());
}

void DisplayManager::addLines()
{
    m_display.writeLine(0,       m_crypto_box_y,
                        m_max_x, m_crypto_box_y,
                        GxEPD_BLACK);
    m_display.writeLine(m_crypto_box_x,       0,
                        m_crypto_box_x, m_max_y, 
                        GxEPD_BLACK);
    m_display.writeLine(0,              77,
                        m_crypto_box_x, 77,
                        GxEPD_BLACK);
    m_display.fillRect(0,                           0,
                       m_crypto_box_x, m_crypto_box_y,
                       GxEPD_BLACK);
}

void DisplayManager::writeMainPrice(String price)
{
    m_display.setFont(&FreeSans18pt7b);
    m_display.setTextColor(GxEPD_BLACK);

    // centre the price in this region
    int16_t tbx, tby; uint16_t tbw, tbh;
    m_display.getTextBounds(price, 0, 0, &tbx, &tby, &tbw, &tbh);
    uint16_t x = (((m_max_x-m_crypto_box_x) - tbw) / 2) - tbx;
    uint16_t y = ((m_crypto_box_y - tbh) / 2) - tby;

    m_display.setCursor(x+m_crypto_box_x, y+1); // looked better moving down 1 more pixel
    m_display.print(price);
}

void DisplayManager::writeCrypto(String crypto)
{
    m_display.setFont(&FreeSans18pt7b);
    m_display.setTextColor(GxEPD_WHITE);

    // centre the crypto in this region
    int16_t tbx, tby; uint16_t tbw, tbh;
    m_display.getTextBounds(crypto, 0, 0, &tbx, &tby, &tbw, &tbh);
    uint16_t x = ((m_crypto_box_x - tbw) / 2) - tbx;
    uint16_t y = ((m_crypto_box_y - tbh) / 2) - tby;
    
    m_display.setCursor(x, y+1); // looked better moving down 1 more pixel
    m_display.print(crypto);
}

String DisplayManager::formatPriceString(float price)
{
    // crypto price can be anything from <1 to >10k
    // we want to write something neat like "12,345" or "0.1234"
    // if number >= 1000, lose decimals and add commas
    // if number < 1000, keep 2 decimals
    // if number < 10, keep 3 decimals
    // if number < 1, keep 4 decimals

    String formattedPrice;

    if (price < 1)
        formattedPrice = String(price, 4);
    else if (price < 10)
        formattedPrice = String(price, 3);
    else if (price < 1000)
        formattedPrice = String(price, 2);
    else
    {
        // >1000 needs formatting with commas
        char buf[21] = "";
        formatCommas(buf, (int)(price+0.5));
        formattedPrice = buf;
    }

    return formattedPrice;
}

void DisplayManager::formatCommas(char *buf, int price) {
    // recursively call dividing by 1000 until we get here to write the first digit(s) before first comma
    if (price < 1000) {
        sprintf(buf+strlen(buf), "%d", price);
        return;
    }
    formatCommas(buf, price / 1000);
    // continue writing a comma and next 3 digits as the recursions come back
    sprintf(buf+strlen(buf), ",%03d", price % 1000);
    return;
}

void DisplayManager::hibernate()
{
    delay(1000);
    m_display.hibernate();
}
