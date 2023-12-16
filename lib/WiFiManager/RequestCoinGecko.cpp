#ifndef RequestCoinGecko_H
#define RequestCoinGecko_H

#include "RequestBase.h"
#include "Constants.h"

#include <ArduinoJson.h>
#include <map>

namespace
{
    // map of crypto symbol to coingecko id, e.g. BTC -> bitcoin
    // the coingecko api cannot be called with symbol
    std::map<String, String> coinGeckoSymbolToId = {{"ADA",  "cardano"},
                                                    {"BNB",  "binancecoin"},
                                                    {"BTC",  "bitcoin"},
                                                    {"DOGE", "dogecoin"},
                                                    {"ETH",  "ethereum"},
                                                    {"LINK", "chainlink"},
                                                    {"SOL",  "solana"},
                                                    {"XRP",  "ripple"}};
}


String RequestCoinGecko::getServer()
{
    return "api.coingecko.com";
}

String RequestCoinGecko::urlCurrentPrice(const String& crypto, const String& fiat)
{
    // {"bitcoin":{"gbp":33357.5612}}
    // https://api.coingecko.com/api/v3/simple/price?ids=bitcoin&vs_currencies=gbp&precision=4
    // precision has to be specified otherwise it isn't as helpful as binance
    String rtn;
    rtn.reserve(96); // can vary a bit more due to id instead of symbol

    rtn += "https://api.coingecko.com/api/v3/simple/price?ids=";
    rtn += coinGeckoSymbolToId[crypto];
    rtn += "&vs_currencies=";
    rtn += fiat;
    rtn += "&precision=4";

    return rtn;
}

String RequestCoinGecko::urlPriceAtTime(uint32_t currentUnix, uint32_t unixOffset, const String& crypto, const String& fiat)
{
    // /coins/{id}/market_chart/range
    // https://api.coingecko.com/api/v3/coins/bitcoin/market_chart/range?vs_currency=gbp&from=1701021297&to=1701021597&precision=4
    // granularity is based on time offset:
    //     1 day from current time = 5 minute interval data
    //     2 - 90 days of date range = hourly data
    //     above 90 days of date range = daily data (00:00 UTC)
    // so to get 1 we need to request the epoch - offset until that + the interval for our offset
    // in reality the granularity can vary slightly, so the exact amount will sometimes return nothing if the window doesn't
    // cross a data point, so add an extra 5 minutes to each

    uint32_t startTime = currentUnix - unixOffset;
    uint32_t endTime;

    switch (unixOffset)
    {
        case constants::SecondsOneDay:
            endTime = startTime + 300 + 300;
            break;
        case constants::SecondsOneMonth:
            endTime = startTime + 3600 + 300;
            break;
        case constants::SecondsOneYear:
        default:
            endTime = startTime + 3600 + 300; // seems that it actually gives data hourly even at 1 year despite info above
            break;
    }

    String rtn;
    rtn.reserve(128); // expect it is ~123 but allow a few extra in case of longer symbol

    rtn += "https://api.coingecko.com/api/v3/coins/";
    rtn += coinGeckoSymbolToId[crypto];
    rtn += "/market_chart/range?vs_currency=";
    rtn += fiat;
    rtn += "&from=";
    rtn += startTime;
    rtn += "&to=";
    rtn += endTime;
    rtn += "&precision=4";

    return rtn;
}

bool RequestCoinGecko::currentPrice(const String& content, const String& crypto, const String& fiat, float& price_out)
{
    log_d("content = %s", content.c_str());
    DynamicJsonDocument doc(64); // https://arduinojson.org/v6/assistant/#/step1
    deserializeJson(doc, content);

    String accessString = fiat;
    accessString.toLowerCase(); // coingecko converts all fiat symbols to lower case

    if (doc.containsKey(coinGeckoSymbolToId[crypto]))
    {
        String price = doc[coinGeckoSymbolToId[crypto]][accessString];
        price_out = price.toFloat();
        log_d("symbol: %s has price: %f", coinGeckoSymbolToId[crypto].c_str(), price_out);
        return true;
    }

    return false;
}

bool RequestCoinGecko::priceAtTime(const String& content, float& priceAtTime_out)
{
    log_d("content = %s", content.c_str());

    if (content == "")
    {
        log_w("No content, returning");
        return false;
    }

    // for coingecko we will use the first price returned in the json tag "prices"
    // content e.g. {"prices":[[1701021346883,29585.391271772718]],
    //                                        ^^^^^^^^^^^^^^^^^^
    //               "market_caps":[[1701021346883,578750969047.6592]],
    //               "total_volumes":[[1701021346883,8726978835.980974]]}
    // might end up with 2 data points due to granularity errors but this is fine

    DynamicJsonDocument doc(384); // https://arduinojson.org/v6/assistant/#/step1
    deserializeJson(doc, content);

    priceAtTime_out = 0;
    if (doc.containsKey("prices"))
    {
        JsonArray pricesContent = doc["prices"];
        if (pricesContent.size() && pricesContent[0].size() == 2)
        {
            priceAtTime_out = pricesContent[0][1].as<float>();
        }
        log_d("priceAtTime_out = %f", priceAtTime_out);

        if (priceAtTime_out > 0)
            return true;
    }

    return false;
}

#endif
