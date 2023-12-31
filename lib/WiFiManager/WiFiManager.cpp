#include "WiFiManager.h"
#include <ArduinoJson.h>
#include "Constants.h"

#include <AsyncElegantOTA.h>

#include "SPIFFS.h"

void WiFiManager::initNormalMode(const CurrentConfig& cfg, bool initAllDataSources)
{
    m_ssid = cfg.ssid;
    m_password = cfg.pass;
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

        struct tm timeinfo;

        log_d("Setting timezone to %s", cfg.tz.c_str());
        setenv("TZ", cfg.tz.c_str(), 1); // will be in config
        tzset();

        bool gotTime = false;
        int timeRetries = 0;

        while (!gotTime && timeRetries < 10)
        {
            gotTime = getTime(timeinfo);
            if (gotTime)
            {
                setTimeVars(timeinfo);
                log_d("Time: %s %s", m_dayMonth.c_str(), m_time.c_str());
                log_d("Epoch: %d", m_epoch);
            }
            timeRetries++;
            delay(500);
        }
    }
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
            StaticJsonDocument<512> doc;
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
            else if (doc[constants::ConfigKeySsid].isNull())
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
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
}

bool WiFiManager::getTime(tm& timeinfo)
{
    if(!getLocalTime(&timeinfo))
    {
        log_w("Failed to obtain time");
        return false;
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
        strftime(buf, 20, "%H:%M", &timeinfo);
        m_time = String(buf);
    }

    time(&m_epoch);
}

void WiFiManager::refreshTime()
{
    log_d("Refreshing stored time");
    struct tm timeinfo;

    if (getTime(timeinfo))
    {
        setTimeVars(timeinfo);
        log_d("Time: %s %s", m_dayMonth.c_str(), m_time.c_str());
        log_d("Epoch: %d", m_epoch);
    }
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

bool WiFiManager::isConnected()
{
    return (WiFi.status() == WL_CONNECTED);
}

bool WiFiManager::hasInternet()
{
    // we can be confident the NTP pool server will not be down
    // if we have an epoch time, we have an internet connection
    return m_epoch > 0;
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

        // kucoin can only do USD for full data
        if (request->getServer() == "api.kucoin.com" && fiat != "USD")
        {
            log_d("Non USD fiat requested from KuCoin, moving to next source");
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
    if (!isConnected() || !hasInternet())
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

String WiFiManager::generateConfigJs(CurrentConfig cfg)
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
    
    return configJs;
}

void WiFiManager::initAllAvailableDataSources()
{
    m_requests.push_back(std::make_unique<RequestCoinGecko>());
    m_requests.push_back(std::make_unique<RequestKuCoin>());
    m_requests.push_back(std::make_unique<RequestBinance>());
}
