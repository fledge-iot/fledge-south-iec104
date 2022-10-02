#include <gtest/gtest.h>
#include <iec104.h>
#include <plugin_api.h>
#include <rapidjson/document.h>
#include <string.h>

#include <string>

using namespace std;
using namespace rapidjson;

extern "C"
{
    PLUGIN_INFORMATION *plugin_info();
};

TEST(IEC104, PluginInfo)
{
    PLUGIN_INFORMATION *info = plugin_info();
    ASSERT_STREQ(info->name, "iec104");
    ASSERT_STREQ(info->type, PLUGIN_TYPE_SOUTH);
}

TEST(IEC104, PluginInfoConfigParse)
{
    PLUGIN_INFORMATION *info = plugin_info();
    Document doc;
    doc.Parse(info->config);
    ASSERT_EQ(doc.HasParseError(), false);
    ASSERT_EQ(doc.IsObject(), true);
    ASSERT_EQ(doc.HasMember("plugin"), true);
}
