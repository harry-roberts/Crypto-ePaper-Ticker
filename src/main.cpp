#include "WiFiManager.h"
#include "DisplayManager.h"
#include "Login.h"
#include "Utils.h"

#include "SPIFFS.h"
#include <ArduinoJson.h>

#define uS_TO_S_FACTOR 1000000
#define ALERT_TIME_S  150

#define SECONDS_ONE_DAY 86400L
#define SECONDS_ONE_MONTH 2592000L
#define SECONDS_ONE_YEAR 31536000L

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
    log_w("Alert triggered, restarting device");
    ESP.restart();
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

    // timer setup
    alert_timer = timerBegin(0, 80, true);
    timerAttachInterrupt(alert_timer, &onTimer, true);
    timerAlarmWrite(alert_timer, ALERT_TIME_S*uS_TO_S_FACTOR, true);
    timerAlarmEnable(alert_timer);

    uint32_t startTime = millis();
    Serial.begin(115200);
    delay(200);
    log_i("Program started");

    pinMode(BUTTON_PIN, INPUT); // high = not pressed, low = pressed
    bool shouldEnterConfig = !digitalRead(BUTTON_PIN);
    log_d("should enter config = %d", shouldEnterConfig);

    DisplayManager display;

    if (shouldEnterConfig)
    {
        log_d("Creating ap wifi manager");
        WiFiManager wm;
        String msg = "Connect to:\n";
        msg += wm.getAPIP();
        display.writeGenericText(msg);

        // async server alive in background, it will restart device when config received
        while(true){}
    }
    else
    {
        // read config from spiffs
        if (!utils::initSpiffs())
        {
            log_w("Could not init spiffs");
            return;
        }

        // read spiffs config
        File file = SPIFFS.open("/config.json");
        StaticJsonDocument<200> doc;
        DeserializationError error = deserializeJson(doc, file);
        if (error)
        {
            log_w("Failed to read config file"); // should probably restart in force config mode
            return;
        }

        String ssid = doc["s"];
        String pass = doc["p"];
        String crypto = doc["c"];
        String fiat = doc["f"];
        String refreshMins = doc["r"];
        int refreshSeconds = refreshMins.toInt() * 60;

        log_d("Using config: ssid=%s, pass=%s, crypto=%s, fiat=%s, refresh mins=%s", ssid, pass, crypto, fiat, refreshMins);

        file.close();
        
        WiFiManager wm(ssid, pass);

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
            hasPriceOneDay = wm.getPriceAtTime(crypto, fiat, SECONDS_ONE_DAY, priceOneDay);
            delay(1000);
            retries++;
        }

        bool hasPriceOneMonth = false;
        retries = 0;
        while(!hasPriceOneMonth && retries < 2) // in case of failure retry
        {
            hasPriceOneMonth = wm.getPriceAtTime(crypto, fiat, SECONDS_ONE_MONTH, priceOneMonth);
            delay(1000);
            retries++;
        }

        bool hasPriceOneYear = false;
        retries = 0;
        while(!hasPriceOneYear && retries < 2) // in case of failure retry
        {
            hasPriceOneYear = wm.getPriceAtTime(crypto, fiat, SECONDS_ONE_YEAR, priceOneYear);
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
