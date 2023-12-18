#include "Utils.h"
#include "Constants.h"

#include "TickerCoordinator.h"

#define BUTTON_PIN 39

SET_LOOP_TASK_STACK_SIZE(16*1024);

RTC_DATA_ATTR int bootCount = 0;

hw_timer_t *alert_timer = NULL;

void IRAM_ATTR onTimer()
{
    // failsafe to reboot if taking too long
    // If some wifi request gets stuck it can either take a very long time or never recover.
    // It would be a bad situation for the device to just be sitting there draining battery.
    // This timer will get triggered regardless of what the program is doing, and will reboot.
    // Enable at start, disable after all requests have gone through and screen update starts.
    log_w("Alert triggered, forcing deep sleep for 30 seconds");
    utils::ticker_deep_sleep(30 * constants::MicrosToSecondsFactor);
    // deep sleep so we can pause then recover without printing a startup screen
    // maybe add some kind of logging to SPIFFS to warn if this is happening too frequently
}

// The Arduino framework internally defines the main() function, which
// calls setup() once and loop() on repeat
// Tricky to remove this behaviour, so just treat setup() as this 
// program's main() and leave loop() blank

void setup() 
{
    // must get battery as first thing
    int batPct = utils::battery_percent(utils::battery_read());
    ++bootCount;

    uint32_t startTime = millis();
    Serial.begin(115200);
    delay(200);
    
    String wifiMac = utils::getDeviceID();

    log_i("Program started");
    log_i("MAC ID: %s", wifiMac.c_str());

    pinMode(BUTTON_PIN, INPUT);
    bool shouldEnterConfig = !digitalRead(BUTTON_PIN); // high = not pressed, low = pressed
    log_d("should enter config = %d", shouldEnterConfig);

    // timer setup
    alert_timer = timerBegin(0, 80, true);
    timerAttachInterrupt(alert_timer, &onTimer, true);
    if (shouldEnterConfig)
        timerAlarmWrite(alert_timer, constants::ConfigAlertTimeSeconds * constants::MicrosToSecondsFactor, true);
    else
        timerAlarmWrite(alert_timer, constants::NormalAlertTimeSeconds * constants::MicrosToSecondsFactor, true);
    timerAlarmEnable(alert_timer); 

    TickerCoordinator ticker(batPct, shouldEnterConfig);
    int refreshSeconds = ticker.run();

    log_i("Program awake time: %d", millis() - startTime);
    // start deep sleep
    log_d("Starting deep sleep for %d seconds", refreshSeconds);
    Serial.flush();
    utils::ticker_deep_sleep(refreshSeconds * constants::MicrosToSecondsFactor);
}

void loop() {}
