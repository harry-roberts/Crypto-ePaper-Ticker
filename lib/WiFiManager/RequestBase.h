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
    virtual String urlPriceAtTime(uint32_t unix, const String& crypto, const String& fiat) = 0;
    virtual String urlCurrentTime() = 0; // might need an outside function for this, ok for binance, confirm other sources

    // data functions
    virtual float currentPrice(const String& content) = 0;
    virtual uint32_t currentTime(const String& content) = 0;

    // **Note** unix time between all functions should be consistent as SECONDS
};

class RequestBinance : public RequestBase
{
public:
    // defines functions as needed for the Binance API
    String getServer() override;

    String urlCurrentPrice(const String& crypto, const String& fiat) override;
    String urlPriceAtTime(uint32_t unix, const String& crypto, const String& fiat) override;
    String urlCurrentTime() override;

    float currentPrice(const String& content) override;
    uint32_t currentTime(const String& content) override;
};

#endif
