#ifndef WIFIMANAGER_H
#define WIFIMANAGER_H

#include <Arduino.h>
#include <WiFiClientSecure.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include "time.h"
#include "Utils.h"

#include "RequestBase.h"

#include <memory>
#include <map>
#include <set>

using utils::CurrentConfig;

class WiFiManager
{
public:
    WiFiManager() = default;

    void initConfigMode(const CurrentConfig& cfg, int port); // configures access point
    void initNormalMode(const CurrentConfig& cfg, bool initAllDataSources = true); // connects to known network

    bool isConnected(); // is connected to wifi
    bool hasInternet(); // have made a successful remote connection (have epoch time)

    // input set of unix offsets to get data for
    // return map of unix offsets to price, or empty map if failed
    std::map<long, float> getPriceData(const String& crypto, const String& fiat, std::set<long> unixOffsets);

    String getDayMonthStr();
    String getTimeStr();
    time_t getEpoch();
    void refreshTime();

    String getSsid();
    String getAPIP();

    void disconnect();

    void addDataSource(RequestBasePtr request);

private:
    String getUrlContent(const String& server, const String& url);
    void initAllAvailableDataSources();

    bool getPriceAtTime(const String& crypto, const String& fiat, time_t unixOffset, float& priceAtTime_out, const RequestBasePtr& request);
    bool getTime(tm& timeinfo);
    void setTimeVars(tm& timeinfo);
    String generateConfigJs(CurrentConfig cfg);

    String m_ssid;
    String m_password;
    std::vector<RequestBasePtr> m_requests;
    bool m_isAccessPoint;
    bool m_spiffsInit = false;
    std::set<String> m_scannedSsids;

    String m_dayMonth = "Error"; // e.g. "12 Oct"
    String m_time = "";          // e.g. "12:34" 
    time_t m_epoch = -1;         // unix seconds

    WiFiClientSecure m_client;
    std::unique_ptr<AsyncWebServer> m_server;
};

#endif
