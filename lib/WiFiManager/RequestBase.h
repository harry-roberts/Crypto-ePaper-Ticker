#ifndef REQUESTBASE_H
#define REQUESTBASE_H

#include <Arduino.h>

class RequestBase
{
public:
    // this class is a base for a data source that will have its own implementations of these functions

    // has functions for:
    //   - generating a url for a given request type
    //   - taking content received from this url and returning data of interest

    virtual String getServer() = 0;

    // url functions
    virtual String urlCurrentPrice(const String& crypto, const String& fiat) = 0;
    virtual String urlPriceAtTime(uint32_t currentUnix, uint32_t unixOffset, const String& crypto, const String& fiat) = 0;

    // data functions
    virtual bool currentPrice(const String& content, const String& crypto, const String& fiat, float& price_out) = 0;
    virtual bool priceAtTime(const String& content, float& priceAtTime_out) = 0;

    // **Note** unix time between all functions should be consistent as SECONDS
};

class RequestBinance : public RequestBase
{
public:
    // defines functions as needed for the Binance API
    String getServer() override;

    String urlCurrentPrice(const String& crypto, const String& fiat) override;
    String urlPriceAtTime(uint32_t currentUnix, uint32_t unixOffset, const String& crypto, const String& fiat) override;

    bool currentPrice(const String& content, const String& crypto, const String& fiat, float& price_out) override;
    bool priceAtTime(const String& content, float& priceAtTime_out) override;
};

class RequestCoinGecko : public RequestBase
{
public:
    // defines functions as needed for the CoinGecko API
    String getServer() override;

    String urlCurrentPrice(const String& crypto, const String& fiat) override;
    String urlPriceAtTime(uint32_t currentUnix, uint32_t unixOffset, const String& crypto, const String& fiat) override;

    bool currentPrice(const String& content, const String& crypto, const String& fiat, float& price_out) override;
    bool priceAtTime(const String& content, float& priceAtTime_out) override;
};

#endif
