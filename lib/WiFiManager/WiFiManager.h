#ifndef WIFIMANAGER_H
#define WIFIMANAGER_H

#include <Arduino.h>
#include <WiFiClientSecure.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include "time.h"

#include "RequestBase.h"

class WiFiManager
{
public:
    WiFiManager(const String& ssid, const String& password); // for connecting to known network
    WiFiManager(); // for access point

    bool isConnected(); // if connected to wifi and made a successful remote connection (have epoch time)

    bool getCurrentPrice(const String& crypto, const String& fiat, float& price_out);
    bool getPriceAtTime(const String& crypto, const String& fiat, time_t unixOffset, float& priceAtTime_out);

    String getDayMonthStr();
    String getTimeStr();
    time_t getEpoch();

    String getSsid();
    String getAPIP();

private:
    String getUrlContent(const String& server, const String& url);

    bool getTime(tm& timeinfo);
    void setTimeVars(tm& timeinfo);

    String m_ssid;
    String m_password;
    RequestBase* m_request;
    bool m_isAccessPoint;
    bool m_spiffsInit = false;

    String m_dayMonth = "Error"; // e.g "12 Oct"
    String m_time = "";          // e.g. 12:34 
    time_t m_epoch = -1;         // unix seconds

    WiFiClientSecure m_client;
    AsyncWebServer m_server;
};

#endif
