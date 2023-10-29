#ifndef WIFIMANAGER_H
#define WIFIMANAGER_H

#include <Arduino.h>
#include <WiFiClientSecure.h>
#include "time.h"

#include "RequestBase.h"

class WiFiManager
{
public:
    WiFiManager(const String& ssid, const String& password);

    bool isConnected();

    bool getCurrentPrice(const String& crypto, const String& fiat, float& price_out);
    bool getPriceAtTime(const String& crypto, const String& fiat, time_t unixOffset, float& priceAtTime_out);

    String getDayMonthStr();
    String getTimeStr();

    String getSsid();

private:
    String getUrlContent(const String& server, const String& url);

    bool getTime(tm& timeinfo);
    void setTimeVars(tm& timeinfo);

    String m_ssid;
    String m_password;
    RequestBase* m_request;

    String m_dayMonth = "Error"; // e.g "12 Oct"
    String m_time = "";          // e.g. 12:34 
    time_t m_epoch = -1;         // unix seconds

    WiFiClientSecure m_client;
};

#endif
