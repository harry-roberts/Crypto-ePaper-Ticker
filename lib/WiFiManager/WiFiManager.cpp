#include "WiFiManager.h"
#include <ArduinoJson.h>
#include "Constants.h"

#include <AsyncElegantOTA.h>

#include "SPIFFS.h"
#include "esp_sntp.h"

namespace WiFiManagerLib
{

WiFiStatus WiFiManager::initNormalMode(const CurrentConfig& cfg, bool waitForNtpSync, bool initAllDataSources)
{
    m_ssid = cfg.ssid;
    m_password = cfg.pass;
    m_is24Hour = cfg.is24Hour;
    m_isAccessPoint = false;
    if (initAllDataSources)
        initAllAvailableDataSources();

    log_d("Connecting to known WiFi point %s", m_ssid.c_str());
    WiFi.begin(m_ssid, m_password);
    int retries = 0;
    while ((WiFi.status() != WL_CONNECTED) && (retries < 15)) 
    {
        retries++;
        delay(500);
    }
    if (WiFi.status() == WL_CONNECTED)
    {
        log_i("Connected to %s", m_ssid.c_str());

        const char* ntpServer = "pool.ntp.org";
        log_d("Getting time from ntp");
        configTime(0, 0, ntpServer);

        log_d("Setting timezone to %s", cfg.tz.c_str());
        setenv("TZ", cfg.tz.c_str(), 1); // will be in config
        tzset();

        bool gotTime = false;
        int timeRetries = 0;

        while (!gotTime && timeRetries < 10)
        {
            gotTime = getTime(m_timeinfo, waitForNtpSync);
            if (gotTime)
            {
                setTimeVars(m_timeinfo);
                log_d("Time: %s %s", m_dayMonth.c_str(), m_time.c_str());
                log_d("Epoch: %d", m_epoch);
                m_status = WiFiStatus::OK;
            }
            else
                delay(500);
            timeRetries++;
            
        }
        if (!gotTime)
        {
            // we can be confident the NTP pool server will not be down
            // if we don't have an epoch time, we don't have an internet connection
            m_status = WiFiStatus::NO_INTERNET;
        }
    }
    else
    {
        m_status = WiFiStatus::NO_CONNECTION;
    }
    
    return m_status;
}

void WiFiManager::initConfigMode(const CurrentConfig& cfg, int port)
{
    log_d("Creating access point for configuration");
    m_server = std::make_unique<AsyncWebServer>(port);
    WiFi.mode(WIFI_AP_STA);

    log_d("Scanning for networks");
    WiFi.disconnect();
    int n = WiFi.scanNetworks();
    for (int i = 0; i < n; ++i)
    {
        log_d("Network %d: %s Strength: %d", i, WiFi.SSID(i).c_str(), WiFi.RSSI(i));
        m_scannedSsids.insert(WiFi.SSID(i));
    }

    m_isAccessPoint = WiFi.softAP(constants::WifiAccessPointName, NULL);
    if (!m_isAccessPoint)
    {
        log_w("Access point init failed");
        return;
    }

    if (utils::initSpiffs())
    {
        // Web Server Root URL
        m_server->on("/", HTTP_GET, [](AsyncWebServerRequest *request)
        {
            request->send(SPIFFS, "/config.html", "text/html");
        });
        m_server->on("/config.js", HTTP_GET, [this, cfg](AsyncWebServerRequest *request){
            request->send(200, "text/html", generateConfigJs(cfg));
        });
        m_server->serveStatic("/", SPIFFS, "/");
        m_server->on("/", HTTP_POST, [this](AsyncWebServerRequest *request) 
        {
            int params = request->params();
            StaticJsonDocument<768> doc;
            for(int i = 0; i < params; i++)
            {
                AsyncWebParameter* p = request->getParam(i);
                if(p->isPost())
                {
                    // use the name of the input as the json key
                    // they were set to a single character in the html to save space in this json
                    doc[p->name()] = p->value().c_str();
                    log_d("config \"%s\" set to: %s", p->name(), p->value().c_str());
                }
            }

            File file = SPIFFS.open(constants::SpiffsConfigFileName, FILE_WRITE);
            if (!file)
            {
                log_w("failed to open config file for writing");
                return;
            }

            if (doc.overflowed()) 
            {
                log_w("Json overflowed - some values will be missing");
                request->send(200, "text/html", "<h1 style=\"font-family: Arial, Helvetica, sans-serif;\">Error: the config was too large and could not be saved. Restarting device</h1>");
            }
            else if (serializeJson(doc, file) == 0) 
            {
                log_w("Failed to write config to file");
                request->send(200, "text/html", "<h1 style=\"font-family: Arial, Helvetica, sans-serif;\">An error occurred, the config was not saved. Restarting device</h1>");
            }
            else if (doc[constants::ConfigKeySsid] == "")
            {
                request->send(200, "text/html", "<h1 style=\"font-family: Arial, Helvetica, sans-serif;\">No SSID given. Restarting device in config mode.</h1>");
            }
            else
                request->send(200, "text/html", "<h1 style=\"font-family: Arial, Helvetica, sans-serif;\">Config saved successfully. Restarting device.</h1>");
        
            file.close();

            delay(500);
            ESP.restart();
        });

        // this library adds a /update page to the existing web server, which handles an OTA file upload for new firmware (it just works)
        // ideal solution would be to connect to a remote web server to fetch a file itself, but this works fine for first version
        AsyncElegantOTA.begin(m_server.get());

        m_server->begin();
    }
}

void WiFiManager::disconnect()
{
    log_d("Disconnecting from WiFi");
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
}

bool WiFiManager::getTime(tm& timeinfo, bool waitForNtpSync)
{
    if(!getLocalTime(&timeinfo))
    {
        log_w("Failed to obtain time");
        return false;
    }
    if (waitForNtpSync)
    {
        // wait until the ntp server has responded
        uint32_t start = millis();
        uint32_t end = start + (constants::NtpResyncTimeoutSeconds * 1000);
        while (sntp_get_sync_status() != SNTP_SYNC_STATUS_COMPLETED && millis() < end)
            delay(100);
        if (millis() >= end)
            log_w("Did not see a requested NTP sync after timeout of %d seconds", constants::NtpResyncTimeoutSeconds);
        else
        {
            log_d("Successfully synced time with NTP");
            getLocalTime(&timeinfo); // refresh our stored time with the sync
        }
    }

    return true;
}

void WiFiManager::setTimeVars(tm& timeinfo)
{
    {
        char buf[20];
        strftime(buf, 20, "%e %b", &timeinfo);
        m_dayMonth = String(buf);
        m_dayMonth.trim();
    }
    {
        char buf[20];
        if (m_is24Hour)
            strftime(buf, 20, "%H:%M", &timeinfo);
        else
            strftime(buf, 20, "%I:%M%p", &timeinfo);

        m_time = String(buf);
        m_time.toLowerCase();
    }

    time(&m_epoch);
}

void WiFiManager::refreshTime()
{
    log_d("Refreshing stored time");

    if (getTime(m_timeinfo))
    {
        setTimeVars(m_timeinfo);
        log_d("Time: %s %s", m_dayMonth.c_str(), m_time.c_str());
        log_d("Epoch: %d", m_epoch);
    }
}

void WiFiManager::setTimeInfo(int h, int m, int s)
{
    m_timeinfo.tm_hour = h;
    m_timeinfo.tm_min = m;
    m_timeinfo.tm_sec = s;
}

// whether a given hour number is between the start and end of the overnight sleep
bool WiFiManager::isCurrentTimeDuringOvernightSleep(int sleepStartHour, int sleepHoursLength, uint64_t& secondsLeftOfSleep)
{    
    log_d("The current time is %d:%d:%d", m_timeinfo.tm_hour, m_timeinfo.tm_min, m_timeinfo.tm_sec);
    log_d("The overnight sleep should start at hour %d and last %d hours", sleepStartHour, sleepHoursLength);

    for (int i = 0; i < sleepHoursLength; i++)
    {
        int compareHour = i + sleepStartHour; // could be over 23, i.e. 1am would be value of 25
        if (compareHour >= 24) compareHour -= 24; // turn a value of e.g. 25 -> 1
        if (compareHour == m_timeinfo.tm_hour)
        {
            log_d("The time is within the overnight sleep period");
            // full hours left during sleep is length - (i+1)
            // full time between now and end of sleep is that + the minutes left to next hour number
            int fullHoursToSleepEnd = sleepHoursLength - (i + 1);
            int minsToNextHour = 60 - m_timeinfo.tm_min;
            secondsLeftOfSleep = ((fullHoursToSleepEnd*3600) + (minsToNextHour*60)) - m_timeinfo.tm_sec;
            log_d("The sleep should last another %" PRIu64 " seconds from now", secondsLeftOfSleep);
            return true;
        }
    }
    log_d("The time is not within the overnight sleep period");
    return false;
}

String WiFiManager::getDayMonthStr()
{
    return m_dayMonth;
}

String WiFiManager::getTimeStr()
{
    return m_time;
}

time_t WiFiManager::getEpoch()
{
    return m_epoch;
}

String WiFiManager::getSsid()
{
    return m_ssid;
}

String WiFiManager::getAPIP()
{
    if (m_isAccessPoint)
        return WiFi.softAPIP().toString();
    else
        log_w("Access point init failed, cannot get IP");
    return "N/A - Error";
}

void WiFiManager::addDataSource(RequestBasePtr request)
{
    m_requests.push_back(std::move(request));
}

std::map<long, float> WiFiManager::getPriceData(const String& crypto, const String& fiat, std::set<long> unixOffsets)
{
    // try and get all data requested from a single data source
    // if it fails, retry from start with next data source

    // initialise a map with the required keys
    std::map<long, float> successRtn;
    for (const auto &i : unixOffsets)
    {
        successRtn[i];
    }

    for (const auto& request : m_requests)
    {
        log_d("Starting requests for symbol=%s fiat=%s using source %s", crypto.c_str(), fiat.c_str(), request->getServer().c_str());
        // for each data source, try and get price for each given unix offset
        // if one fails, move on to next data source

        // make sure this crypto/fiat is allowed for the data source
        if (!request->isValidRequest(crypto, fiat))
        {
            log_d("Crypto/fiat combo is not valid for this data source, moving on");
            continue;
        } 

        bool fullSuccess = false;
        for (auto& [key, value] : successRtn)
        {
            bool iSuccess = false;
            int retries = 0;
            // try to get price with retry
            while (!iSuccess && retries < 2)
            {
                log_d("Requesting price with unix offset %d", key);
                iSuccess = getPriceAtTime(crypto, fiat, key, value, request); // sets the value in successRtn for its unix offset
                retries++;
            }
            
            if (!iSuccess)
            {
                log_d("Request failed, will try next data source");
                fullSuccess = false;
                break;
            }
            fullSuccess = true;
                
        }
        // if fullSuccess = true here then we have all price data from a single source successfully
        if (fullSuccess)
            return successRtn;
        // else we carry on to next data source 
    }
    // if we get here then we never got all data from a single data source, return empty map
    return std::map<long, float>();
}

bool WiFiManager::getPriceAtTime(const String& crypto, const String& fiat, time_t unixOffset, 
                                 float& priceAtTime_out, const RequestBasePtr& request)
{        
    if (unixOffset == 0)
    {
        String url = request->urlCurrentPrice(crypto, fiat);
        return request->currentPrice(getUrlContent(request->getServer(), url), crypto, fiat, priceAtTime_out);
    }

    String url = request->urlPriceAtTime(m_epoch, unixOffset, crypto, fiat);
    return request->priceAtTime(getUrlContent(request->getServer(), url), priceAtTime_out);
}

String WiFiManager::getUrlContent(const String& server, const String& url)
{
    // check WL_CONNECTED as well as some time may have passed since initial connection 
    if (m_status != WiFiStatus::OK || WiFi.status() != WL_CONNECTED) 
        return "";

    String content;

    log_d("Starting connection to server with url %s", url.c_str());
    m_client.setInsecure(); //skip verification - binance only accepts https but we don't need it secure

    if (!m_client.connect(server.c_str(), 443))
        log_w("Connection failed");
    else 
    {
        log_d("Connected to server, sending HTTP request");
        // Make a HTTP request:
        m_client.println("GET " + url + " HTTP/1.0");
        m_client.println("Host: " + server);
        m_client.println("Connection: close");
        m_client.println();

        while (m_client.connected()) 
        {
            String line = m_client.readStringUntil('\n');
            if (line == "\r") 
            {
                log_d("Headers received");
                break;
            }
        }

        if (m_client.available())
        {
            while (m_client.available())
            {
                char c = m_client.read();
                content.concat(c);
            }
            log_d("Received content");
            return content;
        }

        m_client.stop();
    }

    return "";
}

String WiFiManager::generateConfigJs(const CurrentConfig& cfg)
{
    // creates a String containing a JavaScript struct of the given config, to be served with the config html 
    // to pre-populate inputs with the current values
    String configJs = "window.config = { ssid: \"";
    configJs += cfg.ssid;
    configJs += "\", pass: \"";
    configJs += cfg.pass;
    configJs += "\", crypto: \"";
    configJs += cfg.crypto;
    configJs += "\", fiat: \"";
    configJs += cfg.fiat;
    configJs += "\", refresh: \"";
    configJs += cfg.refreshMins;
    configJs += "\", tz: \"";
    configJs += cfg.tz;
    configJs += "\", display: \"";
    configJs += cfg.displayMode;
    configJs += "\", is24Hour: \"";
    configJs += cfg.is24Hour;
    configJs += "\", overnightStart: \"";
    configJs += cfg.overnightSleepStart;
    configJs += "\", overnightLength: \"";
    configJs += cfg.overnightSleepLength;
    configJs += "\"};";

    // var wifis = ["WiFi 1","WiFi 2"];
    configJs += "var wifis = [";
    for (const auto& s : m_scannedSsids)
    {
        configJs += "\"";
        configJs += s;
        configJs += "\",";
    }
    configJs += "];";

    configJs += "var deviceIdText = \"Device ID: ";
    configJs += utils::getDeviceID();
    configJs += "\";";
    
    configJs += "var versionNumberText = \"Firmware Version: V";
    configJs += constants::VersionNumber;
    configJs += "\";";
    
    return configJs;
}

void WiFiManager::initAllAvailableDataSources()
{
    m_requests.push_back(std::make_unique<RequestCoinGecko>());
    m_requests.push_back(std::make_unique<RequestKuCoin>());
    m_requests.push_back(std::make_unique<RequestBinance>());
}

} // namespace WiFiManagerLib
