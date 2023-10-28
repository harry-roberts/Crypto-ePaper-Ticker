#include "Utils.h"
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "compile_time.h"

class UtilsTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // Serial output required - see note in main
        // new line also helps with test formatting in serial monitor
        Serial.println();
    }

};

TEST_F(UtilsTest, batteryPercent)
{
    EXPECT_EQ(utils::battery_percent(3.2), 0);
    EXPECT_EQ(utils::battery_percent(4.2), 100);
    EXPECT_EQ(utils::battery_percent(3.7), 50);

    EXPECT_GT(utils::battery_percent(utils::battery_read()), 0); // it will actually read 100 becasue of plugged in voltage
}
