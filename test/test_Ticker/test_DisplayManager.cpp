#include "DisplayManager.h"
#include <gtest/gtest.h>
#include <gmock/gmock.h>

class DisplayManagerTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // Serial output required - see note in main
        // new line also helps with test formatting in serial monitor
        Serial.println();
    }

};

TEST_F(DisplayManagerTest, formatPrice)
{
    DisplayManager dm;
    EXPECT_EQ(dm.formatPriceString(1234567.12345), "1,234,567");
    EXPECT_EQ(dm.formatPriceString(123456.12345), "123,456");
    EXPECT_EQ(dm.formatPriceString(12345.12345), "12,345");
    EXPECT_EQ(dm.formatPriceString(1234.12345), "1,234");
    EXPECT_EQ(dm.formatPriceString(123.12345), "123.12");
    EXPECT_EQ(dm.formatPriceString(12.12345), "12.12");
    EXPECT_EQ(dm.formatPriceString(1.12345), "1.123");
    EXPECT_EQ(dm.formatPriceString(0.12345), "0.1235");
    EXPECT_EQ(dm.formatPriceString(0.1), "0.1000");
}

TEST_F(DisplayManagerTest, formatPriceChange)
{
    DisplayManager dm;
    EXPECT_EQ(dm.formatPriceChangeString(0.1234,  "1d"), "1d: +0.12%");
    EXPECT_EQ(dm.formatPriceChangeString(12.345,  "1M"), "1M: +12.3%");
    EXPECT_EQ(dm.formatPriceChangeString(123.456, "1Y"), "1Y: + 123%");
    EXPECT_EQ(dm.formatPriceChangeString(0, "1Y"), "1Y: +0.00%");
    EXPECT_EQ(dm.formatPriceChangeString(-0.1234,  "1d"), "1d: -0.12%");
    EXPECT_EQ(dm.formatPriceChangeString(-12.345,  "1M"), "1M: -12.3%");
    EXPECT_EQ(dm.formatPriceChangeString(-123.456, "1Y"), "1Y: - 123%");
}

