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

using namespace std;
using namespace nlohmann;

#define TEST_PORT 2404

// PLUGIN DEFAULT PROTOCOL STACK CONF
#define PROTOCOL_STACK_DEF                                                     \
    QUOTE({                                                                    \
        "protocol_stack" : {                                                   \
            "name" : "iec104client",                                           \
            "version" : "1.0",                                                 \
            "transport_layer" : {                                              \
                "connection" : {                                               \
                    "path" : [                                                 \
                        {                                                      \
                            "srv_ip" : "127.0.0.1",                            \
                            "clt_ip" : "",                                     \
                            "port" : 2404                                      \
                        },                                                     \
                        {"srv_ip" : "127.0.0.1", "clt_ip" : "", "port" : 2404} \
                    ],                                                         \
                    "tls" : false                                              \
                },                                                             \
                "k_value" : 12,                                                \
                "w_value" : 8,                                                 \
                "t0_timeout" : 10,                                             \
                "t1_timeout" : 15,                                             \
                "t2_timeout" : 10,                                             \
                "t3_timeout" : 20,                                             \
                "conn_all" : true,                                             \
                "start_all" : false,                                           \
                "conn_passv" : false                                           \
            },                                                                 \
            "application_layer" : {                                            \
                "orig_addr" : 10,                                               \
                "ca_asdu_size" : 2,                                            \
                "ioaddr_size" : 3,                                             \
                "startup_time" : 180,                                          \
                "asdu_size" : 0,                                               \
                "gi_time" : 60,                                                \
                "gi_cycle" : false,                                            \
                "gi_all_ca" : true,                                           \
                "gi_repeat_count" : 2,                                         \
                "disc_qual" : "NT",                                            \
                "send_iv_time" : 0,                                            \
                "tsiv" : "REMOVE",                                             \
                "utc_time" : false,                                            \
                "comm_wttag" : false,                                          \
                "comm_parallel" : 0,                                           \
                "exec_cycl_test" : false,                                      \
                "startup_state" : true,                                        \
                "reverse" : false,                                             \
                "time_sync_period" : 100,                                      \
                "time_sync" : true                                    \
            }                                                                  \
        }                                                                      \
    })

#define PROTOCOL_STACK_DEF2                                                    \
    QUOTE({                                                                    \
        "protocol_stack" : {                                                   \
            "name" : "iec104client",                                           \
            "version" : "1.0",                                                 \
            "transport_layer" : {                                              \
                "connection" : {                                               \
                    "path" : [                                                 \
                        {                                                      \
                            "srv_ip" : "127.0.0.1",                            \
                            "clt_ip" : "",                                     \
                            "port" : 2404                                      \
                        },                                                     \
                        {"srv_ip" : "127.0.0.1", "clt_ip" : "", "port" : 2404} \
                    ],                                                         \
                    "tls" : false                                              \
                },                                                             \
                "k_value" : 12,                                                \
                "w_value" : 8,                                                 \
                "t0_timeout" : 10,                                             \
                "t1_timeout" : 15,                                             \
                "t2_timeout" : 10,                                             \
                "t3_timeout" : 20,                                             \
                "conn_all" : true,                                             \
                "start_all" : false,                                           \
                "conn_passv" : false                                           \
            },                                                                 \
            "application_layer" : {                                            \
                "orig_addr" : 10,                                               \
                "ca_asdu_size" : 2,                                            \
                "ioaddr_size" : 3,                                             \
                "startup_time" : 180,                                          \
                "asdu_size" : 0,                                               \
                "gi_time" : 60,                                                \
                "gi_cycle" : false,                                            \
                "gi_all_ca" : false,                                           \
                "gi_repeat_count" : 2,                                         \
                "disc_qual" : "NT",                                            \
                "send_iv_time" : 0,                                            \
                "tsiv" : "REMOVE",                                             \
                "utc_time" : false,                                            \
                "comm_wttag" : false,                                          \
                "comm_parallel" : 0,                                           \
                "exec_cycl_test" : false,                                      \
                "startup_state" : true,                                        \
                "reverse" : false,                                             \
                "time_sync_period" : 100,                                      \
                "time_sync" : true                                    \
            }                                                                  \
        }                                                                      \
    })

