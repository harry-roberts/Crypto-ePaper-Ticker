#include "WiFiManager.h"
#include <ArduinoJson.h>

WiFiManager::WiFiManager(String ssid, String password) :
    m_ssid(ssid),
    m_password(password)
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

bool WiFiManager::isConnected()
{
    return WiFi.status() == WL_CONNECTED;
}

float WiFiManager::getCryptoPrice(String crypto)
{
    if (!isConnected())
        return -1;

    const char* server = "api.binance.com";
    String content;
    DynamicJsonDocument doc(96); // https://arduinojson.org/v6/assistant/#/step1

    Serial.println("\nStarting connection to server");
    m_client.setInsecure(); //skip verification - binance only accepts https but we don't need it secure

    if (!m_client.connect(server, 443))
        Serial.println("Connection failed");
    else 
    {
        Serial.println("Connected to server");
        // Make a HTTP request:
        m_client.println("GET https://api.binance.com/api/v3/ticker/price?symbol=" + crypto + "USDT HTTP/1.0"); // need to find a way to add £ symbol
        m_client.println("Host: " + String(server));
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
            Serial.println("Reading content to JSON");
            deserializeJson(doc, content);

            if (doc.containsKey("symbol") && doc.containsKey("price"))
            {
                String symbol = doc["symbol"];
                String price = doc["price"];
                Serial.println("symbol: " + symbol + " has price: " + price);
                return price.toFloat();
            }
            
        }

        m_client.stop();
    }

    return -1;
}
