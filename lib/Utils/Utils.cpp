#include "Utils.h"
#include "SPIFFS.h"
#include "Constants.h"
#include <ArduinoJson.h>

namespace utils
{

float raw_voltage()
{
    // ESP32 ADC is notoriously spiky
    // take 20 readings and average
    pinMode(35, INPUT);
    uint32_t reading = 0;
    for (int i = 0; i < 20; i++)
    {
        reading += analogRead(35);
        delay(10);
    }
    float avg = reading / 20;
    float reading_voltage = (avg / 4095) * 3.3; // max adc in is 3.3v
    log_d("Battery: reading_voltage=%f", reading_voltage);
    return reading_voltage;
}

float convert_voltage_reading(const float volt)
{
    // schematic says voltage divider is two 100k resistors
    return 2 * volt;
}

float correct_battery_voltage(float volt)
{
    float rtn;
    #if TTGO_BOARD_VERSION == 1
    // experimentally found this offset
    // see images/voltage_correction_2.3.1.png
    rtn = ((volt*0.988) + 0.354);
    #elif TTGO_BOARD_VERSION == 2
    rtn = ((volt*1.03) + 0.267);
    #else
    rtn = volt;
    #endif

    log_d("Battery: correct_battery_voltage=%f", rtn);
    return rtn;

}

float battery_read()
{
    return correct_battery_voltage(convert_voltage_reading(raw_voltage()));
}

int battery_percent(const float volt)
{
    // expermentally found by discharging at constant load and logging voltage readings over time
    // mapped here so that index of array = percentage value
    // minimum was 3.43V before stopping, calling this 5%
    float voltToPercent[101] = {3.4,3.4,3.4,3.4,3.4,3.43,3.45,3.462,3.474,3.486,3.493,3.503,3.512,3.521,3.531,3.537,3.543,3.552,3.561,
                                3.57,3.578,3.585,3.588,3.593,3.599,3.607,3.615,3.626,3.633,3.639,3.642,3.648,3.655,3.661,3.666,3.671,
                                3.676,3.683,3.688,3.691,3.697,3.701,3.706,3.712,3.717,3.72,3.725,3.728,3.731,3.736,3.741,3.743,3.746,
                                3.751,3.757,3.762,3.767,3.771,3.776,3.782,3.787,3.79,3.793,3.796,3.801,3.805,3.811,3.817,3.822,3.827,
                                3.834,3.839,3.843,3.848,3.854,3.862,3.87,3.875,3.883,3.89,3.895,3.899,3.903,3.908,3.916,3.921,3.93,
                                3.937,3.945,3.95,3.958,3.969,3.978,3.988,3.996,4.004,4.02,4.031,4.047,4.071,4.11};

    // save the iterating in these cases
    if (volt <= 3.4)
        return 0;
    if (volt >= 4.11)
        return 100;

    // iterate the voltToPercent array and find the closest match
    float smallestDiff = abs(volt - voltToPercent[0]);
    int percent = 0;
    for (int i = 1; i <= 100; i++)
    {
        float diff = abs(volt - voltToPercent[i]);
        if (diff < smallestDiff)
        {
            smallestDiff = diff;
            percent = i;
        }
    }

    log_d("Battery: voltage=%f, percent=%d", volt, percent);

    return percent;
}

void ticker_hibernate()
{
    // hibernation settings
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH,   ESP_PD_OPTION_OFF);
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_SLOW_MEM, ESP_PD_OPTION_OFF);
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_FAST_MEM, ESP_PD_OPTION_OFF);
    esp_sleep_pd_config(ESP_PD_DOMAIN_XTAL,         ESP_PD_OPTION_OFF);

    // shouldn't be the case but just in case this is enabled
    esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_TIMER);

    log_d("Hibernating forever");
    Serial.flush();
    // will sleep in lowest power forever
    esp_deep_sleep_start(); 
}

void ticker_deep_sleep(const uint64_t time)
{
    // normal deep sleep for time period, rtc memory stays active
    esp_sleep_enable_timer_wakeup(time);
    Serial.flush();
    esp_deep_sleep_start(); 
}

