#include "TickerCoordinator.h"
#include "Constants.h"

#include "SPIFFS.h"

TickerCoordinator::TickerCoordinator(const TickerInput& input) :
    m_batPct(input.batPercent),
    m_shouldEnterConfig(input.shouldEnterConfig),
    m_numWifiFailures(input.numConsecutiveWifiFails),
    m_numDataFailures(input.numConsecutiveDataFails),
    m_bootCount(input.bootCount),
    m_waitForNtpSync(input.waitForNtpSync),
    m_alertTimer(input.alert_timer)
{
    log_d("Created TickerCoordinator with input: batPercent=%d, shouldEnterConfig=%d, WifiFails=%d, DataFails=%d, "
          "bootCount=%d, waitForNtpSync=%d", 
           m_batPct, m_shouldEnterConfig, m_numWifiFailures, m_numDataFailures, m_bootCount, m_waitForNtpSync);
}

TickerOutput TickerCoordinator::run()
{
    utils::initSpiffs();

    if (m_batPct < constants::MinimumAllowedBatteryPercent)
    {
        log_d("battery is below minimum needed");
        bool hasWrittenLowBatWarning = false;

        // read the file and update hasWrittenLowBatWarning if needed
        File file = SPIFFS.open(constants::SpiffsBatLogFileName, FILE_READ);
        if (!file)
        {
            log_d("cannot open battery log file");
        }
        else
        {
            log_d("reading low battery log");
            String batLogFile = file.readString();
            if (batLogFile == "1")
                hasWrittenLowBatWarning = true;
        }
        file.close();

        if (hasWrittenLowBatWarning) // no need to write to display again
            log_d("already drawn display with low battery, will hibernate");
        else
        {
            // update display and write to the file that it has now been updated
            m_displayManager.drawLowBattery();
            File file = SPIFFS.open(constants::SpiffsBatLogFileName, FILE_WRITE);
            if (!file)
                log_w("m_batPct < 10, hasWrittenLowBatWarning = false, and failed to open battery log file");
            else
            {
                log_d("writing to low battery log");
                file.print("1");
                file.close();
            }
        }
        // display has been updated if needed, log file written if needed, now sleep forever
        utils::ticker_hibernate();
    }

    // battery is ok if we get here, just remove the file
    log_d("battery is ok, removing log file if it exists");
    SPIFFS.remove(constants::SpiffsBatLogFileName);

    utils::ConfigState cfgState = utils::readConfig(m_cfg);

    if (m_shouldEnterConfig)
    {
        m_displayManager.writeGenericText("Starting config mode...");
        enterConfigMode();
    }
    else if (cfgState == utils::ConfigState::CONFIG_FAIL)
    {
        m_displayManager.writeGenericText("Error with given config\nRestarting in config mode");
        enterConfigMode();
    }
    else if (cfgState == utils::ConfigState::CONFIG_NO_SSID)
    {
        m_displayManager.writeGenericText("SSID is required\nRestarting in config mode");
        enterConfigMode();
    }
    else
    {
        enterNormalMode();
    }

    m_wifiManager.disconnect();

    if (m_wifiStatus != WiFiStatus::OK)
    {
        if (m_numWifiFailures > 0 || m_bootCount == 1) // don't draw on first fail, sleep smallest time and try again first
                                                       // unless first boot, then do draw it
        {
            if (m_wifiStatus == WiFiStatus::NO_CONNECTION)
                m_displayManager.drawCannotConnectToWifi(m_cfg.ssid, m_cfg.pass);
            else if (m_wifiStatus == WiFiStatus::NO_INTERNET)
                m_displayManager.drawWifiHasNoInternet();
        }
        m_numWifiFailures++;
        log_d("Consecutive WiFi connection failure number %d", m_numWifiFailures);
        int failLevel = min(m_numWifiFailures, constants::SleepSecondsAfterWiFiFailLevels);
        m_refreshSeconds = constants::SleepSecondsAfterWiFiFail[failLevel-1];
        log_d("Set sleep time to %d", m_refreshSeconds);
    }
    else if (m_dataFailed) // wifi fail takes priority so this is only if wifi was ok
    {
        if (m_numDataFailures > 0 || m_bootCount == 1) // don't draw on first fail, sleep smallest time and try again first
                                                       // unless first boot, then do draw it
            m_displayManager.drawYesWifiNoCrypto(m_wifiManager.getDayMonthStr(), m_wifiManager.getTimeStr());

        m_numDataFailures++;
        log_d("Consecutive data retrieval failure number %d", m_numDataFailures);
        int failLevel = min(m_numDataFailures, constants::SleepSecondsAfterDataFailLevels);
        m_refreshSeconds = constants::SleepSecondsAfterDataFail[failLevel-1];
        log_d("Set sleep time to %d", m_refreshSeconds);
    }
    else if (m_secondsLeftOfSleep > 0) // we should be in overnight sleep with this many seconds left
    {
        log_d("Should be in overnight sleep - updating display");
        m_displayManager.drawOvernightSleep();
    }

    m_displayManager.hibernate();

    TickerOutput output{m_refreshSeconds, m_wifiStatus != WiFiStatus::OK, m_dataFailed, m_secondsLeftOfSleep};
    return output;
}

