#include "WiFiManager.h"
#include "RequestBase.h"
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "Login.h"
#include "compile_time.h"

class WiFiManagerTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // Serial output required - see note in main
        // new line also helps with test formatting in serial monitor
        Serial.println();
    }

};

TEST_F(WiFiManagerTest, DISABLED_badDetails)
{
    WiFiManager wm("not real", "xyz");
    EXPECT_EQ(wm.isConnected(), false);
}

TEST_F(WiFiManagerTest, data)
{
    WiFiManager wm(login_ssid, login_password);
    EXPECT_EQ(wm.isConnected(), true);

    float currentPrice;
    EXPECT_TRUE(wm.getCurrentPrice("BTC", "USD", currentPrice));
    EXPECT_GT(currentPrice, 0);

    // see compile_time.h
    // expect time is within 24h of test compile time    
    EXPECT_GT(wm.getEpoch(), UNIX_TIMESTAMP-SEC_PER_DAY);
    EXPECT_LT(wm.getEpoch(), UNIX_TIMESTAMP+SEC_PER_DAY);
}

TEST_F(WiFiManagerTest, requests)
{
    std::vector<RequestBase*> rfs;
    rfs.push_back(new RequestBinance());

    EXPECT_EQ(rfs.at(0)->getServer(), "api.binance.com");
}

TEST_F(WiFiManagerTest, ssid)
{
    WiFiManager wm(login_ssid, login_password);
    EXPECT_EQ(wm.getSsid(), login_ssid);
}