// PLUGIN DEFAULT EXCHANGED DATA CONF
#define EXCHANGED_DATA_DEF                   \
    QUOTE({                                  \
        "exchanged_data" : {                 \
            "name" : "iec104client",         \
            "version" : "1.0",               \
            "asdu_list" : [                  \
                {                            \
                    "ca" : 41025,            \
                    "type_id" : "M_ME_NA_1", \
                    "label" : "TM-1",        \
                    "ioa" : 4202832          \
                },                           \
                {                            \
                    "ca" : 41025,            \
                    "type_id" : "M_ME_NA_1", \
                    "label" : "TM-2",        \
                    "ioa" : 4202852          \
                },                           \
                {                            \
                    "ca" : 41025,            \
                    "type_id" : "M_SP_TB_1", \
                    "label" : "TS-1",        \
                    "ioa" : 4206948          \
                },                           \
                {                            \
                    "ca" : 41025,            \
                    "type_id" : "C_SC_NA_1", \
                    "label" : "C-1",         \
                    "ioa" : 2000             \
                },                           \
                {                            \
                    "ca" : 41025,            \
                    "type_id" : "C_SC_TA_1", \
                    "label" : "C-2",         \
                    "ioa" : 2001             \
                },                           \
                {                            \
                    "ca" : 41025,            \
                    "type_id" : "C_DC_NA_1", \
                    "label" : "C-3",         \
                    "ioa" : 2002             \
                },                           \
                {                            \
                    "ca" : 41026,            \
                    "type_id" : "M_ME_NA_1", \
                    "label" : "TM-B-1",      \
                    "ioa" : 2001             \
                }                            \
            ]                                \
        }                                    \
    })

// PLUGIN DEFAULT TLS CONF
#define TLS_DEF                               \
    QUOTE({                                   \
        "tls_conf:" : {                       \
            "private_key" : "server-key.pem", \
            "server_cert" : "server.cer",     \
            "ca_cert" : "root.cer"            \
        }                                     \
    })


// Define configuration, important part is the exchanged_data
// It contains all asdu to take into account
struct json_config
{
    string protocol_stack = PROTOCOL_STACK_DEF;

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

class InterrogationTest : public testing::Test
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

        // for (Datapoint* sdp : dataPoints) {
        //     printf("name: %s value: %s\n", sdp->getName().c_str(), sdp->getData().toString().c_str());
        // }
        storedReading = new Reading(reading);

