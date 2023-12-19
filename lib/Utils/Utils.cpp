#include "Utils.h"
#include "SPIFFS.h"
#include "Constants.h"
#include <ArduinoJson.h>

namespace utils
{

float raw_voltage()
{
    // ESP32 ADC is notoriously spiky
    // take 10 readings and average
    pinMode(35, INPUT);
    uint16_t reading = 0;
    for (int i = 0; i < 10; i++)
    {
        reading += analogRead(35);
        delay(5);
    }
    float avg = reading / 10;
    float reading_voltage = (avg / 4095) * 3.3;
    return reading_voltage;
}

float convert_voltage_reading(float volt)
{
    // schematic says voltage divider is two 100k resistors
    return 2 * volt;
}

float correct_battery_voltage(float volt)
{
    #if TTGO_BOARD_VERSION == 1
    // experimentally found this offset
    // see images/voltage_correction_2.3.1.png
    return ((volt*0.988) + 0.354);
    #elif TTGO_BOARD_VERSION == 2
    return ((volt*1.03) + 0.267);
    #else
    return volt;
    #endif

}

float battery_read()
{
    return correct_battery_voltage(convert_voltage_reading(raw_voltage()));
}

int battery_percent(float volt)
{
    using constants::BatteryMinVoltage;
    using constants::BatteryMaxVoltage;
    float inVolt = volt;

    if (inVolt < BatteryMinVoltage)
        inVolt = BatteryMinVoltage;
    if (inVolt > BatteryMaxVoltage)
        inVolt = BatteryMaxVoltage;

    float pct = ( (inVolt - BatteryMinVoltage) / (BatteryMaxVoltage - BatteryMinVoltage) ) * 100;
    return (int)pct;
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

    // will sleep in lowest power forever
    esp_deep_sleep_start(); 
}

void ticker_deep_sleep(uint64_t time)
{
    // normal deep sleep for time period, rtc memory stays active
    esp_sleep_enable_timer_wakeup(time);
    esp_deep_sleep_start(); 
}

// could go in specific lib
bool initSpiffs(bool formatOnFail)
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
    uint8_t mac[6];
    char macStr[6] = { 0 };
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
        return ConfigState::CONFIG_FAIL;
    }

    File file = SPIFFS.open("/config.json");
    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, file);
    file.close();
    if (error)
    {
        log_w("Couldn't read config");
        return ConfigState::CONFIG_FAIL;
    }

    String ssid = doc[constants::ConfigKeySsid];
    String pass = doc[constants::ConfigKeyPassword];
    String crypto = doc[constants::ConfigKeyCrypto];
    String fiat = doc[constants::ConfigKeyFiat];
    String refreshMins = doc[constants::ConfigKeyRefreshMins];
    String tz = doc[constants::ConfigKeyTimezone];
    String displayMode = doc[constants::ConfigKeyDisplayMode];
    int refreshSeconds = refreshMins.toInt() * 60;

    log_d("Read config: ssid=%s, pass=%s, crypto=%s, fiat=%s, refresh mins=%s, display mode=%s, timezone=%s", 
            ssid, pass, crypto, fiat, refreshMins, displayMode, tz.c_str());

    cfg = CurrentConfig{ssid, pass, crypto, fiat, refreshMins, tz, displayMode};

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
