#ifndef WIFIMANAGER_H
#define WIFIMANAGER_H

#include <Arduino.h>
#include <WiFiClientSecure.h>

class WiFiManager
{
public:
    WiFiManager(String ssid, String password);

    bool isConnected();
    float getCryptoPrice(String crypto);

private:
    String m_ssid;
    String m_password;

    WiFiClientSecure m_client;
};

#endif
