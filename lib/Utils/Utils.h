#ifndef TICKER_UTILS_H
#define TICKER_UTILS_H

#include <Arduino.h>

// various hardware utility functions that don't really fit into a class

namespace utils
{

struct CurrentConfig
{
    String ssid;
    String pass;
    String crypto;
    String fiat;
    String refreshMins;
    String tz;
    String displayMode;
};

enum class ConfigState
{
    CONFIG_OK,       // the config was read successfully
    CONFIG_NO_SSID,  // ssid was missing, should be only value that is possible as blank
    CONFIG_FAIL      // something failed, couldn't read config
};

float raw_voltage();
float convert_voltage_reading(const float volt);
float battery_read();
int battery_percent(const float volt);

void ticker_hibernate();
void ticker_deep_sleep(const uint64_t time);

bool initSpiffs(const bool formatOnFail = true);

String getDeviceID();

ConfigState readConfig(CurrentConfig& cfg);

}

#endif
