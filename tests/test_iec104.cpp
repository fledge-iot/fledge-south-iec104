#include <config_category.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <iec104.h>
#include <plugin_api.h>
#include <string.h>
#include "cs104_slave.h"

#include <boost/thread.hpp>
#include <utility>
#include <vector>

#include "conf_init.h"

using namespace std;
using namespace nlohmann;

#define TEST_PORT 2404

// Define configuration, important part is the exchanged_data
// It contains all asdu to take into account
struct json_config
{
    string protocol_stack = PROTOCOL_STACK_DEF_INFO;

    string exchanged_data = EXCHANGED_DATA_DEF;

    string tls = TLS_DEF;
};

class IEC104TestComp : public IEC104
{
public:
    IEC104TestComp() : IEC104()
    {
        // CS104_Connection new_connection =
        //     CS104_Connection_create("127.0.0.1", TEST_PORT);
        // if (new_connection != nullptr)
        // {
        //     cout << "Connexion initialisée" << endl;
        // }
        // m_connections.push_back(new_connection);
    }
};

class IEC104Test : public testing::Test
{
protected:

    struct sTestInfo {
        int callbackCalled;
        Reading* storedReading;
    };

    // Per-test-suite set-up.
    // Called before the first test in this test suite.
    // Can be omitted if not needed.
    static void SetUpTestSuite()
    {
        // Avoid reallocating static objects if called in subclasses of FooTest.
        if (iec104 == nullptr)
        {
            iec104 = new IEC104TestComp();
            iec104->setJsonConfig(PROTOCOL_STACK_DEF_INFO, EXCHANGED_DATA_DEF, TLS_DEF);

            iec104->registerIngest(NULL, ingestCallback);

            //startIEC104();
            //thread_ = boost::thread(&IEC104Test::startIEC104);
        }
    }

    // Per-test-suite tear-down.
    // Called after the last test in this test suite.
    // Can be omitted if not needed.
    static void TearDownTestSuite()
    {
        iec104->stop();
        //thread_.interrupt();
        // delete iec104;
        // iec104 = nullptr;
        //thread_.join();
    }

    static void startIEC104() { iec104->start(); }

    static bool hasChild(Datapoint& dp, std::string childLabel)
    {
        DatapointValue& dpv = dp.getData();

        auto dps = dpv.getDpVec();

        for (auto sdp : *dps) {
            if (sdp->getName() == childLabel) {
                return true;
            }
        }

        return false;
    }

    static Datapoint* getChild(Datapoint& dp, std::string childLabel)
    {
        DatapointValue& dpv = dp.getData();

        auto dps = dpv.getDpVec();

        for (Datapoint* childDp : *dps) {
            if (childDp->getName() == childLabel) {
                return childDp;
            }
        }

        return nullptr;
    }

    static int64_t getIntValue(Datapoint* dp)
    {
        DatapointValue dpValue = dp->getData();
        return dpValue.toInt();
    }

    static std::string getStrValue(Datapoint* dp)
    {
        return dp->getData().toStringValue();
    }

    static bool hasObject(Reading& reading, std::string label)
    {
        std::vector<Datapoint*> dataPoints = reading.getReadingData();

        for (Datapoint* dp : dataPoints) 
        {
            if (dp->getName() == label) {
                return true;
            }
        }

        return false;
    }

    static Datapoint* getObject(Reading& reading, std::string label)
    {
        std::vector<Datapoint*> dataPoints = reading.getReadingData();

        for (Datapoint* dp : dataPoints) 
        {
            if (dp->getName() == label) {
                return dp;
            }
        }

        return nullptr;
    }


    static void ingestCallback(void* parameter, Reading reading)
    {
        printf("ingestCallback called -> asset: (%s)\n", reading.getAssetName().c_str());

        std::vector<Datapoint*> dataPoints = reading.getReadingData();

        printf("  number of readings: %lu\n", dataPoints.size());

        // for (Datapoint* sdp : dataPoints) {
        //     printf("name: %s value: %s\n", sdp->getName().c_str(), sdp->getData().toString().c_str());
        // }
        storedReading = new Reading(reading);

        ingestCallbackCalled++;
    }

