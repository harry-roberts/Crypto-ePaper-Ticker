#ifndef REQUESTBASE_H
#define REQUESTBASE_H

#include <Arduino.h>
#include <memory>

class RequestBase
{
public:
    // this class is a base for a data source that will have its own implementations of these functions

    // has functions for:
    //   - generating a url for a given request type
    //   - taking content received from this url and returning data of interest

    virtual ~RequestBase() = default;

    virtual String getServer() = 0;

    // url functions
    virtual String urlCurrentPrice(const String& crypto, const String& fiat) = 0;
    virtual String urlPriceAtTime(uint32_t currentUnix, uint32_t unixOffset, const String& crypto, const String& fiat) = 0;

    // data functions
    virtual bool currentPrice(const String& content, const String& crypto, const String& fiat, float& price_out) = 0;
    virtual bool priceAtTime(const String& content, float& priceAtTime_out) = 0;

    // some sources will have restrictions on which cryptos/fiats are available
    // crypto restrictions will be complex, probably just allow these requests to fail. Before making a crypto available,
    // will just make sure it is available form at least 1 data source
    virtual bool isValidRequest(const String& crypto, const String& fiat) = 0;

    // **Note** unix time between all functions should be consistent as SECONDS
};

using RequestBasePtr = std::unique_ptr<RequestBase>;

class RequestBinance : public RequestBase
{
public:
    // defines functions as needed for the Binance API
    String getServer() override;

    String urlCurrentPrice(const String& crypto, const String& fiat) override;
    String urlPriceAtTime(uint32_t currentUnix, uint32_t unixOffset, const String& crypto, const String& fiat) override;

    bool currentPrice(const String& content, const String& crypto, const String& fiat, float& price_out) override;
    bool priceAtTime(const String& content, float& priceAtTime_out) override;

    bool isValidRequest(const String& crypto, const String& fiat) override;
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

    bool isValidRequest(const String& crypto, const String& fiat) override;
};

class RequestKuCoin : public RequestBase
{
public:
    // defines functions as needed for the KuCoin API
    String getServer() override;

    String urlCurrentPrice(const String& crypto, const String& fiat) override;
    String urlPriceAtTime(uint32_t currentUnix, uint32_t unixOffset, const String& crypto, const String& fiat) override;

    bool currentPrice(const String& content, const String& crypto, const String& fiat, float& price_out) override;
    bool priceAtTime(const String& content, float& priceAtTime_out) override;

    bool isValidRequest(const String& crypto, const String& fiat) override;
};

#endif
