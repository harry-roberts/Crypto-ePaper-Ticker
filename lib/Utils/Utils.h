#ifndef TICKER_UTILS_H
#define TICKER_UTILS_H

#include <Arduino.h>

// various hardware utility functions that don't really fit into a class

namespace utils
{

float raw_voltage();
float convert_voltage_reading(float volt);
float battery_read();
int battery_percent(float volt);

void ticker_hibernate();
void ticker_deep_sleep(uint64_t time);

bool initSpiffs(bool formatOnFail = true);

}

#endif
