#include <config_category.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <iec104.h>
#include <plugin_api.h>
#include <string.h>

#include <boost/thread.hpp>
#include <utility>
#include <vector>

#include "conf_init.h"

using namespace std;
using namespace nlohmann;

// Define configuration, important part is the exchanged_data
// It contains all asdu to take into account
struct json_config
{
    string protocol_stack = PROTOCOL_STACK_DEF_INFO;

    string protocol_translation = PROTOCOL_TRANSLATION_DEF;

    string exchanged_data = EXCHANGED_DATA_DEF;

    string tls = TLS_DEF;
};

class IEC104TestComp : public IEC104
{
public:
    IEC104TestComp() : IEC104()
    {
        CS104_Connection new_connection =
            CS104_Connection_create("127.0.0.1", 2404);
        if (new_connection != nullptr)
        {
            cout << "Connexion initialisée" << endl;
        }
        m_connections.push_back(new_connection);
    }
};

class IEC104Test : public testing::Test
{
protected:
    static void SetConfig()
    {
        cout << "[IEC104BaseTest] setJsonConfig." << endl;
        IEC104::setJsonConfig(config.protocol_stack, config.exchanged_data,

                              config.protocol_translation, config.tls);
    }

    // Per-test-suite set-up.
    // Called before the first test in this test suite.
    // Can be omitted if not needed.
    static void SetUpTestSuite()
    {
        // Avoid reallocating static objects if called in subclasses of FooTest.
        if (iec104 == nullptr)
        {
            iec104 = new IEC104TestComp();
            iec104->setJsonConfig(PROTOCOL_STACK_DEF_INFO, EXCHANGED_DATA_DEF,
                                  PROTOCOL_TRANSLATION_DEF, TLS_DEF);
            thread_ = boost::thread(&IEC104Test::startIEC104);
        }
    }

    // Per-test-suite tear-down.
    // Called after the last test in this test suite.
    // Can be omitted if not needed.
    static void TearDownTestSuite()
    {
        iec104->stop();
        thread_.interrupt();
        // delete iec104;
        // iec104 = nullptr;
        // thread_.join();
    }

    static void startIEC104() { iec104->start(); }

    static boost::thread thread_;
    static IEC104TestComp* iec104;
    static json_config config;
};

boost::thread IEC104Test::thread_;
IEC104TestComp* IEC104Test::iec104;
json_config IEC104Test::config;

TEST_F(IEC104Test, IEC104_operation)
{
    PLUGIN_PARAMETER* params[3];
    vector<string> operations;
    PLUGIN_PARAMETER iec104client = {"iec104client", "4"};
    params[0] = &iec104client;

    // ioa
    PLUGIN_PARAMETER ioa = {"io", "4202832"};
    params[1] = &ioa;

    // Third value
    PLUGIN_PARAMETER buf = {"", "4202832"};
    params[2] = &buf;

    operations.push_back("CS104_Connection_sendInterrogationCommand");
    operations.push_back("CS104_Connection_sendTestCommandWithTimestamp");
    operations.push_back("SingleCommandWithCP56Time2a");
    operations.push_back("DoubleCommandWithCP56Time2a");
    operations.push_back("StepCommandWithCP56Time2a");

    for (const string& operation : operations)
    {
        ASSERT_TRUE(iec104->operation(operation, 3, params));
    }

    ASSERT_FALSE(IEC104Test::iec104->operation("NULL", 0, params));
}

TEST_F(IEC104Test, IEC104_setJsonConfig_Test)
{
    ASSERT_NO_THROW(
        IEC104::setJsonConfig(config.protocol_stack, config.exchanged_data,
                              config.protocol_translation, config.tls));
}

/*Public Member Functions*/
TEST_F(IEC104Test, IEC104_setAssetName_Test)
{
    ASSERT_NO_THROW(IEC104Test::iec104->setAssetName("Name"));
}

/* Static Public Member Functions */
TEST_F(IEC104Test, IEC104_set_get_CommWttag_Test)
{
    IEC104::setCommWttag(false);
    ASSERT_FALSE(IEC104::getCommWttag());

    IEC104::setCommWttag(true);
    ASSERT_TRUE(IEC104::getCommWttag());
}

TEST_F(IEC104Test, IEC104_set_get_Tsiv_Test)
{
    IEC104::setTsiv("Test");
    ASSERT_EQ(IEC104::getTsiv(), "Test");
}