// could go in specific lib
bool initSpiffs(const bool formatOnFail)
{
    if (!SPIFFS.begin(formatOnFail))
    {
        log_w("An error has occurred while mounting SPIFFS");
        return false;
    }
    log_d("SPIFFS mounted successfully");
    return true;
}

String getDeviceID()
{
    uint8_t mac[7];
    char macStr[7] = { 0 };
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    sprintf(macStr, "%02x%02x%02x", mac[3], mac[4], mac[5]); // starting 3 is always the same, only need last 3
    return String(macStr);
}

ConfigState readConfig(CurrentConfig& cfg)
{
    // read config from spiffs
    if (!utils::initSpiffs())
    {
        log_w("Could not init spiffs");
        return ConfigState::CONFIG_SPIFFS_ERROR;
    }

    File file = SPIFFS.open(constants::SpiffsConfigFileName);
    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, file);
    file.close();
    if (error)
    {
        log_w("Couldn't read config, error=%d", error);
        return ConfigState::CONFIG_NO_FILE;
    }

    // ArduinoJSON will set an actual string value of "null" if key is empty, so check for it and set our own empty string
    String ssid = doc[constants::ConfigKeySsid].isNull() ? String() : doc[constants::ConfigKeySsid];
    String pass = doc[constants::ConfigKeyPassword].isNull() ? String() : doc[constants::ConfigKeyPassword];
    String crypto = doc[constants::ConfigKeyCrypto].isNull() ? String() : doc[constants::ConfigKeyCrypto];
    String fiat = doc[constants::ConfigKeyFiat].isNull() ? String() : doc[constants::ConfigKeyFiat];
    String refreshMins = doc[constants::ConfigKeyRefreshMins].isNull() ? String() : doc[constants::ConfigKeyRefreshMins];
    String tz = doc[constants::ConfigKeyTimezone].isNull() ? String() : doc[constants::ConfigKeyTimezone];;
    String displayMode = doc[constants::ConfigKeyDisplayMode].isNull() ? String() : doc[constants::ConfigKeyDisplayMode];
    bool is24Hour = doc[constants::ConfigKeyTimeFormat].isNull() ? true : (doc[constants::ConfigKeyTimeFormat] == "1");
    int overnightSleepStart = doc[constants::ConfigKeyOvernightSleepStart].isNull() ? -1 : doc[constants::ConfigKeyOvernightSleepStart].as<int>();
    int overnightSleepLength = doc[constants::ConfigKeyOvernightSleepLength].isNull() ? 0 : doc[constants::ConfigKeyOvernightSleepLength].as<int>();
    bool simpleBattery = doc[constants::ConfigKeyDisplaySimpleBattery].isNull() ? true : (doc[constants::ConfigKeyDisplaySimpleBattery] == "1");

    log_d("Read config: ssid=%s, pass=%s, crypto=%s, fiat=%s, refresh mins=%s, display mode=%s, timezone=%s, is24Hour=%d, NightStart=%d, NightLength=%d, simpleBattery=%d", 
            ssid, pass, crypto, fiat, refreshMins, displayMode, tz.c_str(), is24Hour, overnightSleepStart, overnightSleepLength, simpleBattery);

    cfg = CurrentConfig{ssid, pass, crypto, fiat, refreshMins, tz, displayMode, is24Hour, overnightSleepStart, overnightSleepLength, simpleBattery};

    if (cfg.ssid.isEmpty()) // password allowed to be blank, others have defaults in html. Could enforce this in html instead 
        return ConfigState::CONFIG_NO_SSID;

    if (!cfg.crypto.isEmpty() && !cfg.fiat.isEmpty() && !cfg.tz.isEmpty() 
        && !cfg.refreshMins.isEmpty() && !cfg.displayMode.isEmpty())
    {
        return ConfigState::CONFIG_OK;
    }

    return ConfigState::CONFIG_FAIL;    
}

}
