#ifndef REQUESTBINANCE_H
#define REQUESTBINANCE_H

#include "RequestBase.h"

#include <ArduinoJson.h>

String RequestBinance::getServer()
{
    return "api.binance.com";
}

bool RequestBinance::isValidRequest(const String& crypto, const String& fiat)
{
    // binance cannot do GBP
    if (fiat == "GBP") return false;
    return true;
}

String RequestBinance::urlCurrentPrice(const String& crypto, const String& fiat)
{
    String rtn;
    rtn.reserve(64); // expect it is 58 but allow a few extra in case of longer symbol

    rtn += "https://api.binance.com/api/v3/ticker/price?symbol=";
    rtn += crypto;
    rtn += (fiat == "USD") ? "USDT" : fiat; // Binance prices USD with only USDT

    return rtn;
}

String RequestBinance::urlPriceAtTime(uint32_t currentUnix, uint32_t unixOffset, const String& crypto, const String& fiat)
{
    // binance takes milliseconds as unix time, add zeros
    // can get the price by requesting a 1m kline between the current time and current time+60
    // should return just one bar https://binance-docs.github.io/apidocs/spot/en/#kline-candlestick-data 

    // **
    // Binance has now stopped dealing GBP - can get current price but no longer usable for historic data
    // **

    uint32_t startTime = currentUnix - unixOffset;
    String rtn;
    rtn.reserve(124); // expect it is 118 but allow a few extra in case of longer symbol

    rtn += "https://api.binance.com/api/v3/klines?symbol=";
    rtn += crypto;
    rtn += (fiat == "USD") ? "USDT" : fiat; // Binance prices USD with only USDT
    rtn += "&interval=1m&startTime=";
    rtn += startTime;
    rtn += "000&endTime=";
    rtn += (startTime + 60);
    rtn += "000&limit=1";

    return rtn;
}

bool RequestBinance::currentPrice(const String& content, const String& crypto, const String& fiat, float& price_out)
{
    log_d("content = %s", content.c_str());
    DynamicJsonDocument doc(96); // https://arduinojson.org/v6/assistant/#/step1
    deserializeJson(doc, content);

    if (doc.containsKey("symbol") && doc.containsKey("price"))
    {
        String symbol = doc["symbol"];
        String price = doc["price"];
        price_out = price.toFloat();
        log_d("symbol: %s has price: %f", symbol.c_str(), price_out);
        return true;
    }

    return false;
}

bool RequestBinance::priceAtTime(const String& content, float& priceAtTime_out)
{
    log_d("content = %s", content.c_str());

    if (content == "" || content.charAt(0) != '[') // errors will have some json starting with {, data is only an array
    {
        log_w("Bad content, returning");
        return false;
    }

    // for binance we will use the open price of this kline
    // content e.g:
    // [[1697382420000,"22138.72000000","22138.72000000","22138.72000000","22138.72000000","0.00000000",1697382479999,"0.00000000",0,"0.00000000","0.00000000","0"]]
    // we want this     ^^^^^^^^^^^^^^

    StaticJsonDocument<384> doc;
    deserializeJson(doc, content);

    JsonArray dataArray = doc.as<JsonArray>();

    priceAtTime_out = 0;
    if (dataArray.size() && dataArray[0].size() == 12)
    {
        priceAtTime_out = dataArray[0][1].as<float>();
    }
    log_d("priceAtTime_out = %f", priceAtTime_out);

    if (priceAtTime_out > 0)
        return true;

    return false;
}

#endif
