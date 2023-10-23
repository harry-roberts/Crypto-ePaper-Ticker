#ifndef REQUESTBINANCE_H
#define REQUESTBINANCE_H

#include "RequestBase.h"

#include <ArduinoJson.h>

String RequestBinance::getServer()
{
    return "api.binance.com";
}

String RequestBinance::urlCurrentPrice(const String& crypto, const String& fiat)
{
    // Binance prices USD with only USDT
    String urlFiat = (fiat == "USD") ? "USDT" : fiat;
    return "https://api.binance.com/api/v3/ticker/price?symbol=" + crypto + urlFiat;
}

String RequestBinance::urlPriceAtTime(uint32_t unix, const String& crypto, const String& fiat)
{
    // Binance prices USD with only USDT
    String urlFiat = (fiat == "USD") ? "USDT" : fiat;
    // binance takes milliseconds as unix time, add zeros
    // can get the price by requesting a 1m kline between the current time and current time+60
    // should return just one bar https://binance-docs.github.io/apidocs/spot/en/#kline-candlestick-data 
    return "https://api.binance.com/api/v3/klines?symbol=" + crypto + urlFiat + "&interval=1m&startTime=" + 
            String(unix) + "000&endTime=" + String(unix+60) + "000&limit=1";
}

String RequestBinance::urlCurrentTime()
{
    return "https://api.binance.com/api/v3/time";
}

float RequestBinance::currentPrice(const String& content)
{
    Serial.println("RequestBinance::currentPrice: content = " + content);
    DynamicJsonDocument doc(96); // https://arduinojson.org/v6/assistant/#/step1
    deserializeJson(doc, content);

    if (doc.containsKey("symbol") && doc.containsKey("price"))
    {
        String symbol = doc["symbol"];
        String price = doc["price"];
        Serial.println("symbol: " + symbol + " has price: " + price);
        return price.toFloat();
    }

    return -1;
}

uint32_t RequestBinance::currentTime(const String& content)
{
    Serial.println("RequestBinance::currentTime: content = " + content);
    DynamicJsonDocument doc(96); // https://arduinojson.org/v6/assistant/#/step1
    deserializeJson(doc, content);

    if (doc.containsKey("serverTime"))
    {
        String serverTime = doc["serverTime"];
        // binance gives milliseconds, remove last 3 digits
        serverTime = serverTime.substring(0, serverTime.length()-3);
        uint32_t serverTimeInt = static_cast<uint32_t>(std::stoul(serverTime.c_str()));
        Serial.print("server time string int: ");
        Serial.println(serverTimeInt);
        return serverTimeInt;
    }

    return 0;
}


#endif
