#include "DisplayManagerImpl.h"
#include "Utils.h"

#include "bitmaps.h"
#include "Constants.h"

#include "FreeSansBold24pt7b_edit.h"
#include <Fonts/FreeSans12pt7b.h>
#include <Fonts/FreeSansBold12pt7b.h>
#include <Fonts/FreeSans9pt7b.h>
#include <Fonts/FreeSansBold9pt7b.h>
#include <Fonts/FreeMonoBold12pt7b.h>
#include <Fonts/FreeMono9pt7b.h>
#include <Fonts/Org_01.h>


DisplayManagerImpl::DisplayManagerImpl(int rotation) :
    m_display(GxEPD2_213_BN(/*CS=5*/ SS, /*DC=*/ 17, /*RST=*/ 16, /*BUSY=*/ 4))
{
    m_display.init(115200, true, 2, false);
    m_display.setRotation(rotation);
};

void DisplayManagerImpl::writeDisplay(const String& crypto, const String& fiat, std::map<long, float>& priceData, const String& dayMonth, 
                                      const String& time, int batteryPercent)
{
    if (priceData.size() == 2 && priceData[0] != 0 && priceData[constants::SecondsOneDay] != 0)
    {
        writeDisplaySimple(crypto, fiat, priceData, dayMonth, time, batteryPercent);
    }
    else if (priceData.size() == 4 && priceData[0] != 0 || priceData[constants::SecondsOneDay] != 0 || 
        priceData[constants::SecondsOneMonth] != 0 || priceData[constants::SecondsOneYear] != 0)
    {
        writeDisplayAdvanced(crypto, fiat, priceData, dayMonth, time, batteryPercent);
    }
    else
    {
        log_w("No known display format for this data");
        writeGenericText("Unsupported data times requested");
        return;
    }
}

void DisplayManagerImpl::writeDisplayAdvanced(const String& crypto, const String& fiat, std::map<long, float>& priceData, const String& dayMonth, 
                                              const String& time, int batteryPercent)
{
    setCryptoBoxWidth(crypto, dayMonth, time);

    m_display.setFullWindow();
    m_display.firstPage();
    do
    {
        m_display.fillScreen(GxEPD_WHITE);
        addLines();
        fillCryptoBox();
        writeMainPriceAdvanced(m_fiatSymbols[fiat] + formatPriceString(priceData[0]));
        writeCrypto(crypto);
        writeDateTimeAdvanced(dayMonth, time);
        writeBattery(batteryPercent);

        // could try and split them by thirds but these offsets fit well
        drawArrow(writePriceChange(priceData[0], priceData[constants::SecondsOneDay], "1d", -6));
        writePriceChange(priceData[0], priceData[constants::SecondsOneMonth], "1M", 20);
        writePriceChange(priceData[0], priceData[constants::SecondsOneYear], "1Y", 46);
    }
    while (m_display.nextPage());
}

void DisplayManagerImpl::writeDisplaySimple(const String& crypto, const String& fiat, std::map<long, float>& priceData, const String& dayMonth, 
                                            const String& time, int batteryPercent)
{
    setCryptoBoxWidth(crypto, dayMonth, time, true);

    m_display.setFullWindow();
    m_display.firstPage();
    do
    {
        m_display.fillScreen(GxEPD_WHITE);
        fillCryptoBox(true);
        writeCrypto(crypto, true);
        writeMainPriceSimple(m_fiatSymbols[fiat] + formatPriceString(priceData[0]));
        writePriceChange(priceData[0], priceData[constants::SecondsOneDay], "1 day", 48, true);
        writeDateTimeSimple(dayMonth, time);
    }
    while (m_display.nextPage());
}

void DisplayManagerImpl::writeGenericText(const String& textToWrite)
{
    m_display.setFullWindow();
    m_display.firstPage();
    do
    {
        m_display.fillScreen(GxEPD_WHITE);

        m_display.setFont(&FreeSans9pt7b);
        m_display.setTextColor(GxEPD_BLACK);

        m_display.setCursor(2, 30);
        m_display.print(textToWrite);

    }
    while (m_display.nextPage());
}
void DisplayManagerImpl::hibernate()
{
    delay(200);
    m_display.hibernate();
}

