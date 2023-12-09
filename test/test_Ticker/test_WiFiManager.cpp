#include "WiFiManager.h"
#include "RequestBase.h"
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "Login.h"
#include "compile_time.h"
#include "Constants.h"

class WiFiManagerTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // Serial output required - see note in main
        // new line also helps with test formatting in serial monitor
        Serial.println();
    }

    CurrentConfig cfg{login_ssid, login_password, "BTC", "USD", "5", "GMT0BST,M3.5.0/1,M10.5.0"};
};

TEST_F(WiFiManagerTest, badDetails)
{
    WiFiManager wm;
    CurrentConfig badCfg = cfg;
    badCfg.ssid = "not real";
    badCfg.pass = "wrong";
    wm.initNormalMode(badCfg);
    EXPECT_EQ(wm.isConnected(), false);
}

TEST_F(WiFiManagerTest, data)
{
    WiFiManager wm;
    wm.initNormalMode(cfg);
    EXPECT_EQ(wm.getSsid(), login_ssid);
    EXPECT_EQ(wm.isConnected(), true);

    // test every available data source
    float price;
    size_t numDataSources = wm.getNumDataSources();
    for (size_t i = 0; i < numDataSources; i++)
    {
        EXPECT_TRUE(wm.getPriceAtTime(cfg.crypto, cfg.fiat, 0, price, i));
        delay(500);
        EXPECT_TRUE(wm.getPriceAtTime(cfg.crypto, cfg.fiat, constants::SecondsOneDay, price, i));
        delay(500);
        EXPECT_TRUE(wm.getPriceAtTime(cfg.crypto, cfg.fiat, constants::SecondsOneMonth, price, i));
        delay(500);
        EXPECT_TRUE(wm.getPriceAtTime(cfg.crypto, cfg.fiat, constants::SecondsOneYear, price, i));
        delay(500);
    }

    // see compile_time.h
    // expect time is within 24h of test compile time    
    EXPECT_GT(wm.getEpoch(), UNIX_TIMESTAMP-SEC_PER_DAY);
    EXPECT_LT(wm.getEpoch(), UNIX_TIMESTAMP+SEC_PER_DAY);
}

TEST_F(WiFiManagerTest, requests)
{
    std::vector<RequestBasePtr> rfs;
    rfs.push_back(std::make_unique<RequestBinance>());
    rfs.push_back(std::make_unique<RequestCoinGecko>());

    EXPECT_EQ(rfs.at(0)->getServer(), "api.binance.com");
    EXPECT_EQ(rfs.at(1)->getServer(), "api.coingecko.com");
}

TEST_F(WiFiManagerTest, testBinance)
{
    RequestBasePtr binance(new RequestBinance());
    const String currentPriceContent = "{\"symbol\":\"BTCGBP\",\"price\":\"29396.32000000\"}";
    const String priceAtTimeContent = "[[1697382420000,\"22138.72000000\",\"22138.72000000\",\"22138.72000000\",\"22138.72000000\",\"0.00000000\",1697382479999,\"0.00000000\",0,\"0.00000000\",\"0.00000000\",\"0\"]]";
    float currentPrice_out;
    float timePrice_out;

    EXPECT_EQ(binance->urlCurrentPrice("BTC", "GBP"), "https://api.binance.com/api/v3/ticker/price?symbol=BTCGBP");
    EXPECT_EQ(binance->urlCurrentPrice("BTC", "USD"), "https://api.binance.com/api/v3/ticker/price?symbol=BTCUSDT");
    EXPECT_EQ(binance->urlPriceAtTime(1701021297, constants::SecondsOneDay, "BTC", "GBP"),
                  "https://api.binance.com/api/v3/klines?symbol=BTCGBP&interval=1m&startTime=1700934897000&endTime=1700934957000&limit=1");

    EXPECT_TRUE(binance->currentPrice(currentPriceContent, "BTC", "GBP", currentPrice_out));
    EXPECT_NEAR(currentPrice_out, 29396.32, 0.1);

    EXPECT_TRUE(binance->priceAtTime(priceAtTimeContent, timePrice_out));
    EXPECT_NEAR(timePrice_out, 22138.72, 0.1);
}

TEST_F(WiFiManagerTest, testCoinGecko)
{
    RequestBasePtr coingecko(new RequestCoinGecko());
    const String currentPriceContent = "{\"bitcoin\":{\"gbp\":29319.1767}}";
    const String priceAtTimeContent = "{\"prices\":[[1701021346883,29585.3913]],\"market_caps\":[[1701021346883,578750969047.6592]],\"total_volumes\":[[1701021346883,8726978835.980974]]}";
    float currentPrice_out;
    float timePrice_out;

    EXPECT_EQ(coingecko->urlCurrentPrice("BTC", "GBP"), "https://api.coingecko.com/api/v3/simple/price?ids=bitcoin&vs_currencies=GBP&precision=4");
    EXPECT_EQ(coingecko->urlCurrentPrice("BTC", "USD"), "https://api.coingecko.com/api/v3/simple/price?ids=bitcoin&vs_currencies=USD&precision=4");
    EXPECT_EQ(coingecko->urlPriceAtTime(1701021297, constants::SecondsOneDay, "BTC", "GBP"), 
                  "https://api.coingecko.com/api/v3/coins/bitcoin/market_chart/range?vs_currency=GBP&from=1700934897&to=1700935497&precision=4");

    EXPECT_TRUE(coingecko->currentPrice(currentPriceContent, "BTC", "GBP", currentPrice_out));
    EXPECT_NEAR(currentPrice_out, 29319.18, 0.1);

    EXPECT_TRUE(coingecko->priceAtTime(priceAtTimeContent, timePrice_out));
    EXPECT_NEAR(timePrice_out, 29585.39, 0.1);
}
