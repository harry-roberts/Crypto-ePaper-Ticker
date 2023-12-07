#ifndef CONSTANTS_H
#define CONSTANTS_H

namespace constants
{
    inline constexpr long SecondsOneDay = 86400;
    inline constexpr long SecondsOneMonth = 2592000;
    inline constexpr long SecondsOneYear = 31536000;

    inline constexpr float BatteryMaxVoltage = 4.15;
    inline constexpr float BatteryMinVoltage = 3.4;

    inline constexpr const char* WifiAccessPointName = "Ticker";

    inline constexpr const char* ConfigKeySsid = "s";
    inline constexpr const char* ConfigKeyPassword = "p";
    inline constexpr const char* ConfigKeyCrypto = "c";
    inline constexpr const char* ConfigKeyFiat = "f";
    inline constexpr const char* ConfigKeyRefreshMins = "r";
    inline constexpr const char* ConfigKeyTimezone = "t";
}

#endif
