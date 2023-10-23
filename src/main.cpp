#include "WiFiManager.h"
#include "DisplayManager.h"
#include "Login.h"

#define uS_TO_S_FACTOR 1000000
#define DEEP_SLEEP_TIME_S  300

SET_LOOP_TASK_STACK_SIZE(16*1024);

RTC_DATA_ATTR int bootCount = 0;

float rawVoltage() {
    uint16_t reading1 = analogRead(35);
    delay(10);
    uint16_t reading2 = analogRead(35);
    delay(10);
    uint16_t reading3 = analogRead(35);

    float avg = ((float)reading1 + reading2 + reading3) / 3;
    float battery_voltage = avg / 4095 * 3.3;
    return battery_voltage;
}

double convertVoltage(float volt) {
  return (100 + 100) / 100  * volt;
}

double volt_read(){
  return convertVoltage( rawVoltage());
}

// The Arduino framework internally defines the main() function, which
// calls setup() once and loop() on repeat
// Tricky to remove this behaviour, so just treat setup() as this 
// program's main() and leave loop() blank

void setup() 
{
    // temporary battery voltage monitoring
    String bat(volt_read());
    bat.concat("V");

    ++bootCount;
    bat.concat(",");
    bat.concat(bootCount);

    uint32_t startTime = millis();
    Serial.begin(115200);
    delay(200);
    Serial.println("Program started");

    esp_sleep_enable_timer_wakeup(DEEP_SLEEP_TIME_S * uS_TO_S_FACTOR);

    Serial.print(millis());
    
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
