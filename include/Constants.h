#ifndef CONSTANTS_H
#define CONSTANTS_H

namespace constants
{
    inline constexpr const long SecondsOneDay = 86400;
    inline constexpr const long SecondsOneMonth = 2592000;
    inline constexpr const long SecondsOneYear = 31536000;

    inline constexpr const float BatteryMaxVoltage = 4.15;
    inline constexpr const float BatteryMinVoltage = 3.4;

    inline constexpr const char* WifiAccessPointName = "Ticker";

    inline constexpr const char* ConfigKeySsid = "s";
    inline constexpr const char* ConfigKeyPassword = "p";
    inline constexpr const char* ConfigKeyCrypto = "c";
    inline constexpr const char* ConfigKeyFiat = "f";
    inline constexpr const char* ConfigKeyRefreshMins = "r";
    inline constexpr const char* ConfigKeyTimezone = "t";

    inline constexpr const int WiFiRequestRetries = 2;

    inline constexpr const int MicrosToSecondsFactor = 1000000;

    inline constexpr const int SleepSecondsAfterDataFail = 300;
    inline constexpr const int SleepSecondsAfterWiFiFail = 600;
}

#endif
