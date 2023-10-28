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
    
    WiFiManager wm(login_ssid, login_password);
    DisplayManager display;
    delay(1000);

    float price = -1;
    int retries = 0;
    do
    {
        price = wm.getCurrentPrice("BTC", "USD");
        delay(1000);
        retries++;
    }
    while(price < 0 && retries < 5); // in case of failure retry

    if (price < 0)
    {
        display.writeDisplay("Error", "", 0, 0, 0, 0, "12 Oct", "12:34", batPct); 
    }
    else
    {
        display.writeDisplay("BTC", "$", price, 0, 0, 0, "12 Oct", "12:34", batPct); // need to find a way to add £/€ symbol
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