void DisplayManagerImpl::drawCannotConnectToWifi(const String& ssid, const String& password)
{
    m_display.setFullWindow();
    m_display.firstPage();
    do
    {
        m_display.fillScreen(GxEPD_WHITE);

        m_display.drawBitmap(3, 5, epd_bitmap_no_wifi, 68, 60, GxEPD_BLACK);

        m_display.setTextColor(GxEPD_BLACK);
        m_display.setFont(&FreeSans12pt7b);
        m_display.setCursor(75, 30);
        m_display.print("Couldn't connect");
        m_display.setCursor(98, 58);
        m_display.print("to your WiFi");

        m_display.setFont(&FreeSans9pt7b);
        m_display.setCursor(5, 90);
        m_display.print("Name: ");
        m_display.print(ssid);
        m_display.setCursor(5, 113);
        m_display.print("Pass: ");
        m_display.print(password);

    }
    while (m_display.nextPage());
}

void DisplayManagerImpl::drawWifiHasNoInternet()
{
    m_display.setFullWindow();
    m_display.firstPage();
    do
    {
        m_display.fillScreen(GxEPD_WHITE);

        m_display.drawBitmap(5, 5, epd_bitmap_wifi_warn, 66, 60, GxEPD_BLACK);

        m_display.setTextColor(GxEPD_BLACK);
        m_display.setFont(&FreeSans12pt7b);
        m_display.setCursor(90, 25);
        m_display.print("WiFi couldn't");
        m_display.setCursor(110, 47);
        m_display.print("reach the");
        m_display.setCursor(120, 69);
        m_display.print("internet");

        m_display.setFont(&FreeSans9pt7b);
        m_display.setCursor(10, 100);
        m_display.print("Will retry after configured");
        m_display.setCursor(10, 117);
        m_display.print("refresh time");

    }
    while (m_display.nextPage());
}

void DisplayManagerImpl::drawLowBattery()
{
    m_display.setFullWindow();
    m_display.firstPage();
    do
    {
        m_display.fillScreen(GxEPD_WHITE);

        m_display.drawBitmap(10, 10, epd_bitmap_low_battery, 104, 60, GxEPD_BLACK);

        m_display.setTextColor(GxEPD_BLACK);
        m_display.setFont(&FreeSans18pt7b);
        m_display.setCursor(130, 30);
        m_display.print("Battery");
        m_display.setCursor(155, 65);
        m_display.print("low");

        m_display.setFont(&FreeSans9pt7b);
        m_display.setCursor(40, 110);
        m_display.print("Please charge device");

    }
    while (m_display.nextPage());
}

void DisplayManagerImpl::drawYesWifiNoCrypto(const String& dayMonth, const String& time)
{
    m_display.setFullWindow();
    m_display.firstPage();
    do
    {
        m_display.fillScreen(GxEPD_WHITE);

        m_display.drawBitmap(16, 5, epd_bitmap_yes_wifi_no_crypto, 212, 60, GxEPD_BLACK);

        m_display.setFont(&FreeSans9pt7b);
        m_display.setTextColor(GxEPD_BLACK);
        m_display.setCursor(5, 80);
        m_display.print("There was an error connecting");
        m_display.setCursor(5, 95);
        m_display.print("to the data source. Will retry...");
        m_display.setCursor(5, 115);
        m_display.print("(Last attempt @ ");
        m_display.print(dayMonth);
        m_display.print(" ");
        m_display.print(time);
        m_display.print(")");

    }
    while (m_display.nextPage());
}

void DisplayManagerImpl::drawConfig(const String& ssid, const String& password, const String& crypto, const String& fiat,
                                    int refreshInterval)
{
    const String topMsg = "Starting Ticker";
    m_display.setFullWindow();
    m_display.firstPage();
    do
    {
        m_display.fillScreen(GxEPD_WHITE);
        m_display.setTextColor(GxEPD_BLACK);


        m_display.setFont(&FreeSans12pt7b);
        m_display.setCursor(3, 25);
        m_display.print(topMsg);

        // write device id in corner
        m_display.setFont(&FreeMono9pt7b);
        m_display.setCursor(160, 25);
        m_display.print("(");
        m_display.print(utils::getDeviceID());
        m_display.print(")");

        m_display.writeLine(0,       32,
                            m_max_x, 32,
                            GxEPD_BLACK);

        m_display.setFont(&FreeSans9pt7b);
        m_display.setCursor(5, 51);
        m_display.print("Wifi: ");
        m_display.print(ssid);
        m_display.setCursor(5, 72);
        m_display.print("Pass: ");
        m_display.print(password);
        m_display.setCursor(5, 93);
        m_display.print("Crypto: ");
        m_display.print(crypto);
        m_display.print(", Fiat: ");
        m_display.print(fiat);
        m_display.setCursor(5, 114);
        m_display.print("Refresh Interval: ");
        m_display.print(refreshInterval);
        m_display.print(" mins");

    }
    while (m_display.nextPage());
    
}

