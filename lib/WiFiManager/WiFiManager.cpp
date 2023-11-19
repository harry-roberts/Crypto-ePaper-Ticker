#include "WiFiManager.h"
#include <ArduinoJson.h>
#include "Utils.h"

#include <AsyncElegantOTA.h>

#include "SPIFFS.h"

const char* CONFIG_PARAM_INPUT_SSID = "ssid";
const char* CONFIG_PARAM_INPUT_PASS = "pass";
const char* CONFIG_PARAM_INPUT_CRYPTO = "crypto";
const char* CONFIG_PARAM_INPUT_FIAT = "fiat";
const char* CONFIG_PARAM_INPUT_REFRESH = "refresh";

WiFiManager::WiFiManager(const String& ssid, const String& password) :
    m_ssid(ssid),
    m_password(password),
    m_request(new RequestBinance()),
    m_server(0), // unused in this mode
    m_isAccessPoint(false)
{
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

        log_d("Setting timezone to London");
        setenv("TZ", "GMT0BST,M3.5.0/1,M10.5.0", 1); // will be in config
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
            delay(1000);
        }
    }
}

WiFiManager::WiFiManager() :
    m_server(80)
{
    log_d("Creating access point for configuration");
    WiFi.mode(WIFI_AP);
    m_isAccessPoint = WiFi.softAP("Ticker", NULL);
    if (!m_isAccessPoint)
    {
        log_w("Access point init failed");
        return;
    }

    if (utils::initSpiffs())
    {
        // Web Server Root URL
        m_server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
        {
            request->send(SPIFFS, "/config.html", "text/html");
        });
        m_server.serveStatic("/", SPIFFS, "/");
        m_server.on("/", HTTP_POST, [this](AsyncWebServerRequest *request) 
        {
            int params = request->params();
            StaticJsonDocument<256> doc;
            for(int i = 0; i < params; i++)
            {
                AsyncWebParameter* p = request->getParam(i);
                if(p->isPost())
                {
                    if (p->name() == CONFIG_PARAM_INPUT_SSID)
                    {
                        doc["s"] = p->value().c_str();
                        log_d("SSID set to: %s", p->value().c_str());
                    }
                    if (p->name() == CONFIG_PARAM_INPUT_PASS)
                    {
                        doc["p"] = p->value().c_str();
                        log_d("Password set to: %s", p->value().c_str());
                    }
                    if (p->name() == CONFIG_PARAM_INPUT_CRYPTO)
                    {
                        doc["c"] = p->value().c_str();
                        log_d("Crypto set to: %s", p->value().c_str());
                    }
                    if (p->name() == CONFIG_PARAM_INPUT_FIAT)
                    {
                        doc["f"] = p->value().c_str();
                        log_d("Fiat set to: %s", p->value().c_str());
                    }
                    if (p->name() == CONFIG_PARAM_INPUT_REFRESH)
                    {
                        doc["r"] = p->value().c_str();
                        log_d("Refresh interval set to: %s", p->value().c_str());
                    }
                }
            }

            File file = SPIFFS.open("/config.json", FILE_WRITE);
            if (!file)
            {
                log_w("failed to open config file for writing");
                return;
            }

            if (doc.overflowed()) 
            {
                log_w("Json overflowed - some values will be missing");
                request->send(200, "text/plain", "Error: the config was too large and could not be saved. Restarting device");
            }
            else if (serializeJson(doc, file) == 0) 
            {
                log_w("Failed to write config to file");
                request->send(200, "text/plain", "An error occurred, the config was not saved. Restarting device");
            }
            else
                request->send(200, "text/plain", "Done. Restarting device");
        
            file.close();

            delay(2000);
            ESP.restart();
        });

        // this library adds a /update page to the existing web server, which handles an OTA file upload for new firmware (it just works)
        // ideal solution would be to connect to a remote web server to fetch a file itself, but this works fine for first version
        AsyncElegantOTA.begin(&m_server);

        m_server.begin();
    }
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
    char buf[20];
    strftime(buf, 20, "%e %b", &timeinfo);
    m_dayMonth = String(buf);

    char buf2[20];
    strftime(buf2, 20, "%H:%M", &timeinfo);
    m_time = String(buf2);

    time(&m_epoch);
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
    {
        log_w("Access point init failed, cannot get IP");
    }
    return "";
}

bool WiFiManager::isConnected()
{
    return (WiFi.status() == WL_CONNECTED) && (m_epoch > 0);
}

bool WiFiManager::getCurrentPrice(const String& crypto, const String& fiat, float& price_out)
{
    String url = m_request->urlCurrentPrice(crypto, fiat);
    return m_request->currentPrice(getUrlContent(m_request->getServer(), url), price_out);
}

bool WiFiManager::getPriceAtTime(const String& crypto, const String& fiat, time_t unixOffset, float& priceAtTime_out)
{
    String url = m_request->urlPriceAtTime(m_epoch-unixOffset, crypto, fiat);
    return m_request->priceAtTime(getUrlContent(m_request->getServer(), url), priceAtTime_out);
}

String WiFiManager::getUrlContent(const String& server, const String& url)
{
    if (!isConnected())
        return "";

    String content;

    log_d("Starting connection to server");
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
