#include "DisplayManager.h"

#include <Fonts/FreeSans18pt7b.h>
#include <Fonts/FreeSans9pt7b.h>
#include <Fonts/FreeMonoBold12pt7b.h>
#include <Fonts/Org_01.h>

DisplayManager::DisplayManager() :
    m_display(GxEPD2_213_BN(/*CS=5*/ SS, /*DC=*/ 17, /*RST=*/ 16, /*BUSY=*/ 4))
{
    m_display.init(115200, true, 2, false);
    m_display.setRotation(1);
}

void DisplayManager::writeDisplay(const String& crypto, const String& fiat, float mainPrice, float priceOneDay, 
                                  float priceOneMonth, float priceOneYear, const String& dayMonth, const String& time, 
                                  int batteryPercent)
{    
    m_display.setFullWindow();
    m_display.firstPage();
    do
    {
        m_display.fillScreen(GxEPD_WHITE);
        addLines();
        writeMainPrice(fiat + formatPriceString(mainPrice)); // need to find a way to add £/€ symbol
        writeCrypto(crypto);
        writeDateTime(dayMonth, time);
        writeBattery(batteryPercent);

        // could try and split them by thirds but these offsets fit well
        drawArrow(writePriceChange(mainPrice, priceOneDay, "1d", -6));
        writePriceChange(mainPrice, priceOneMonth, "1M", 20);
        writePriceChange(mainPrice, priceOneYear, "1Y", 46);
    }
    while (m_display.nextPage());
}

void DisplayManager::addLines()
{
    m_display.writeLine(0,       m_crypto_box_y2,
                        m_max_x, m_crypto_box_y2,
                        GxEPD_BLACK);
    m_display.writeLine(m_crypto_box_x2,       0,
                        m_crypto_box_x2, m_max_y, 
                        GxEPD_BLACK);
    m_display.writeLine(0,               m_date_box_y1,
                        m_crypto_box_x2, m_date_box_y1,
                        GxEPD_BLACK);
    m_display.writeLine(m_date_box_x1,   m_date_box_y1,
                        m_date_box_x1,   m_max_y,
                        GxEPD_BLACK);
    m_display.writeLine(0,             m_bat_box_y1+10,
                        m_date_box_x1, m_bat_box_y1+10,
                        GxEPD_BLACK);
    m_display.fillRect(0,                            0,
                       m_crypto_box_x2, m_crypto_box_y2,
                       GxEPD_BLACK);
}

void DisplayManager::writeMainPrice(const String& price)
{
    m_display.setFont(&FreeSans18pt7b);
    m_display.setTextColor(GxEPD_BLACK);

    // centre the price in this region
    int16_t tbx, tby; uint16_t tbw, tbh;
    m_display.getTextBounds(price, 0, 0, &tbx, &tby, &tbw, &tbh);
    uint16_t x = (((m_max_x-m_crypto_box_x2) - tbw) / 2) - tbx;
    uint16_t y = ((m_crypto_box_y2 - tbh) / 2) - tby;

    m_display.setCursor(x+m_crypto_box_x2, y+1); // looked better moving down 1 more pixel
    m_display.print(price);
}

bool DisplayManager::writePriceChange(float mainPrice, float priceToCompare, const String& timeframe, int yOffset)
{
    m_display.setFont(&FreeMonoBold12pt7b);
    m_display.setTextColor(GxEPD_BLACK);

    float percentChange = ((mainPrice - priceToCompare) / mainPrice) * 100;

    String changeLine = formatPriceChangeString(percentChange, timeframe);

    // centre the change in this region
    int16_t tbx, tby; uint16_t tbw, tbh;
    m_display.getTextBounds(changeLine, 0, 0, &tbx, &tby, &tbw, &tbh);
    uint16_t x = (((m_max_x-m_crypto_box_x2) - tbw) / 2) - tbx;
    uint16_t y = ((m_max_y - tbh) / 2) - tby;

    m_display.setCursor(x+m_crypto_box_x2, y+yOffset);
    m_display.print(changeLine);

    return percentChange >= 0;
}

