#include "WiFiManager.h"
#include "DisplayManager.h"
#include "Login.h"

#define uS_TO_S_FACTOR 1000000
#define DEEP_SLEEP_TIME_S  300

SET_LOOP_TASK_STACK_SIZE(16*1024);

// The Arduino framework internally defines the main() function, which
// calls setup() once and loop() on repeat
// Tricky to remove this behaviour, so just treat setup() as this 
// program's main() and leave loop() blank

void setup() 
{
    uint32_t startTime = millis();
    Serial.begin(115200);
    delay(200);
    Serial.println("Program started");

    esp_sleep_enable_timer_wakeup(DEEP_SLEEP_TIME_S * uS_TO_S_FACTOR);

    Serial.print(millis());
    Serial.println("\tcreating DisplayManager");
    
    WiFiManager wm(login_ssid, login_password);
    DisplayManager display;
    delay(1000);

    float price = -1;
    int retries = 0;
    do
    {
        price = wm.getCryptoPrice("BTC");
        if (price > 0)
            display.writePriceDisplay(price, "BTC", "$"); // need to find a way to add £ symbol
        delay(1000);
        retries++;
    }
    while(price < 0 && retries < 5); // in case of failure retry

    if (price < 0)
    {
        display.writePriceDisplay(0, "Error", ""); // will need a proper fail screen/message
    }
    
    Serial.println("Starting deep sleep");
    display.hibernate();
    Serial.print("Program awake time: ");
    Serial.println(millis() - startTime);
    Serial.flush();
    
    esp_deep_sleep_start(); 

    // measured deep sleep current at ~255-260uA on 9102 version
    // version without 9102 chip much higher for some reason, ~700uA
    // active current quite spiky, 50-150mA
}

void loop() {}