        ingestCallbackCalled++;
    }

    static bool clockSynchronizationHandler(void* parameter, IMasterConnection connection, CS101_ASDU asdu, CP56Time2a newTime)
    {
        clockSyncHandlerCalled++;

        return true;
    }

    static bool asduHandler (void* parameter, IMasterConnection connection, CS101_ASDU asdu)
    {
        InterrogationTest* self = (InterrogationTest*)parameter;

        printf("asduHandler: type: %i\n", CS101_ASDU_getTypeID(asdu));

        lastConnection = NULL;
        lastOA = CS101_ASDU_getOA(asdu);

        int ca = CS101_ASDU_getCA(asdu);

        InformationObject io = CS101_ASDU_getElement(asdu, 0);

        int ioa = InformationObject_getObjectAddress(io);

        if (CS101_ASDU_getTypeID(asdu) == C_SC_NA_1) {
            printf("  C_SC_NA_1 (single-command)\n");

            if (ca == 41025 && ioa == 2000) {
                IMasterConnection_sendACT_CON(connection, asdu, false);
                lastConnection = connection;
            }

            
        }
        else if (CS101_ASDU_getTypeID(asdu) == C_SC_TA_1) {
            printf("  C_SC_TA_1 (single-command w/timetag)\n");

            if (ca == 41025 && ioa == 2001) {
                IMasterConnection_sendACT_CON(connection, asdu, false);
                lastConnection = connection;
            }
        }
        else if (CS101_ASDU_getTypeID(asdu) == C_DC_NA_1) {
            printf("  C_DC_NA_1 (double-command)\n");

            if (ca == 41025 && ioa == 2002) {
                IMasterConnection_sendACT_CON(connection, asdu, false);
                lastConnection = connection;
            }
        }

        asduHandlerCalled++;

        return true;
    }

    static bool interrogationHandler(void* parameter, IMasterConnection connection, CS101_ASDU asdu, uint8_t qoi)
    {
        InterrogationTest* self = (InterrogationTest*)parameter;

        interrogationRequestsReceived++;

        printf("Received interrogation for group %i\n", qoi);

        if (qoi == 20) { /* only handle station interrogation */

            CS101_AppLayerParameters alParams = IMasterConnection_getApplicationLayerParameters(connection);

            IMasterConnection_sendACT_CON(connection, asdu, false);

            /* The CS101 specification only allows information objects without timestamp in GI responses */

            CS101_ASDU newAsdu = CS101_ASDU_create(alParams, false, CS101_COT_INTERROGATED_BY_STATION,
                    0, 1, false, false);

            InformationObject io = (InformationObject) MeasuredValueScaled_create(NULL, 100, -1, IEC60870_QUALITY_GOOD);

            CS101_ASDU_addInformationObject(newAsdu, io);

            CS101_ASDU_addInformationObject(newAsdu, (InformationObject)
                MeasuredValueScaled_create((MeasuredValueScaled) io, 101, 23, IEC60870_QUALITY_GOOD));

            CS101_ASDU_addInformationObject(newAsdu, (InformationObject)
                MeasuredValueScaled_create((MeasuredValueScaled) io, 102, 2300, IEC60870_QUALITY_GOOD));

            InformationObject_destroy(io);

            IMasterConnection_sendASDU(connection, newAsdu);

            CS101_ASDU_destroy(newAsdu);

            newAsdu = CS101_ASDU_create(alParams, false, CS101_COT_INTERROGATED_BY_STATION,
                        0, 1, false, false);

            io = (InformationObject) SinglePointInformation_create(NULL, 104, true, IEC60870_QUALITY_GOOD);

            CS101_ASDU_addInformationObject(newAsdu, io);

            CS101_ASDU_addInformationObject(newAsdu, (InformationObject)
                SinglePointInformation_create((SinglePointInformation) io, 105, false, IEC60870_QUALITY_GOOD));

            InformationObject_destroy(io);

            IMasterConnection_sendASDU(connection, newAsdu);

            CS101_ASDU_destroy(newAsdu);

            newAsdu = CS101_ASDU_create(alParams, true, CS101_COT_INTERROGATED_BY_STATION,
                    0, 1, false, false);

            CS101_ASDU_addInformationObject(newAsdu, io = (InformationObject) SinglePointInformation_create(NULL, 300, true, IEC60870_QUALITY_GOOD));
            CS101_ASDU_addInformationObject(newAsdu, (InformationObject) SinglePointInformation_create((SinglePointInformation) io, 301, false, IEC60870_QUALITY_GOOD));
            CS101_ASDU_addInformationObject(newAsdu, (InformationObject) SinglePointInformation_create((SinglePointInformation) io, 302, true, IEC60870_QUALITY_GOOD));
            CS101_ASDU_addInformationObject(newAsdu, (InformationObject) SinglePointInformation_create((SinglePointInformation) io, 303, false, IEC60870_QUALITY_GOOD));
            CS101_ASDU_addInformationObject(newAsdu, (InformationObject) SinglePointInformation_create((SinglePointInformation) io, 304, true, IEC60870_QUALITY_GOOD));
            CS101_ASDU_addInformationObject(newAsdu, (InformationObject) SinglePointInformation_create((SinglePointInformation) io, 305, false, IEC60870_QUALITY_GOOD));
            CS101_ASDU_addInformationObject(newAsdu, (InformationObject) SinglePointInformation_create((SinglePointInformation) io, 306, true, IEC60870_QUALITY_GOOD));
            CS101_ASDU_addInformationObject(newAsdu, (InformationObject) SinglePointInformation_create((SinglePointInformation) io, 307, false, IEC60870_QUALITY_GOOD));

            InformationObject_destroy(io);

            IMasterConnection_sendASDU(connection, newAsdu);

            CS101_ASDU_destroy(newAsdu);

            newAsdu = CS101_ASDU_create(alParams, false, CS101_COT_INTERROGATED_BY_STATION,
                            0, 1, false, false);

            io = (InformationObject) BitString32_create(NULL, 500, 0xaaaa);

            CS101_ASDU_addInformationObject(newAsdu, io);

            InformationObject_destroy(io);

            IMasterConnection_sendASDU(connection, newAsdu);

            CS101_ASDU_destroy(newAsdu);

            IMasterConnection_sendACT_TERM(connection, asdu);
        }
        else {
            IMasterConnection_sendACT_CON(connection, asdu, true);
        }

        return true;
    }

    static bool interrogationHandler_No_ACT_CON(void* parameter, IMasterConnection connection, CS101_ASDU asdu, uint8_t qoi)
    {
        InterrogationTest* self = (InterrogationTest*)parameter;

        interrogationRequestsReceived++;

        printf("Received interrogation for group %i\n", qoi);

        return true;
    }


    static boost::thread thread_;
    static IEC104TestComp* iec104;
    static json_config config;
    static int ingestCallbackCalled;
    static Reading* storedReading;
    static int clockSyncHandlerCalled;
    static int asduHandlerCalled;
    static IMasterConnection lastConnection;
    static int lastOA;
    static int interrogationRequestsReceived;
};

