#ifndef WIFIMANAGER_H
#define WIFIMANAGER_H

#include <Arduino.h>
#include <WiFiClientSecure.h>

#include "RequestBase.h"

class WiFiManager
{
public:
    WiFiManager(const String& ssid, const String& password);

    bool isConnected();
    float getCurrentPrice(const String& crypto, const String& fiat);
    uint32_t getCurrentTime(); // returns current unix seconds

    String getSsid();

private:
    String getUrlContent(const String& server, const String& url);

    String m_ssid;
    String m_password;
    RequestBase* m_request;

    WiFiClientSecure m_client;
};

#endif
