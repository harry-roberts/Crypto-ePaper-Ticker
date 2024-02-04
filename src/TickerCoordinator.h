#ifndef TICKERCOORDINATOR_H
#define TICKERCOORDINATOR_H

#include <Arduino.h>
#include "DisplayManager.h"
#include "WiFiManager.h"
#include "Utils.h"

using namespace WiFiManagerLib;

struct TickerInput
{
    int batPercent;
    bool shouldEnterConfig;
    int numConsecutiveWifiFails;
    int numConsecutiveDataFails;
    int bootCount;
    bool waitForNtpSync;
    hw_timer_t *alert_timer;
};

struct TickerOutput
{
    int refreshSeconds;
    bool wifiFailed;
    bool dataFailed;
    uint64_t secondsLeftOfSleep;
};

class TickerCoordinator
{
public:
    TickerCoordinator(const TickerInput& input);

    TickerOutput run();

private:
    DisplayManager m_displayManager;
    WiFiManager m_wifiManager;
    CurrentConfig m_cfg;

    int m_refreshSeconds = 300;
    int m_batPct;
    bool m_shouldEnterConfig;
    int m_numWifiFailures;
    int m_numDataFailures;
    WiFiStatus m_wifiStatus = WiFiStatus::UNKNOWN;
    bool m_dataFailed = false; // default false as don't want to mark it failed if wifi failed
    int m_bootCount;
    bool m_waitForNtpSync;

    uint64_t m_secondsLeftOfSleep = 0;

    hw_timer_t* m_alertTimer;

    void enterConfigMode();
    void enterNormalMode();
};


#endif