#include "Utils.h"

#include "SPIFFS.h"

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
    // map given voltage between these two values, value below or above is capped at min/max
    const float maxVoltage = 4.15; // supposedly 4.2v but found hard to charge all the way to this
    const float minVoltage = 3.4; // discharge after 3.3v is quite fast, just call this the min to allow
                                   // some charge for long hibernate
    float inVolt = volt;

    if (inVolt < minVoltage)
        inVolt = minVoltage;
    if (inVolt > maxVoltage)
        inVolt = maxVoltage;

    float pct = ( (inVolt - minVoltage) / (maxVoltage - minVoltage) ) * 100;
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

}
