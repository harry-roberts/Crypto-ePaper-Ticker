#include "WiFiManager.h"
#include "DisplayManager.h"
#include "Utils.h"
#include "Constants.h"

#include "SPIFFS.h"
#include <ArduinoJson.h>

#define uS_TO_S_FACTOR 1000000
#define ALERT_TIME_S  150

#define BUTTON_PIN 39

SET_LOOP_TASK_STACK_SIZE(16*1024);

RTC_DATA_ATTR int bootCount = 0;

hw_timer_t *alert_timer = NULL;

using utils::CurrentConfig;

void IRAM_ATTR onTimer()
{
    // failsafe to reboot if taking too long
    // If some wifi request gets stuck it can either take a very long time or never recover.
    // It would be a bad situation for the device to just be sitting there draining battery.
    // This timer will get triggered regardless of what the program is doing, and will reboot.
    // Enable at start, disable after all requests have gone through and screen update starts.
    log_w("Alert triggered, forcing deep sleep for 30 seconds");
    utils::ticker_deep_sleep(30 * uS_TO_S_FACTOR);
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

    DisplayManager display;

    if (batPct < 10)
    {
        display.drawLowBattery();
        utils::ticker_hibernate();
    }

    uint32_t startTime = millis();
    Serial.begin(115200);
    delay(200);
    auto wakeup_reason = esp_sleep_get_wakeup_cause();

    String wifiMac = utils::getDeviceID();

    log_i("Program started");
    log_i("MAC ID: %s", wifiMac.c_str());
    log_d("ESP wakeup reason = %d", wakeup_reason);

    // timer setup
    alert_timer = timerBegin(0, 80, true);
    timerAttachInterrupt(alert_timer, &onTimer, true);
    timerAlarmWrite(alert_timer, ALERT_TIME_S*uS_TO_S_FACTOR, true);
    timerAlarmEnable(alert_timer); 

    pinMode(BUTTON_PIN, INPUT); // high = not pressed, low = pressed
    bool shouldEnterConfig = !digitalRead(BUTTON_PIN);
    log_d("should enter config = %d", shouldEnterConfig);

    bool hasConfig = false;
    CurrentConfig cfg = utils::readConfig(hasConfig);

    if (shouldEnterConfig)
    {
        log_d("Creating ap wifi manager");
        WiFiManager wm(cfg, 80);
        display.drawAccessPoint(wm.getAPIP());

        // async server alive in background, it will restart device when config received
        while(true){}
    }
    else
    {
        if (!hasConfig)
        {
            log_w("Failed to get config values");

            // maybe add a config site option to "factory reset" to delete config and force this mode on startup
            log_d("Creating ap wifi manager");
            WiFiManager wm(cfg, 80);
            display.drawAccessPoint(wm.getAPIP());

            // async server alive in background, it will restart device when config received
            while(true){}
        }

        String ssid = cfg.ssid;
        String pass = cfg.pass;
        String crypto = cfg.crypto;
        String fiat = cfg.fiat;
        String refreshMins = cfg.refreshMins;
        String tz = cfg.tz;
        int refreshSeconds = refreshMins.toInt() * 60;

        log_d("Using config: ssid=%s, pass=%s, crypto=%s, fiat=%s, refresh mins=%s, timezone=%s", 
              ssid, pass, crypto, fiat, refreshMins, tz.c_str());

        // don't write the config if it is part of an expected deep sleep timer wakeup
        if (wakeup_reason != ESP_SLEEP_WAKEUP_TIMER)
            display.drawConfig(ssid, pass, crypto, fiat, refreshMins.toInt());
        
        WiFiManager wm(cfg);

        if (!wm.isConnected())
        {
            log_d("Could not connect to given WiFi");
            display.drawCannotConnectToWifi(ssid, pass);

            log_d("Starting deep sleep for %d seconds", refreshSeconds);
            utils::ticker_deep_sleep(refreshSeconds * uS_TO_S_FACTOR);
        }

        if (!wm.hasInternet())
        {
            // can't get any prices if we don't have internet
            log_d("WiFi couldn't make internet connection");
            display.drawWifiHasNoInternet();

            log_d("Starting deep sleep for %d seconds", refreshSeconds);
            utils::ticker_deep_sleep(refreshSeconds * uS_TO_S_FACTOR);
        }

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
            hasPriceOneDay = wm.getPriceAtTime(crypto, fiat, constants::SecondsOneDay, priceOneDay);
            delay(1000);
            retries++;
        }

        bool hasPriceOneMonth = false;
        retries = 0;
        while(!hasPriceOneMonth && retries < 2) // in case of failure retry
        {
            hasPriceOneMonth = wm.getPriceAtTime(crypto, fiat, constants::SecondsOneMonth, priceOneMonth);
            delay(1000);
            retries++;
        }

        bool hasPriceOneYear = false;
        retries = 0;
        while(!hasPriceOneYear && retries < 2) // in case of failure retry
        {
            hasPriceOneYear = wm.getPriceAtTime(crypto, fiat, constants::SecondsOneYear, priceOneYear);
            delay(1000);
            retries++;
        }

        // can turn wifi off now - saves some power while updating display
        wm.disconnect();
        // no need for alert timer after wifi is finished
        timerAlarmDisable(alert_timer);

        // requests may have taken some time, refresh so display will show time at point of update
        wm.refreshTime(); 

        if (!hasPrice || !hasPriceOneDay || !hasPriceOneMonth || !hasPriceOneYear)
        {
            // if we do have main price could just write that with a smaller error message below it
            log_d("Could not get all price data");
            log_d("hasPrice=%d, hasPriceOneDay=%d, hasPriceOneMonth=%d, hasPriceOneYear=%d", 
                  hasPrice, hasPriceOneDay, hasPriceOneMonth, hasPriceOneYear);
            display.drawYesWifiNoCrypto(wm.getDayMonthStr(), wm.getTimeStr());
            
            // start deep sleep for 5 mins - we had internet so was likely just a blip
            log_d("Starting deep sleep for %d seconds", refreshSeconds);
            utils::ticker_deep_sleep(300 * uS_TO_S_FACTOR);
        }
        else
        {
            display.writeDisplay(crypto, fiat, price, priceOneDay, priceOneMonth, priceOneYear, wm.getDayMonthStr(), wm.getTimeStr(), batPct);
        }
        display.hibernate();
        
        log_i("Starting deep sleep");
        log_i("Program awake time: %d", millis() - startTime);

        Serial.flush();

        // start deep sleep
        log_d("Starting deep sleep for %d seconds", refreshSeconds);
        utils::ticker_deep_sleep(refreshSeconds * uS_TO_S_FACTOR);
    }
}

void loop() {}
