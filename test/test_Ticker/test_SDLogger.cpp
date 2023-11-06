#include "SDLogger.h"
#include <gtest/gtest.h>
#include <gmock/gmock.h>

class SDLoggerTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // Serial output required - see note in main
        // new line also helps with test formatting in serial monitor
        Serial.println();
    }

};

TEST_F(SDLoggerTest, init)
{
    SDLogger sd;

    String msg = __DATE__;
    msg += " ";
    msg += __TIME__;
    msg += "\n";

    EXPECT_TRUE(sd.appendFile("/testfile.txt", msg));
}
