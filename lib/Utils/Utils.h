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

float raw_voltage();
float convert_voltage_reading(float volt);
float battery_read();
int battery_percent(float volt);

void ticker_hibernate();
void ticker_deep_sleep(uint64_t time);

bool initSpiffs(bool formatOnFail = true);

String getDeviceID();

CurrentConfig readConfig(bool& success);

}

#endif