boost::thread InterrogationTest::thread_;
IEC104TestComp* InterrogationTest::iec104;
json_config InterrogationTest::config;
int InterrogationTest::ingestCallbackCalled;
Reading* InterrogationTest::storedReading;
int InterrogationTest::asduHandlerCalled;
int InterrogationTest::interrogationRequestsReceived;
int InterrogationTest::clockSyncHandlerCalled;
IMasterConnection InterrogationTest::lastConnection;
int InterrogationTest::lastOA;


TEST_F(InterrogationTest, IEC104Client_startupProcedureSeparateRequestForEachCA)
{
    iec104->setJsonConfig(PROTOCOL_STACK_DEF, EXCHANGED_DATA_DEF, TLS_DEF);

    asduHandlerCalled = 0;
    interrogationRequestsReceived = 0;
    clockSyncHandlerCalled = 0;
    lastConnection = NULL;
    ingestCallbackCalled = 0;

    CS104_Slave slave = CS104_Slave_create(10, 10);

    CS104_Slave_setLocalPort(slave, TEST_PORT);

    CS104_Slave_setClockSyncHandler(slave, clockSynchronizationHandler, this);
    CS104_Slave_setASDUHandler(slave, asduHandler, this);
    CS104_Slave_setInterrogationHandler(slave, interrogationHandler, this);

    CS104_Slave_start(slave);

    CS101_AppLayerParameters alParams = CS104_Slave_getAppLayerParameters(slave);

    startIEC104();

    Thread_sleep(500);

    ASSERT_EQ(1, clockSyncHandlerCalled);
    ASSERT_EQ(0, asduHandlerCalled);

    Thread_sleep(2500);

    ASSERT_EQ(2, interrogationRequestsReceived);

    CS104_Slave_stop(slave);

    CS104_Slave_destroy(slave);
}

TEST_F(InterrogationTest, IEC104Client_startupProcedureBroadcastCA)
{
    iec104->setJsonConfig(PROTOCOL_STACK_DEF2, EXCHANGED_DATA_DEF, TLS_DEF);

    asduHandlerCalled = 0;
    interrogationRequestsReceived = 0;
    clockSyncHandlerCalled = 0;
    lastConnection = NULL;
    ingestCallbackCalled = 0;

    CS104_Slave slave = CS104_Slave_create(10, 10);

    CS104_Slave_setLocalPort(slave, TEST_PORT);

    CS104_Slave_setClockSyncHandler(slave, clockSynchronizationHandler, this);
    CS104_Slave_setASDUHandler(slave, asduHandler, this);
    CS104_Slave_setInterrogationHandler(slave, interrogationHandler, this);

    CS104_Slave_start(slave);

    CS101_AppLayerParameters alParams = CS104_Slave_getAppLayerParameters(slave);

    startIEC104();

    Thread_sleep(500);

    ASSERT_EQ(1, clockSyncHandlerCalled);
    ASSERT_EQ(0, asduHandlerCalled);

    Thread_sleep(2500);

    ASSERT_EQ(1, interrogationRequestsReceived);

    CS104_Slave_stop(slave);

    CS104_Slave_destroy(slave);
}


