#include "WiFiManager.h"
#include "DisplayManager.h"
#include "Login.h"
#include "Utils.h"

#define uS_TO_S_FACTOR 1000000
#define DEEP_SLEEP_TIME_S  300

SET_LOOP_TASK_STACK_SIZE(16*1024);

RTC_DATA_ATTR int bootCount = 0;



// The Arduino framework internally defines the main() function, which
// calls setup() once and loop() on repeat
// Tricky to remove this behaviour, so just treat setup() as this 
// program's main() and leave loop() blank

void setup() 
{
    // temporary battery voltage monitoring
    String bat(utils::battery_read(), 3);
    bat.concat("V");

    ++bootCount;
    bat.concat(",");
    bat.concat(bootCount);

    uint32_t startTime = millis();
    Serial.begin(115200);
    delay(200);
    Serial.println("Program started");
    
    WiFiManager wm(login_ssid, login_password);
    DisplayManager display;
    delay(1000);

    float price = -1;
    int retries = 0;
    do
    {
        price = wm.getCurrentPrice("BTC", "USD");
        if (price > 0)
            display.writePriceDisplay(price, "BTC", "$", bat); // need to find a way to add £/€ symbol
        delay(1000);
        retries++;
    }
    while(price < 0 && retries < 5); // in case of failure retry

    if (price < 0)
    {
        display.writePriceDisplay(0, "Error", "", bat); // will need a proper fail screen/message
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