void TickerCoordinator::enterConfigMode()
{
    timerAlarmWrite(m_alertTimer, constants::ConfigAlertTimeSeconds * constants::MicrosToSecondsFactor, true);
    timerAlarmEnable(m_alertTimer);
    log_d("Creating access point for config");
    m_wifiManager.initConfigMode(m_cfg, 80);
    m_displayManager.drawAccessPoint(m_wifiManager.getAPIP());

    // async server alive in background, it will restart device when config received
    log_i("Config mode is complete - waiting for config to be received");
    while(true)
    {
        AdminRequest req = m_wifiManager.getAdminRequest();
        if (req.set)
        {
            switch(req.action)
            {
                case AdminAction::REQUEST_BTC:
                    log_d("Received admin action to request BTC, ssid=%s pass=%s", req.ssid, req.password);
                    m_cfg = CurrentConfig{req.ssid, req.password, "BTC", "USD", "60", 
                                          "GMT0BST,M3.5.0/1,M10.5.0", "simple"};
                    enterNormalMode();
                    log_d("Finished admin action");
                    // access point no longer active now since we entered normal mode
                    // probably don't want to do anything further after this anyway
                    break;
                case AdminAction::FORMAT_SPIFFS:
                {
                    log_d("Received admin action to format SPIFFS");
                    bool success = SPIFFS.format();
                    log_d("SPIFFS formatted with result=%d", success);
                    break;
                }
                case AdminAction::FILL_SCREEN:
                    log_d("Received admin action to fill screen");
                    m_displayManager.fillScreen();
                    break;
                case AdminAction::NONE:
                default:
                    break;
            }
            m_wifiManager.resetAdminRequest();
        }
        delay(500);
    }
}

void TickerCoordinator::enterNormalMode()
{
    timerAlarmWrite(m_alertTimer, constants::NormalAlertTimeSeconds * constants::MicrosToSecondsFactor, true);
    timerAlarmEnable(m_alertTimer);
    m_refreshSeconds = m_cfg.refreshMins.toInt() * 60;

    // in case of some kind of error - don't want this null
    if (m_refreshSeconds < 60)
    {
        m_refreshSeconds = 300;
        m_cfg.refreshMins = String(m_refreshSeconds);
    }

    log_d("Using config: ssid=%s, pass=%s, crypto=%s, fiat=%s, refresh mins=%s, timezone=%s, is24Hour=%d", 
           m_cfg.ssid, m_cfg.pass, m_cfg.crypto, m_cfg.fiat, m_cfg.refreshMins, m_cfg.tz.c_str(), m_cfg.is24Hour);

    // don't draw the config if it is part of an expected deep sleep timer wakeup
    if (esp_sleep_get_wakeup_cause() != ESP_SLEEP_WAKEUP_TIMER)
        m_displayManager.drawConfig(m_cfg.ssid, m_cfg.pass, m_cfg.crypto, m_cfg.fiat, m_cfg.refreshMins.toInt());

    m_wifiStatus = m_wifiManager.initNormalMode(m_cfg, m_waitForNtpSync);
    
    if (m_wifiStatus != WiFiStatus::OK)
        return;

    // check if we are supposed to be on an overnight sleep now that we have the time
    if (m_cfg.overnightSleepStart >= 0)
    {
        bool shouldBeAsleep = m_wifiManager.isCurrentTimeDuringOvernightSleep(m_cfg.overnightSleepStart,
                                                                            m_cfg.overnightSleepLength,
                                                                            m_secondsLeftOfSleep);
        if (shouldBeAsleep)
            return;
    }

    std::set<long> unixOffsets;
    if (m_cfg.displayMode == constants::ConfigDisplayModeSimple)
        unixOffsets = {0, constants::SecondsOneDay};
    else
        unixOffsets = {0, constants::SecondsOneDay, constants::SecondsOneMonth, constants::SecondsOneYear};
    
    std::map<long, float> priceData = m_wifiManager.getPriceData(m_cfg.crypto, m_cfg.fiat, unixOffsets);
    
    // can turn wifi off now - saves some power while updating display
    m_wifiManager.disconnect();
    // requests may have taken some time, refresh so display will show time at point of update
    m_wifiManager.refreshTime(); 

    bool shouldDisplayBattery = (m_cfg.showSimpleBattery && m_cfg.displayMode == constants::ConfigDisplayModeSimple) ||
                                m_cfg.displayMode == constants::ConfigDisplayModeAdvanced;

    if (!priceData.empty())
    {
        m_displayManager.writeDisplay(m_cfg.crypto, m_cfg.fiat, priceData, 
                                      m_wifiManager.getDayMonthStr(), m_wifiManager.getTimeStr(), 
                                      shouldDisplayBattery ? m_batPct : 0);
    }
    else
    {
        // if we do have main price could just write that with a smaller error message below it
        log_d("Could not get all price data");        
        m_dataFailed = true;
        return;
    }

    log_i("Normal mode is complete");
}

