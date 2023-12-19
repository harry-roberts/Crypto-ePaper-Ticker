#ifndef TICKERCOORDINATOR_H
#define TICKERCOORDINATOR_H

#include <Arduino.h>
#include "DisplayManager.h"
#include "WiFiManager.h"
#include "Utils.h"

class TickerCoordinator
{
public:
    TickerCoordinator(int batPct, bool shouldEnterConfig, hw_timer_t *alert_timer);

    int run();

private:
    DisplayManager m_displayManager;
    WiFiManager m_wifiManager;
    CurrentConfig m_cfg;
    hw_timer_t* m_alertTimer;

    int m_refreshSeconds = 300;
    int m_batPct;
    bool m_shouldEnterConfig;
    bool m_initial;

    void enterConfigMode();
    void enterNormalMode();
    
    bool checkWifi();
};


#endif