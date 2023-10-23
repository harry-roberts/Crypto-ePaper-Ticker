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
        Serial.println("Connected to " + m_ssid);
    }
}

String WiFiManager::getSsid()
{
    return m_ssid;
}

bool WiFiManager::isConnected()
{
    return WiFi.status() == WL_CONNECTED;
}

float WiFiManager::getCurrentPrice(const String& crypto, const String& fiat)
{
    String url = m_request->urlCurrentPrice(crypto, fiat);
    return m_request->currentPrice(getUrlContent(m_request->getServer(), url));
}

uint32_t WiFiManager::getCurrentTime()
{
    String url = m_request->urlCurrentTime();
    return m_request->currentTime(getUrlContent(m_request->getServer(), url));
}

String WiFiManager::getUrlContent(const String& server, const String& url)
{
    if (!isConnected())
        return "";

    String content;

    Serial.println("\nStarting connection to server");
    m_client.setInsecure(); //skip verification - binance only accepts https but we don't need it secure

    if (!m_client.connect(server.c_str(), 443))
        Serial.println("Connection failed");
    else 
    {
        Serial.println("Connected to server");
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
                Serial.println("Headers received");
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
            Serial.println("Received content");
            return content;
        }

        m_client.stop();
    }

    return "";
}
