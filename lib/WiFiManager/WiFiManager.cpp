#include "WiFiManager.h"
#include <ArduinoJson.h>

WiFiManager::WiFiManager(const String& ssid, const String& password) :
    m_ssid(ssid),
    m_password(password),
    m_request(new RequestBinance())
{
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