    static boost::thread thread_;
    static IEC104TestComp* iec104;
    static json_config config;
    static int ingestCallbackCalled;
    static Reading* storedReading;
};

boost::thread IEC104Test::thread_;
IEC104TestComp* IEC104Test::iec104;
json_config IEC104Test::config;
int IEC104Test::ingestCallbackCalled;
Reading* IEC104Test::storedReading;

#if 0
TEST_F(IEC104Test, IEC104_operation_notConnected)
{
    startIEC104();

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
        ASSERT_FALSE(iec104->operation(operation, 3, params));
    }

    ASSERT_FALSE(IEC104Test::iec104->operation("NULL", 0, params));
}
#endif

TEST_F(IEC104Test, IEC104_receiveMonitoringAsdus)
{
    CS104_Slave slave = CS104_Slave_create(10, 10);

    CS104_Slave_setLocalPort(slave, TEST_PORT);

    CS104_Slave_start(slave);

    CS101_AppLayerParameters alParams = CS104_Slave_getAppLayerParameters(slave);

    startIEC104();

    CS101_ASDU newAsdu = CS101_ASDU_create(alParams, false, CS101_COT_PERIODIC, 0, 41025, false, false);

    struct sCP56Time2a ts;

    uint64_t timestamp = Hal_getTimeInMs();
            
    CP56Time2a_createFromMsTimestamp(&ts, timestamp);

    InformationObject io = (InformationObject) SinglePointWithCP56Time2a_create(NULL, 4206948, true, IEC60870_QUALITY_GOOD, &ts);

    CS101_ASDU_addInformationObject(newAsdu, io);

    InformationObject_destroy(io);

    /* Add ASDU to slave event queue */
    CS104_Slave_enqueueASDU(slave, newAsdu);

    CS101_ASDU_destroy(newAsdu);

    Thread_sleep(500);

    ASSERT_EQ(ingestCallbackCalled, 1);
    ASSERT_EQ("TS-1", storedReading->getAssetName());
    ASSERT_TRUE(hasObject(*storedReading, "data_object"));
    Datapoint* data_object = getObject(*storedReading, "data_object");
    ASSERT_NE(nullptr, data_object);
    ASSERT_TRUE(hasChild(*data_object, "do_type"));
    ASSERT_TRUE(hasChild(*data_object, "do_ca"));
    ASSERT_TRUE(hasChild(*data_object, "do_oa"));
    ASSERT_TRUE(hasChild(*data_object, "do_cot"));
    ASSERT_TRUE(hasChild(*data_object, "do_test"));
    ASSERT_TRUE(hasChild(*data_object, "do_negative"));
    ASSERT_TRUE(hasChild(*data_object, "do_ioa"));
    ASSERT_TRUE(hasChild(*data_object, "do_value"));
    ASSERT_TRUE(hasChild(*data_object, "do_quality_iv"));
    ASSERT_TRUE(hasChild(*data_object, "do_quality_bl"));
    ASSERT_TRUE(hasChild(*data_object, "do_quality_sb"));
    ASSERT_TRUE(hasChild(*data_object, "do_quality_nt"));
    ASSERT_TRUE(hasChild(*data_object, "do_ts"));
    ASSERT_TRUE(hasChild(*data_object, "do_ts_iv"));
    ASSERT_TRUE(hasChild(*data_object, "do_ts_su"));
    ASSERT_TRUE(hasChild(*data_object, "do_ts_sub"));

    ASSERT_EQ("M_SP_TB_1", getStrValue(getChild(*data_object, "do_type")));
    ASSERT_EQ((int64_t) 41025, getIntValue(getChild(*data_object, "do_ca")));
    ASSERT_EQ((int64_t) 1, getIntValue(getChild(*data_object, "do_cot")));
    ASSERT_EQ((int64_t) 4206948, getIntValue(getChild(*data_object, "do_ioa")));
    ASSERT_EQ((int64_t) timestamp, getIntValue(getChild(*data_object, "do_ts")));

    delete storedReading;

    CS101_ASDU newAsdu2 = CS101_ASDU_create(alParams, false, CS101_COT_INTERROGATED_BY_STATION, 0, 41025, false, false);

    io = (InformationObject) SinglePointInformation_create(NULL, 4206948, true, IEC60870_QUALITY_GOOD);

    CS101_ASDU_addInformationObject(newAsdu2, io);

    InformationObject_destroy(io);

    /* Add ASDU to slave event queue */
    CS104_Slave_enqueueASDU(slave, newAsdu2);

    CS101_ASDU_destroy(newAsdu2);

    Thread_sleep(500);

    ASSERT_EQ(ingestCallbackCalled, 2);
    ASSERT_EQ("TS-1", storedReading->getAssetName());
    ASSERT_TRUE(hasObject(*storedReading, "data_object"));
    data_object = getObject(*storedReading, "data_object");
    ASSERT_NE(nullptr, data_object);
    ASSERT_TRUE(hasChild(*data_object, "do_type"));
    ASSERT_TRUE(hasChild(*data_object, "do_ca"));
    ASSERT_TRUE(hasChild(*data_object, "do_oa"));
    ASSERT_TRUE(hasChild(*data_object, "do_cot"));
    ASSERT_TRUE(hasChild(*data_object, "do_test"));
    ASSERT_TRUE(hasChild(*data_object, "do_negative"));
    ASSERT_TRUE(hasChild(*data_object, "do_ioa"));
    ASSERT_TRUE(hasChild(*data_object, "do_value"));
    ASSERT_TRUE(hasChild(*data_object, "do_quality_iv"));
    ASSERT_TRUE(hasChild(*data_object, "do_quality_bl"));
    ASSERT_TRUE(hasChild(*data_object, "do_quality_sb"));
    ASSERT_TRUE(hasChild(*data_object, "do_quality_nt"));
    ASSERT_FALSE(hasChild(*data_object, "do_ts"));
    ASSERT_FALSE(hasChild(*data_object, "do_ts_iv"));
    ASSERT_FALSE(hasChild(*data_object, "do_ts_su"));
    ASSERT_FALSE(hasChild(*data_object, "do_ts_sub"));

    ASSERT_EQ("M_SP_NA_1", getStrValue(getChild(*data_object, "do_type")));
    ASSERT_EQ((int64_t) 41025, getIntValue(getChild(*data_object, "do_ca")));
    ASSERT_EQ((int64_t) 20, getIntValue(getChild(*data_object, "do_cot")));
    ASSERT_EQ((int64_t) 4206948, getIntValue(getChild(*data_object, "do_ioa")));
    ASSERT_EQ(0, getIntValue(getChild(*data_object, "do_quality_iv")));
    ASSERT_EQ(0, getIntValue(getChild(*data_object, "do_quality_bl")));
    ASSERT_EQ(0, getIntValue(getChild(*data_object, "do_quality_sb")));
    ASSERT_EQ(0, getIntValue(getChild(*data_object, "do_quality_nt")));

    delete storedReading;

    CS101_ASDU newAsdu3 = CS101_ASDU_create(alParams, false, CS101_COT_INTERROGATED_BY_STATION, 0, 41025, false, false);

    io = (InformationObject) SinglePointInformation_create(NULL, 4206948, true, IEC60870_QUALITY_INVALID | IEC60870_QUALITY_NON_TOPICAL);

    CS101_ASDU_addInformationObject(newAsdu3, io);

    InformationObject_destroy(io);

    /* Add ASDU to slave event queue */
    CS104_Slave_enqueueASDU(slave, newAsdu3);

    CS101_ASDU_destroy(newAsdu3);

    Thread_sleep(500);

    ASSERT_EQ(ingestCallbackCalled, 3);
    data_object = getObject(*storedReading, "data_object");
    ASSERT_EQ(1, getIntValue(getChild(*data_object, "do_quality_iv")));
    ASSERT_EQ(0, getIntValue(getChild(*data_object, "do_quality_bl")));
    ASSERT_EQ(0, getIntValue(getChild(*data_object, "do_quality_sb")));
    ASSERT_EQ(1, getIntValue(getChild(*data_object, "do_quality_nt")));

    delete storedReading;

    CS104_Slave_stop(slave);

    CS104_Slave_destroy(slave);
}

TEST_F(IEC104Test, IEC104_setJsonConfig_Test)
{
    ASSERT_NO_THROW(
        iec104->setJsonConfig(config.protocol_stack, config.exchanged_data, config.tls));
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
