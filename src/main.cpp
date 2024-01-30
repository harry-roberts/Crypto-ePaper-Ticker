#include "Utils.h"
#include "Constants.h"

#include "TickerCoordinator.h"

#include "esp_sntp.h"

#define BUTTON_PIN 39

SET_LOOP_TASK_STACK_SIZE(16*1024);

RTC_DATA_ATTR int bootCount = 0;
RTC_DATA_ATTR int wifiFails = 0;
RTC_DATA_ATTR int dataFails = 0;

RTC_DATA_ATTR int numberOfOvernightSleepPeriodsLeft = 0; // how many individual deep sleeps left in overnight sleep
RTC_DATA_ATTR int overnightSleepPeriodLength = 0;        // length of each sleep during overnight sleep (seconds)
RTC_DATA_ATTR bool waitForNtpSync = false;               // after a long sleep time we want to resync before using the time

hw_timer_t *alert_timer = NULL;

void time_sync_notification_cb(struct timeval *tv) {
    log_d("NTP SYNC");
}

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
    Serial.begin(115200); 
    // must get battery as first thing
    int batPct = utils::battery_percent(utils::battery_read());
    ++bootCount;

    sntp_set_time_sync_notification_cb(time_sync_notification_cb);

    // check for overnight sleeps
    // in the case of too low battery then ditch the overnight sleep idea and let the screen update 
    // the display with the warning normally
    if (batPct >= constants::MinimumAllowedBatteryPercent)
    {
        if (numberOfOvernightSleepPeriodsLeft > 0)
        {
            // perform another required sleep period
            numberOfOvernightSleepPeriodsLeft--;
            log_d("Overnight sleeping for %d seconds, with %d periods left after this", 
                  overnightSleepPeriodLength, 
                  numberOfOvernightSleepPeriodsLeft);
            if (numberOfOvernightSleepPeriodsLeft == 0)
            {
                log_d("Ticker will wait for NTP on next reboot");
                waitForNtpSync = true;
            }
            utils::ticker_deep_sleep((uint64_t)overnightSleepPeriodLength * constants::MicrosToSecondsFactor);
        }
        
    }

    uint32_t startTime = millis();
    delay(200);
    
    String wifiMac = utils::getDeviceID();

    log_i("Program started");
    log_i("MAC ID: %s", wifiMac.c_str());
    log_i("Firmware version: %s", constants::VersionNumber);

    pinMode(BUTTON_PIN, INPUT);
    bool shouldEnterConfig = !digitalRead(BUTTON_PIN); // high = not pressed, low = pressed
    log_d("should enter config = %d", shouldEnterConfig);

    // timer setup
    alert_timer = timerBegin(0, 80, true);
    timerAttachInterrupt(alert_timer, &onTimer, true); 

    TickerInput tickerInput{batPct, shouldEnterConfig, wifiFails, dataFails, bootCount, waitForNtpSync, alert_timer};
    if (waitForNtpSync)
        waitForNtpSync = false; // only do it once

    TickerCoordinator ticker(tickerInput);

    TickerOutput tickerOutput = ticker.run();

    if (tickerOutput.wifiFailed)
        wifiFails++;
    else
        wifiFails = 0;

    if (tickerOutput.dataFailed)
        dataFails++;
    else
        dataFails = 0;

    // if overnight sleep value returned, do an overnight sleep period
    // max deep sleep time of ESP is ~1h10m (unsigned 32 bit number of microseconds)
    // the internal clock of the ESP isn't great - can be out by ~20 seconds per hour
    // fine for a single 1 hour sleep but to get it more accurate at the end of the sleep we will aim
    // to finish a chain of sleeps with 10 minutes remaining, then after resyncing the time with NTP,
    // the last sleep will be pretty close to the requested end time. 
    // with a max of 1 hour individual sleep length, calculate minimum number required to get to 10 mins
    // left of total sleep time, then divide them evenly
    // E.g. for 100 mins overnight sleep, to get to 10 mins left we have 90 mins, need 2 sleeps of 45 mins
    if (tickerOutput.secondsLeftOfSleep > 0)
    {
        // should either be many hours, or ~10 mins (call it under 1 hour)
        log_d("Ticker should be in overnight sleep with %" PRIu64 " seconds left", tickerOutput.secondsLeftOfSleep);
        if (tickerOutput.secondsLeftOfSleep < 3600) // 1 hour, can just sleep this then continue normally
        {
            log_d("Final sleep, resetting overnight sleeps");   
            numberOfOvernightSleepPeriodsLeft = 0;
            overnightSleepPeriodLength = 0;
            utils::ticker_deep_sleep((uint64_t)tickerOutput.secondsLeftOfSleep * constants::MicrosToSecondsFactor);
        }
        
        int secondsUntilTenMinutesRemaining = tickerOutput.secondsLeftOfSleep - 600;
        int minNumberOfSleeps = secondsUntilTenMinutesRemaining / 3600;
        if (secondsUntilTenMinutesRemaining % 3600 > 0)
            minNumberOfSleeps++;

        overnightSleepPeriodLength = secondsUntilTenMinutesRemaining / minNumberOfSleeps; // close enough
        numberOfOvernightSleepPeriodsLeft = minNumberOfSleeps - 1; // -1 as we are about to do one of them

        log_d("Overnight sleeping for %d seconds, with %d periods left after this", 
              overnightSleepPeriodLength, 
              numberOfOvernightSleepPeriodsLeft);
        delay(100);
        utils::ticker_deep_sleep((uint64_t)overnightSleepPeriodLength * constants::MicrosToSecondsFactor);
    }

    log_i("Program awake time: %d", millis() - startTime);
    // start deep sleep
    log_d("Starting deep sleep for %d seconds", tickerOutput.refreshSeconds);
    Serial.flush();
    utils::ticker_deep_sleep((uint64_t)tickerOutput.refreshSeconds * constants::MicrosToSecondsFactor);
}

void loop() {}
