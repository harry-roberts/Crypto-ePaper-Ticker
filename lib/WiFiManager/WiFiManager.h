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

namespace WiFiManagerLib
{
using utils::CurrentConfig;

enum class WiFiStatus
{
    OK,            // has connection with successful internet connection
    NO_CONNECTION, // couldn't connect to the network
    NO_INTERNET,   // connected to network but no internet
    UNKNOWN        // before attempting
};

enum class AdminAction
{
    REQUEST_BTC,   // force BTC display update even with no saved config
    FORMAT_SPIFFS, // force spiffs format
    FILL_SCREEN,   // draw every pixel black
    NONE           // default
};

struct AdminRequest
{
    String ssid;
    String password;
    AdminAction action = AdminAction::NONE;
    bool set = false; // after we have finished looking at all params from the POST set this true
};

class WiFiManager
{
public:
    WiFiManager() = default;

    void initConfigMode(const CurrentConfig& cfg, int port); // configures access point
    WiFiStatus initNormalMode(const CurrentConfig& cfg, bool waitForNtpSync = false, bool initAllDataSources = true); // connects to known network

    // input set of unix offsets to get data for
    // return map of unix offsets to price, or empty map if failed
    std::map<long, float> getPriceData(const String& crypto, const String& fiat, std::set<long> unixOffsets);

    String getDayMonthStr();
    String getTimeStr();
    time_t getEpoch();
    bool isCurrentTimeDuringOvernightSleep(int sleepStartHour, int sleepHoursLength, uint64_t& secondsLeftOfSleep);
    void refreshTime();

    String getSsid();
    String getAPIP();

    void disconnect();

    void addDataSource(RequestBasePtr request);

    void setTimeInfo(int h, int m, int s);

    AdminRequest getAdminRequest();
    void resetAdminRequest();

private:
    String getUrlContent(const String& server, const String& url);
    void initAllAvailableDataSources();

    bool getPriceAtTime(const String& crypto, const String& fiat, time_t unixOffset, float& priceAtTime_out, const RequestBasePtr& request);
    bool getTime(tm& timeinfo, bool waitForNtpSync = false);
    void setTimeVars(tm& timeinfo);
    String generateConfigJs(const CurrentConfig& cfg);

    WiFiStatus m_status = WiFiStatus::UNKNOWN;

    String m_ssid;
    String m_password;
    std::vector<RequestBasePtr> m_requests;
    bool m_isAccessPoint = false;
    std::set<String> m_scannedSsids;

    String m_dayMonth = "Error"; // e.g. "12 Oct"
    String m_time = "";          // e.g. "12:34" or "01:23pm"
    time_t m_epoch = -1;         // unix seconds
    bool m_is24Hour = true;
    struct tm m_timeinfo{};

    WiFiClientSecure m_client;
    std::unique_ptr<AsyncWebServer> m_server;

    AdminRequest m_adminRequest;
};

} // namespace WiFiManagerLib

#endif
