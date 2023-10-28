#include "Utils.h"

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

float battery_read()
{
  return convert_voltage_reading(raw_voltage());
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

}
