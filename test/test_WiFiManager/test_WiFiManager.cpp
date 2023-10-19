#include "WiFiManager.h"
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "Login.h"

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

TEST_F(WiFiManagerTest, badDetails)
{
    WiFiManager wm("not real", "xyz");
    EXPECT_EQ(wm.isConnected(), false);
}

TEST_F(WiFiManagerTest, data)
{
    WiFiManager wm(login_ssid, login_password);
    EXPECT_EQ(wm.isConnected(), true);
    EXPECT_THAT(wm.getCryptoPrice("BTC").c_str(), ::testing::MatchesRegex("[0-9]+.[0-9]+"));
}
