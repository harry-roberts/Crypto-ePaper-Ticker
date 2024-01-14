#ifndef CONSTANTS_H
#define CONSTANTS_H

namespace constants
{
    inline constexpr const char* VersionNumber = "1.0";

    inline constexpr const long SecondsOneDay = 86400;
    inline constexpr const long SecondsOneMonth = 2592000;
    inline constexpr const long SecondsOneYear = 31536000;

    inline constexpr const char* WifiAccessPointName = "Ticker";

    inline constexpr const char* ConfigKeySsid = "s";
    inline constexpr const char* ConfigKeyPassword = "p";
    inline constexpr const char* ConfigKeyCrypto = "c";
    inline constexpr const char* ConfigKeyFiat = "f";
    inline constexpr const char* ConfigKeyRefreshMins = "r";
    inline constexpr const char* ConfigKeyTimezone = "t";
    inline constexpr const char* ConfigKeyDisplayMode = "d";
    inline constexpr const char* ConfigKeyTimeFormat = "o";

    inline constexpr const char* ConfigDisplayModeSimple = "simple";
    inline constexpr const char* ConfigDisplayModeAdvanced = "advanced";

    inline constexpr const int WiFiRequestRetries = 2;

    inline constexpr const int MicrosToSecondsFactor = 1000000;

    inline constexpr const int SleepSecondsAfterWiFiFailLevels = 6;
    inline constexpr const int SleepSecondsAfterWiFiFail[SleepSecondsAfterWiFiFailLevels] = {60, 120, 300, 600, 1800, 3600};
    inline constexpr const int SleepSecondsAfterDataFailLevels = 4;
    inline constexpr const int SleepSecondsAfterDataFail[SleepSecondsAfterDataFailLevels] = {60, 120, 300, 600};

    inline constexpr const int NormalAlertTimeSeconds = 150;
    inline constexpr const int ConfigAlertTimeSeconds = 600;

    inline constexpr const char* SpiffsConfigFileName = "/config.json";
    inline constexpr const char* SpiffsBatLogFileName = "/low_battery.txt";
}

#endif