void DisplayManagerImpl::drawAccessPoint(const String& ip)
{
    const char* browserMsg = "Open in browser:";

    m_display.setFullWindow();
    m_display.firstPage();
    do
    {
        m_display.fillScreen(GxEPD_WHITE);

        m_display.drawBitmap(8, 9, epd_bitmap_access_point, 234, 106, GxEPD_BLACK);

        m_display.setFont(&FreeSans12pt7b);
        m_display.setTextColor(GxEPD_BLACK);
        m_display.setCursor(80, 25);
        m_display.print("Configuration");
        m_display.writeLine(75,  30,
                            225, 30,
                            GxEPD_BLACK);

        m_display.setFont(&FreeSans12pt7b);
        // centre the ip in this region
        int16_t tbx, tby; uint16_t tbw, tbh;
        m_display.getTextBounds(ip, 0, 0, &tbx, &tby, &tbw, &tbh);
        uint16_t x = ((m_max_x - tbw) / 2) - tbx;
        m_display.setCursor(x, 110);
        m_display.print(ip);

        m_display.setFont(&FreeSans9pt7b);

        m_display.setCursor(70, 50);
        m_display.print("Join network: ");
        m_display.print(constants::WifiAccessPointName);

        // centre the ip in this region
        m_display.getTextBounds(browserMsg, 0, 0, &tbx, &tby, &tbw, &tbh);
        x = ((m_max_x - tbw) / 2) - tbx;
        m_display.setCursor(x, 83);
        m_display.print(browserMsg);

    }
    while (m_display.nextPage());
}

void DisplayManagerImpl::addLines()
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
}

void DisplayManagerImpl::fillCryptoBox(bool centre)
{
    m_display.fillRect(centre ? (m_max_x-m_crypto_box_x2)/2 : 0, 0,
                       m_crypto_box_x2, m_crypto_box_y2,
                       GxEPD_BLACK);
}

void DisplayManagerImpl::writeMainPriceAdvanced(const String& price)
{
    m_display.setFont(&FreeSans18pt7b);
    m_display.setTextColor(GxEPD_BLACK);

    // centre the price in this region
    int16_t tbx, tby; uint16_t tbw, tbh;
    m_display.getTextBounds(price, 0, 0, &tbx, &tby, &tbw, &tbh);
    uint16_t x = (((m_max_x-m_crypto_box_x2) - tbw) / 2) - tbx;
    uint16_t y = ((m_crypto_box_y2 - tbh) / 2) - tby;

    if (price.indexOf(",") != -1)
        y += 2; // comma goes below text, looks better moving it down
    m_display.setCursor(x+m_crypto_box_x2, y);
    m_display.print(price);
}

void DisplayManagerImpl::writeMainPriceSimple(const String& price)
{
    m_display.setFont(&FreeSansBold24pt7b);
    m_display.setTextColor(GxEPD_BLACK);

    // centre the price in this region
    int16_t tbx, tby; uint16_t tbw, tbh;
    m_display.getTextBounds(price, 0, 0, &tbx, &tby, &tbw, &tbh);
    uint16_t x = ((m_max_x - tbw) / 2) - tbx;
    uint16_t y = 82;

    m_display.setCursor(x, y);
    m_display.print(price);
}

