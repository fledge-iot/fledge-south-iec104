#include <gtest/gtest.h>

#include "iec104_utility.h"

TEST(PivotIEC104PluginUtility, Logs)
{
    std::string text("This message is at level %s");
    ASSERT_NO_THROW(Iec104Utility::log_debug(text.c_str(), "debug"));
    ASSERT_NO_THROW(Iec104Utility::log_info(text.c_str(), "info"));
    ASSERT_NO_THROW(Iec104Utility::log_warn(text.c_str(), "warning"));
    ASSERT_NO_THROW(Iec104Utility::log_error(text.c_str(), "error"));
    ASSERT_NO_THROW(Iec104Utility::log_fatal(text.c_str(), "fatal"));
}