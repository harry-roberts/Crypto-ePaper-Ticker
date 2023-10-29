#include "WiFiManager.h"
#include "DisplayManager.h"
#include "Login.h"
#include "Utils.h"

#define uS_TO_S_FACTOR 1000000
#define DEEP_SLEEP_TIME_S  60

SET_LOOP_TASK_STACK_SIZE(16*1024);

RTC_DATA_ATTR int bootCount = 0;

// The Arduino framework internally defines the main() function, which
// calls setup() once and loop() on repeat
// Tricky to remove this behaviour, so just treat setup() as this 
// program's main() and leave loop() blank

void setup() 
{
    ++bootCount;

    int batPct = utils::battery_percent(utils::battery_read());

    uint32_t startTime = millis();
    Serial.begin(115200);
    delay(200);
    Serial.println("Program started");

    String crypto = "BTC";
    String fiat = "USD";
    
    WiFiManager wm(login_ssid, login_password);
    DisplayManager display;
    delay(1000);

    int retries = 0;

    float price, priceOneDay, priceOneMonth, priceOneYear;
    bool hasPrice = false;
    while(!hasPrice && retries < 5) // in case of failure retry
    {
        hasPrice = wm.getCurrentPrice(crypto, fiat, price);
        delay(1000);
        retries++;
    }

    bool hasPriceOneDay = false;
    retries = 0;
    while(!hasPriceOneDay && retries < 2) // in case of failure retry
    {
        hasPriceOneDay = wm.getPriceAtTime(crypto, fiat, 86400, priceOneDay);
        delay(1000);
        retries++;
    }

    bool hasPriceOneMonth = false;
    retries = 0;
    while(!hasPriceOneMonth && retries < 2) // in case of failure retry
    {
        hasPriceOneMonth = wm.getPriceAtTime(crypto, fiat, 2592000, priceOneMonth);
        delay(1000);
        retries++;
    }

    bool hasPriceOneYear = false;
    retries = 0;
    while(!hasPriceOneYear && retries < 2) // in case of failure retry
    {
        hasPriceOneYear = wm.getPriceAtTime(crypto, fiat, 31536000, priceOneYear);
        delay(1000);
        retries++;
    }

    if (!hasPrice || !hasPriceOneDay || !hasPriceOneMonth || !hasPriceOneYear)
    {
        // will need a proper fail screen/message
        display.writeDisplay("Error", "", 0, 0, 0, 0, wm.getDayMonthStr(), wm.getTimeStr(), batPct); 
    }
    else
    {
        display.writeDisplay(crypto, "$", price, priceOneDay, priceOneMonth, priceOneYear, wm.getDayMonthStr(), wm.getTimeStr(), batPct); // need to find a way to add £/€ symbol
    }
    display.hibernate();
    
    Serial.println("Starting deep sleep");
    Serial.print("Program awake time: ");
    Serial.println(millis() - startTime);
    Serial.flush();

    // start deep sleep
    utils::ticker_deep_sleep(DEEP_SLEEP_TIME_S * uS_TO_S_FACTOR);
}

void loop() {}
