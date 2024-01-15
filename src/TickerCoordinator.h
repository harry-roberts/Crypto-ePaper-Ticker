#ifndef TICKERCOORDINATOR_H
#define TICKERCOORDINATOR_H

#include <Arduino.h>
#include "DisplayManager.h"
#include "WiFiManager.h"
#include "Utils.h"

struct TickerInput
{
    int batPercent;
    bool shouldEnterConfig;
    int numConsecutiveWifiFails;
    int numConsecutiveDataFails;
    int bootCount;
    hw_timer_t *alert_timer;
};

struct TickerOutput
{
    int refreshSeconds;
    bool wifiFailed;
    bool dataFailed;
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
    hw_timer_t* m_alertTimer;

    int m_refreshSeconds = 300;
    int m_batPct;
    bool m_shouldEnterConfig;
    bool m_initial;
    int m_numWifiFailures;
    int m_numDataFailures;
    bool m_wifiFailed = true;
    bool m_dataFailed = false; // default false as don't want to mark it failed if wifi failed
    int m_bootCount;

    void enterConfigMode();
    void enterNormalMode();
    
    bool checkWifi();
};


#endif