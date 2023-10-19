#include "WiFiManager.h"
#include "Login.h"

#define uS_TO_S_FACTOR 1000000
#define DEEP_SLEEP_TIME_S  30

// The Arduino framework internally defines the main() function, which
// calls setup() once and loop() on repeat
// Tricky to remove this behaviour, so just treat setup() as this 
// program's main() and leave loop() blank

void setup() 
{
    Serial.begin(115200);
    delay(200);
    Serial.println("Program started");

    esp_sleep_enable_timer_wakeup(DEEP_SLEEP_TIME_S * uS_TO_S_FACTOR);
}

void loop() 
{
    WiFiManager wm(login_ssid, login_password);

    Serial.println(wm.getCryptoPrice("BTC"));
    delay(100);
    Serial.println(wm.getCryptoPrice("ETH"));
    delay(100);
    Serial.println(wm.getCryptoPrice("ADA"));

    Serial.println("Starting deep sleep");
    Serial.flush();
    esp_deep_sleep_start(); 

    // measured deep sleep current at ~255-260uA on 9102 version
    // version without 9102 chip much higher for some reason, ~700uA
    // active current quite spiky, 50-150mA
}
