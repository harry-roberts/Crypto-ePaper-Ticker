#ifndef REQUESTKUCOIN_H
#define REQUESTKUCOIN_H

#include "RequestBase.h"

#include <ArduinoJson.h>

String RequestKuCoin::getServer()
{
    return "api.kucoin.com";
}

String RequestKuCoin::urlCurrentPrice(const String& crypto, const String& fiat)
{
    String rtn;
    rtn.reserve(66); // 60 for BTC allow a few extra in case of longer symbol

    rtn += "https://api.kucoin.com/api/v1/prices?base=";
    rtn += fiat;
    rtn += "&currencies=";
    rtn += crypto;
    
    return rtn;
}

String RequestKuCoin::urlPriceAtTime(uint32_t currentUnix, uint32_t unixOffset, const String& crypto, const String& fiat)
{
    // https://api.kucoin.com/api/v1/market/candles?type=1min&symbol=BTC-USDT&startAt=1702653736&endAt=1702653796
    // usd must use USDT here

    // seems earliest GBP/EUR data is March 3 2023, cannot get 1 year data currently
    // USDT is ok

    uint32_t startTime = currentUnix - unixOffset;
    String rtn;
    rtn.reserve(112); // 106 for btc

    rtn += "https://api.kucoin.com/api/v1/market/candles?type=1min&symbol=";
    rtn += crypto;
    rtn += "-";
    rtn += (fiat == "USD") ? "USDT" : fiat; // KuCoin candles use USDT as symbol
    rtn += "&startAt=";
    rtn += startTime;
    rtn += "&endAt=";
    rtn += (startTime + 60);

    return rtn;
}

bool RequestKuCoin::currentPrice(const String& content, const String& crypto, const String& fiat, float& price_out)
{
    // {"code":"200000","data":{"BTC":"33388.8675121283881416"}}
    log_d("content = %s", content.c_str());
    DynamicJsonDocument doc(128); // https://arduinojson.org/v6/assistant/#/step1
    deserializeJson(doc, content);

    if (doc.containsKey("data"))
    {
        String price = doc["data"][crypto];
        if (!price.isEmpty())
        {
            price_out = price.toFloat();
            log_d("crypto: %s has price: %f", crypto.c_str(), price_out);
            return true;
        }
    }

    return false;
}

bool RequestKuCoin::priceAtTime(const String& content, float& priceAtTime_out)
{
    log_d("content = %s", content.c_str());

    if (content == "")
    {
        log_w("No content, returning");
        return false;
    }

    // for kucoin we will use the second element in the json tag "data"
    // content e.g. {"code":"200000","data":[["1702653780","32372.62","32372.62","32372.62","32372.62","0","0"]]}
    //                                                      ^^^^^^^^
    // might end up with 2 data points due to granularity errors but this is fine

    DynamicJsonDocument doc(256); // https://arduinojson.org/v6/assistant/#/step1
    deserializeJson(doc, content);

    if (doc.containsKey("data"))
    {
        JsonArray dataContent = doc["data"];
        priceAtTime_out = dataContent[0][1].as<float>();
        log_d("priceAtTime_out = %f", priceAtTime_out);

        if (priceAtTime_out > 0)
            return true;
    }

    return false;
}

#endif