void DisplayManagerImpl::writeCrypto(const String& crypto, bool centre)
{
    m_display.setFont(m_current_crypto_box_font);
    m_display.setTextColor(GxEPD_WHITE);

    if (centre)
    {
        // centre the crypto in this region
        int16_t tbx, tby; uint16_t tbw, tbh;
        m_display.getTextBounds(crypto, 0, 0, &tbx, &tby, &tbw, &tbh);
        uint16_t x = ((m_max_x - tbw) / 2) - tbx;
        uint16_t y = ((m_crypto_box_y2 - tbh) / 2) - tby;
        
        m_display.setCursor(x, y+1); // looked better moving down 1 more pixel
    }
    else
    {
        // centre the crypto in this region
        int16_t tbx, tby; uint16_t tbw, tbh;
        m_display.getTextBounds(crypto, 0, 0, &tbx, &tby, &tbw, &tbh);
        uint16_t x = ((m_crypto_box_x2 - tbw) / 2) - tbx;
        uint16_t y = ((m_crypto_box_y2 - tbh) / 2) - tby;
        
        m_display.setCursor(x, y+1); // looked better moving down 1 more pixel
    }
    m_display.print(crypto);
}

void DisplayManagerImpl::writeDateTimeAdvanced(const String& dayMonth, const String& time)
{
    m_display.setFont(&FreeSans9pt7b);
    m_display.setTextColor(GxEPD_BLACK);

    // centre the day/month in this region
    int16_t tbx, tby; uint16_t tbw, tbh;
    m_display.getTextBounds(dayMonth, 0, 0, &tbx, &tby, &tbw, &tbh);
    uint16_t x = (((m_crypto_box_x2-m_date_box_x1) - tbw) / 2) - tbx;
    uint16_t y = (((m_date_box_y2-m_date_box_y1) - tbh) / 2) - tby;

    // but move 10 px higher
    m_display.setCursor(x+m_date_box_x1, y+m_date_box_y1-9);
    m_display.print(dayMonth);

    // centre the day/month in this region
    m_display.getTextBounds(time, 0, 0, &tbx, &tby, &tbw, &tbh);
    x = (((m_crypto_box_x2-m_date_box_x1) - tbw) / 2) - tbx;
    y = (((m_date_box_y2-m_date_box_y1) - tbh) / 2) - tby;

    // and move this 10 px lower
    m_display.setCursor(x+m_date_box_x1, y+m_date_box_y1+11);
    m_display.print(time);
}

void DisplayManagerImpl::writeDateTimeSimple(const String& dayMonth, const String& time)
{
    m_display.setFont(&FreeSansBold9pt7b);
    m_display.setTextColor(GxEPD_BLACK);

    // centre the day/month in this region (m_max_x-m_crypto_box_x2)/2
    int16_t tbx, tby; uint16_t tbw, tbh;
    m_display.getTextBounds(dayMonth, 0, 0, &tbx, &tby, &tbw, &tbh);
    uint16_t x = ((((m_max_x-m_crypto_box_x2)/2) - tbw) / 2) - tbx;
    uint16_t y = ((m_crypto_box_y2 - tbh) / 2) - tby;

    m_display.setCursor(x, y);
    m_display.print(dayMonth);

    m_display.getTextBounds(time, 0, 0, &tbx, &tby, &tbw, &tbh);
    x = ((((m_max_x-m_crypto_box_x2)/2) - tbw) / 2) - tbx;
    y = ((m_crypto_box_y2 - tbh) / 2) - tby;

    // and move this 10 px lower
    m_display.setCursor(x+((m_max_x-m_crypto_box_x2)/2)+m_crypto_box_x2, y);
    m_display.print(time);
}

void DisplayManagerImpl::writeBattery(int batPct)
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

bool DisplayManagerImpl::writePriceChange(float mainPrice, float priceToCompare, const String& timeframe, int yOffset, bool centre)
{
    if (centre)
        m_display.setFont(&FreeSansBold12pt7b);
    else
        m_display.setFont(&FreeMonoBold12pt7b);
    m_display.setTextColor(GxEPD_BLACK);

    float percentChange = ((mainPrice - priceToCompare) / mainPrice) * 100;

    String changeLine = formatPriceChangeString(percentChange, timeframe);

    // centre the change in this region
    int16_t tbx, tby; uint16_t tbw, tbh;
    m_display.getTextBounds(changeLine, 0, 0, &tbx, &tby, &tbw, &tbh);
    uint16_t x;
    if (centre)
        x = ((m_max_x - tbw) / 2) - tbx;
    else
        x = (((m_max_x-m_crypto_box_x2) - tbw) / 2) - tbx;
    uint16_t y = ((m_max_y - tbh) / 2) - tby;

    m_display.setCursor(x+(centre ? 0 : m_crypto_box_x2), y+yOffset);
    m_display.print(changeLine);

    return percentChange >= 0;
}

