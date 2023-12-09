#include "TickerCoordinator.h"

#include "Constants.h"

TickerCoordinator::TickerCoordinator(int batPct, bool shouldEnterConfig) :
    m_batPct(batPct),
    m_shouldEnterConfig(shouldEnterConfig)
{
}

int TickerCoordinator::run()
{
    if (m_batPct < 10)
    {
        m_displayManager.drawLowBattery();
        utils::ticker_hibernate();
    }

    bool hasConfig = false;
    m_cfg = utils::readConfig(hasConfig);

    if (m_shouldEnterConfig || !hasConfig)
        enterConfigMode();
    else
        enterNormalMode();

    m_displayManager.hibernate();
    return m_refreshSeconds;
}

void TickerCoordinator::enterConfigMode()
{
    log_d("Creating access point for config");
    m_wifiManager.initConfigMode(m_cfg, 80);
    m_displayManager.drawAccessPoint(m_wifiManager.getAPIP());

    // async server alive in background, it will restart device when config received
    log_i("Config mode is complete - waiting for config to be received");
    while(true){}
}

void TickerCoordinator::enterNormalMode()
{
    m_refreshSeconds = m_cfg.refreshMins.toInt() * 60;

    // in case of some kind of error - don't want this null
    if (m_refreshSeconds < 60)
    {
        m_refreshSeconds = 300;
        m_cfg.refreshMins = String(m_refreshSeconds);
    }

    log_d("Using config: ssid=%s, pass=%s, crypto=%s, fiat=%s, refresh mins=%s, timezone=%s", 
           m_cfg.ssid, m_cfg.pass, m_cfg.crypto, m_cfg.fiat, m_cfg.refreshMins, m_cfg.tz.c_str());

    // don't draw the config if it is part of an expected deep sleep timer wakeup
    if (esp_sleep_get_wakeup_cause() != ESP_SLEEP_WAKEUP_TIMER)
        m_displayManager.drawConfig(m_cfg.ssid, m_cfg.pass, m_cfg.crypto, m_cfg.fiat, m_cfg.refreshMins.toInt());

    m_wifiManager.initNormalMode(m_cfg);
    if (!checkWifi())
    {
        m_refreshSeconds = constants::SleepSecondsAfterWiFiFail;
        return;
    }

    float price, priceOneDay, priceOneMonth, priceOneYear;

    bool fullSuccess = getPriceAtTime(0, price) &&
                       getPriceAtTime(constants::SecondsOneDay, priceOneDay) &&
                       getPriceAtTime(constants::SecondsOneMonth, priceOneMonth) &&
                       getPriceAtTime(constants::SecondsOneYear, priceOneYear, true);
    
    // can turn wifi off now - saves some power while updating display
    m_wifiManager.disconnect();
    // requests may have taken some time, refresh so display will show time at point of update
    m_wifiManager.refreshTime(); 

    if (fullSuccess)
    {
        m_displayManager.writeDisplay(m_cfg.crypto, m_cfg.fiat, price, priceOneDay, priceOneMonth, priceOneYear, 
                                      m_wifiManager.getDayMonthStr(), m_wifiManager.getTimeStr(), m_batPct);
    }
    else
    {
        // if we do have main price could just write that with a smaller error message below it
        log_d("Could not get all price data");
        m_displayManager.drawYesWifiNoCrypto(m_wifiManager.getDayMonthStr(), m_wifiManager.getTimeStr());
        
        m_refreshSeconds = constants::SleepSecondsAfterDataFail;
        return;
    }

    log_i("Normal mode is complete");
}

bool TickerCoordinator::checkWifi()
{
    if (!m_wifiManager.isConnected())
    {
        log_d("Could not connect to given WiFi");
        m_displayManager.drawCannotConnectToWifi(m_cfg.ssid, m_cfg.pass);
        return false;
    }

    if (!m_wifiManager.hasInternet())
    {
        // can't get any prices if we don't have internet
        log_d("WiFi couldn't make internet connection");
        m_displayManager.drawWifiHasNoInternet();
        return false;
    }
    return true;
}

bool TickerCoordinator::getPriceAtTime(time_t unixOffset, float& priceOut, bool quickReturn)
{
    bool gotPrice = false;
    int retries = 0;
    while(!gotPrice && retries < constants::WiFiRequestRetries) // in case of failure retry
    {
        gotPrice = m_wifiManager.getPriceAtTime(m_cfg.crypto, m_cfg.fiat, unixOffset, priceOut);
        // good to have a slight delay between requests, but the final request does not need one
        if (!quickReturn)
            delay(500);
        retries++;
    }

    return gotPrice;
}