void DisplayManager::writeCrypto(const String& crypto)
{
    m_display.setFont(&FreeSans18pt7b);
    m_display.setTextColor(GxEPD_WHITE);

    // centre the crypto in this region
    int16_t tbx, tby; uint16_t tbw, tbh;
    m_display.getTextBounds(crypto, 0, 0, &tbx, &tby, &tbw, &tbh);
    uint16_t x = ((m_crypto_box_x2 - tbw) / 2) - tbx;
    uint16_t y = ((m_crypto_box_y2 - tbh) / 2) - tby;
    
    m_display.setCursor(x, y+1); // looked better moving down 1 more pixel
    m_display.print(crypto);
}

void DisplayManager::drawArrow(bool isPositive)
{
    // todo define this in the centre of the box
    if (isPositive)
    {
        m_display.fillTriangle(27, 70,
                               60, 70,
                               43, 45,
                               GxEPD_BLACK);
        m_display.fillTriangle(27, 70,
                               60, 70,
                               43, 64,
                               GxEPD_WHITE);
    }
    else
    {
        m_display.fillTriangle(27, 45,
                               60, 45,
                               43, 70,
                               GxEPD_BLACK);
        m_display.fillTriangle(27, 45,
                               60, 45,
                               43, 51,
                               GxEPD_WHITE);
    }
}

void DisplayManager::writeDateTime(const String& dayMonth, const String& time)
{
    m_display.setFont(&FreeSans9pt7b);
    m_display.setTextColor(GxEPD_BLACK);

    // centre the day/month in this region
    int16_t tbx, tby; uint16_t tbw, tbh;
    m_display.getTextBounds(dayMonth, 0, 0, &tbx, &tby, &tbw, &tbh);
    uint16_t x = (((m_date_box_x2-m_date_box_x1) - tbw) / 2) - tbx;
    uint16_t y = (((m_date_box_y2-m_date_box_y1) - tbh) / 2) - tby;

    // but move 10 px higher
    m_display.setCursor(x+m_date_box_x1, y+m_date_box_y1-9);
    m_display.print(dayMonth);

    // centre the day/month in this region
    m_display.getTextBounds(time, 0, 0, &tbx, &tby, &tbw, &tbh);
    x = (((m_date_box_x2-m_date_box_x1) - tbw) / 2) - tbx;
    y = (((m_date_box_y2-m_date_box_y1) - tbh) / 2) - tby;

    // and move this 10 px lower
    m_display.setCursor(x+m_date_box_x1, y+m_date_box_y1+11);
    m_display.print(time);
}

void DisplayManager::writeBattery(int batPct)
{
    // write number
    m_display.setFont(&Org_01);
    m_display.setTextColor(GxEPD_BLACK);
    
    int maxHeight = m_max_y - m_bat_box_y1 - 10;

    // centre the battery percent in this region
    // this gets quite confusing, needs better assignment of regions
    // it does all line up nicely though
    int16_t tbx, tby; uint16_t tbw, tbh;
    m_display.getTextBounds(String(batPct), 0, 0, &tbx, &tby, &tbw, &tbh);
    uint16_t x = ((m_bat_box_x2 - tbw) / 2) - tbx;
    uint16_t y = (((m_bat_box_y1+(m_max_y-maxHeight)) - tbh) / 2) - tby;

    m_display.setCursor(x, y+1);
    m_display.print(batPct);

    // draw box
    float drawHeight = (((float)batPct / 100) * maxHeight);
    int drawHeightInt = (int)(drawHeight+0.5);
    m_display.fillRect(0,             m_max_y-drawHeightInt,
                       m_date_box_x1, m_max_y,
                       GxEPD_BLACK);


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

String DisplayManager::formatPriceChangeString(float percentChange, const String& timeframe)
{
    // for price change we want constant width
    // e.g. 1d: +1.23%
    //      1M: +12.3%
    //      1Y: + 123%
    //      1d: +1234%

    String changeLine = timeframe + ": ";
    if (percentChange >= 0)
        changeLine.concat("+");
    else
        changeLine.concat("-");

    percentChange = abs(percentChange); // work with +ve for rounding, +/- already appended

    if (percentChange < 10)
        changeLine.concat(String(percentChange, 2));
    else if (percentChange < 100)
        changeLine.concat(String(percentChange, 1));
    else if (percentChange < 1000)
        changeLine.concat(" " + String(percentChange, 0));
    else
        changeLine.concat(String(percentChange, 0));

    changeLine.concat("%");

    return changeLine;
}

void DisplayManager::hibernate()
{
    delay(1000);
    m_display.hibernate();
}