void DisplayManagerImpl::drawArrow(bool isPositive)
{
    uint16_t midPointX = m_crypto_box_x2 / 2;
    if (isPositive)
    {
        m_display.fillTriangle(midPointX-16, 70,
                               midPointX+16, 70,
                               midPointX,    45,
                               GxEPD_BLACK);
        m_display.fillTriangle(midPointX-16, 70,
                               midPointX+16, 70,
                               midPointX,    64,
                               GxEPD_WHITE);
    }
    else
    {
        m_display.fillTriangle(midPointX-16, 45,
                               midPointX+16, 45,
                               midPointX,    70,
                               GxEPD_BLACK);
        m_display.fillTriangle(midPointX-16, 45,
                               midPointX+16, 45,
                               midPointX,    51,
                               GxEPD_WHITE);
    }
}

void DisplayManagerImpl::setCryptoBoxWidth(const String& crypto, const String& dayMonth, const String& time, bool centre)
{
    // calculate the width that the crypto box should be, based on the widest of the symbol/date/time   
    uint16_t m_max_allowed_crypto_box_width = 0;
    if (centre)
        m_max_allowed_crypto_box_width = m_max_allowed_crypto_box_width_simple;
    else
        m_max_allowed_crypto_box_width = m_max_allowed_crypto_box_width_advanced;

    // crypto symbol
    m_display.setFont(m_current_crypto_box_font);
    int16_t tbx, tby; uint16_t tbw, tbh;
    m_display.getTextBounds(crypto, 0, 0, &tbx, &tby, &tbw, &tbh);
    uint16_t cryptoWidth = (2 * m_min_crypto_padding) + tbw;
    log_d("cryptoWidth = %d", cryptoWidth);
    
    // date
    m_display.setFont(&FreeSans9pt7b);
    m_display.getTextBounds(dayMonth, 0, 0, &tbx, &tby, &tbw, &tbh);
    uint16_t dateWidth = (2 * m_min_date_padding) + tbw;
    log_d("dateWidth1 = %d", dateWidth);

    // time
    m_display.getTextBounds(time, 0, 0, &tbx, &tby, &tbw, &tbh);
    if (((2 * m_min_date_padding) + tbw) > dateWidth)
        dateWidth = (2 * m_min_date_padding) + tbw;
    log_d("dateWidth2 = %d", dateWidth);

    dateWidth += m_date_box_x1;
    log_d("dateWidth3 = %d", dateWidth);
    uint16_t maxWidth = cryptoWidth > dateWidth ? cryptoWidth : dateWidth;

    log_d("Max width = %d", maxWidth);

    if (maxWidth > m_max_allowed_crypto_box_width && m_current_crypto_box_font == m_default_crypto_box_font)
    {
        log_d("Set font to 12pt and try again");
        m_current_crypto_box_font = &FreeSans12pt7b;
        setCryptoBoxWidth(crypto, dayMonth, time);
        return;
    }
    else if (maxWidth > m_max_allowed_crypto_box_width && m_current_crypto_box_font != &FreeSans9pt7b)
    {
        log_d("Set font to 9pt and try again");
        m_current_crypto_box_font = &FreeSans9pt7b;
        setCryptoBoxWidth(crypto, dayMonth, time);
        return;
    }
    else if (maxWidth > m_max_allowed_crypto_box_width)
    {
        // no symbol should be still longer then the max at 9pt
        log_w("Very long symbol/date/time (%s/%s/%s) cannot fit within crypto box, max width of these is %d", 
              crypto.c_str(), dayMonth.c_str(), time.c_str(), maxWidth);
    }

    if (maxWidth < m_min_allowed_crypto_box_width)
        m_crypto_box_x2 = m_min_allowed_crypto_box_width;
    else if (maxWidth > m_max_allowed_crypto_box_width)
        m_crypto_box_x2 = m_max_allowed_crypto_box_width;
    else
        m_crypto_box_x2 = maxWidth;
}

String DisplayManagerImpl::formatPriceString(float price)
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

String DisplayManagerImpl::formatPriceChangeString(float percentChange, const String& timeframe)
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

void DisplayManagerImpl::formatCommas(char *buf, int price)
{
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
