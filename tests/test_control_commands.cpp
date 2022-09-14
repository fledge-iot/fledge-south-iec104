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

#define TEST_PORT 2404

// PLUGIN DEFAULT PROTOCOL STACK CONF
static string protocol_config = QUOTE({
        "protocol_stack" : {
            "name" : "iec104client",
            "version" : "1.0",
            "transport_layer" : {
                "redundancy_groups" : [
                    { 
                        "connections" : [
                            {     
                                "srv_ip" : "127.0.0.1",        
                                "port" : 2404          
                            },    
                            {
                                "srv_ip" : "127.0.0.1", 
                                "port" : 2404
                            }
                        ],
                        "rg_name" : "red-group1",  
                        "tls" : false,
                        "k_value" : 12,  
                        "w_value" : 8,
                        "t0_timeout" : 10,                 
                        "t1_timeout" : 15,                 
                        "t2_timeout" : 10,                 
                        "t3_timeout" : 20    
                    }
                ]                  
            },                
            "application_layer" : {                
                "orig_addr" : 10, 
                "ca_asdu_size" : 2,                
                "ioaddr_size" : 3,                 
                "startup_time" : 180,              
                "asdu_size" : 0, 
                "gi_time" : 60,  
                "gi_cycle" : false,                
                "gi_all_ca" : false,               
                "gi_repeat_count" : 2,             
                "disc_qual" : "NT",                
                "send_iv_time" : 0,                
                "tsiv" : "REMOVE",                 
                "utc_time" : false,                
                "comm_wttag" : false,              
                "comm_parallel" : 0,               
                "exec_cycl_test" : false,          
                "reverse" : false,                 
                "time_sync" : 0                 
            }                 
        }                     
    });

// PLUGIN DEFAULT EXCHANGED DATA CONF

static string exchanged_data = QUOTE({
        "exchanged_data": {
            "name" : "iec104client",        
            "version" : "1.0",               
            "datapoints" : [          
                {
                    "label":"TM-1",
                    "protocols":[
                       {
                          "name":"iec104",
                          "address":"41025-4202832",
                          "typeid":"M_ME_NA_1"
                       }
                    ]
                },
                {
                    "label":"TM-2",
                    "protocols":[
                       {
                          "name":"iec104",
                          "address":"41025-4202852",
                          "typeid":"M_ME_NA_1"
                       }
                    ]
                },
                {
                    "label":"TS-1",
                    "protocols":[
                       {
                          "name":"iec104",
                          "address":"41025-4206948",
                          "typeid":"M_SP_TB_1"
                       }
                    ]
                },
                {
                    "label":"C-1",
                    "protocols":[
                       {
                          "name":"iec104",
                          "address":"41025-2000",
                          "typeid":"C_SC_NA_1"
                       }
                    ]
                },                          
                {
                    "label":"C-2",
                    "protocols":[
                       {
                          "name":"iec104",
                          "address":"41025-2001",
                          "typeid":"C_SC_TA_1"
                       }
                    ]
                },                            
                {
                    "label":"C-3",
                    "protocols":[
                       {
                          "name":"iec104",
                          "address":"41025-2002",
                          "typeid":"C_DC_NA_1"
                       }
                    ]
                }                        
            ]
        }
    });

// PLUGIN DEFAULT TLS CONF  
static string tls_config =  QUOTE({       
        "tls_conf:" : {
            "private_key" : "server-key.pem",
            "server_cert" : "server.cer",
            "ca_cert" : "root.cer"
        }         
    });


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

class ControlCommandsTest : public testing::Test
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
            iec104->setJsonConfig(protocol_config, exchanged_data, tls_config);

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
        ControlCommandsTest* self = (ControlCommandsTest*)parameter;

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

        if (CS101_ASDU_getTypeID(asdu) != C_IC_NA_1)
            asduHandlerCalled++;

        return true;
    }

    static boost::thread thread_;
    static IEC104TestComp* iec104;
    static int ingestCallbackCalled;
    static Reading* storedReading;
    static int clockSyncHandlerCalled;
    static int asduHandlerCalled;
    static IMasterConnection lastConnection;
    static int lastOA;
};

boost::thread ControlCommandsTest::thread_;
IEC104TestComp* ControlCommandsTest::iec104;
int ControlCommandsTest::ingestCallbackCalled;
Reading* ControlCommandsTest::storedReading;
int ControlCommandsTest::asduHandlerCalled;
int ControlCommandsTest::clockSyncHandlerCalled;
IMasterConnection ControlCommandsTest::lastConnection;
int ControlCommandsTest::lastOA;


TEST_F(ControlCommandsTest, IEC104Client_sendSingleCommand)
{
    asduHandlerCalled = 0;
    clockSyncHandlerCalled = 0;
    lastConnection = NULL;
    ingestCallbackCalled = 0;

    CS104_Slave slave = CS104_Slave_create(10, 10);

    CS104_Slave_setLocalPort(slave, TEST_PORT);

    CS104_Slave_setClockSyncHandler(slave, clockSynchronizationHandler, this);
    CS104_Slave_setASDUHandler(slave, asduHandler, this);

    CS104_Slave_start(slave);

    CS101_AppLayerParameters alParams = CS104_Slave_getAppLayerParameters(slave);

    startIEC104();

    Thread_sleep(500);

    PLUGIN_PARAMETER* params[4];

    PLUGIN_PARAMETER ca = {"ca", "41025"};
    params[0] = &ca;

    // ioa
    PLUGIN_PARAMETER ioa = {"ioa", "2000"};
    params[1] = &ioa;

    // Third value
    PLUGIN_PARAMETER value = {"", "1"};
    params[2] = &value;

    // Third value
    PLUGIN_PARAMETER select = {"", "0"};
    params[3] = &select;

    bool operationResult = iec104->operation("SingleCommand", 4, params);

    ASSERT_TRUE(operationResult);

    Thread_sleep(500);

    ASSERT_EQ(1, asduHandlerCalled);

    CS101_ASDU ctAsdu = CS101_ASDU_create(IMasterConnection_getApplicationLayerParameters(lastConnection),
        false, CS101_COT_ACTIVATION_TERMINATION,lastOA, 41025, false, false);

    InformationObject io = (InformationObject)SingleCommand_create(NULL, 2000, true, false, 0);

    CS101_ASDU_addInformationObject(ctAsdu, io);

    IMasterConnection_sendASDU(lastConnection, ctAsdu);

    ASSERT_EQ(10, lastOA);

    Thread_sleep(500);

    // expect ingest callback called two timees:
    //  1. ACT_CON for single command
    //  2. ACT_TERM for single command
    ASSERT_EQ(2, ingestCallbackCalled);

    CS104_Slave_stop(slave);

    CS104_Slave_destroy(slave);
}

TEST_F(ControlCommandsTest, IEC104Client_sendSingleCommandNotInExchangeConfig)
{
    asduHandlerCalled = 0;
    clockSyncHandlerCalled = 0;
    lastConnection = NULL;
    ingestCallbackCalled = 0;

    CS104_Slave slave = CS104_Slave_create(10, 10);

    CS104_Slave_setLocalPort(slave, TEST_PORT);

    CS104_Slave_setClockSyncHandler(slave, clockSynchronizationHandler, this);
    CS104_Slave_setASDUHandler(slave, asduHandler, this);

    CS104_Slave_start(slave);

    CS101_AppLayerParameters alParams = CS104_Slave_getAppLayerParameters(slave);

    startIEC104();

    Thread_sleep(500);

    PLUGIN_PARAMETER* params[4];

    PLUGIN_PARAMETER ca = {"ca", "41025"};
    params[0] = &ca;

    // ioa
    PLUGIN_PARAMETER ioa = {"ioa", "2022"};
    params[1] = &ioa;

    // Third value
    PLUGIN_PARAMETER value = {"", "1"};
    params[2] = &value;

    // Third value
    PLUGIN_PARAMETER select = {"", "0"};
    params[3] = &select;

    bool operationResult = iec104->operation("SingleCommand", 4, params);

    ASSERT_FALSE(operationResult);

    Thread_sleep(500);

    ASSERT_EQ(0, asduHandlerCalled);

    CS104_Slave_stop(slave);

    CS104_Slave_destroy(slave);
}

TEST_F(ControlCommandsTest, IEC104Client_sendSingleCommandButConfiguredAsDoubleCommand)
{
    asduHandlerCalled = 0;
    clockSyncHandlerCalled = 0;
    lastConnection = NULL;
    ingestCallbackCalled = 0;

    CS104_Slave slave = CS104_Slave_create(10, 10);

    CS104_Slave_setLocalPort(slave, TEST_PORT);

    CS104_Slave_setClockSyncHandler(slave, clockSynchronizationHandler, this);
    CS104_Slave_setASDUHandler(slave, asduHandler, this);

    CS104_Slave_start(slave);

    CS101_AppLayerParameters alParams = CS104_Slave_getAppLayerParameters(slave);

    startIEC104();

    Thread_sleep(500);

    PLUGIN_PARAMETER* params[4];

    PLUGIN_PARAMETER ca = {"ca", "41025"};
    params[0] = &ca;

    // ioa
    PLUGIN_PARAMETER ioa = {"ioa", "2021"};
    params[1] = &ioa;

    // Third value
    PLUGIN_PARAMETER value = {"", "1"};
    params[2] = &value;

    // Third value
    PLUGIN_PARAMETER select = {"", "0"};
    params[3] = &select;

    bool operationResult = iec104->operation("SingleCommand", 4, params);

    ASSERT_FALSE(operationResult);

    Thread_sleep(500);

    ASSERT_EQ(0, asduHandlerCalled);

    CS104_Slave_stop(slave);

    CS104_Slave_destroy(slave);
}

TEST_F(ControlCommandsTest, IEC104Client_sendDoubleCommand)
{
    asduHandlerCalled = 0;
    clockSyncHandlerCalled = 0;
    lastConnection = NULL;
    ingestCallbackCalled = 0;

    CS104_Slave slave = CS104_Slave_create(10, 10);

    CS104_Slave_setLocalPort(slave, TEST_PORT);

    CS104_Slave_setClockSyncHandler(slave, clockSynchronizationHandler, this);
    CS104_Slave_setASDUHandler(slave, asduHandler, this);

    CS104_Slave_start(slave);

    CS101_AppLayerParameters alParams = CS104_Slave_getAppLayerParameters(slave);

    startIEC104();

    Thread_sleep(500);

    PLUGIN_PARAMETER* params[4];

    PLUGIN_PARAMETER ca = {"ca", "41025"};
    params[0] = &ca;

    // ioa
    PLUGIN_PARAMETER ioa = {"ioa", "2002"};
    params[1] = &ioa;

    // Third value
    PLUGIN_PARAMETER value = {"", "2"};
    params[2] = &value;

    // Third value
    PLUGIN_PARAMETER select = {"", "0"};
    params[3] = &select;

    bool operationResult = iec104->operation("DoubleCommand", 4, params);

    ASSERT_TRUE(operationResult);

    Thread_sleep(500);

    ASSERT_EQ(1, asduHandlerCalled);

    CS101_ASDU ctAsdu = CS101_ASDU_create(IMasterConnection_getApplicationLayerParameters(lastConnection),
        false, CS101_COT_ACTIVATION_TERMINATION,lastOA, 41025, false, false);

    InformationObject io = (InformationObject)DoubleCommand_create(NULL, 2002, 2, false, 0);

    CS101_ASDU_addInformationObject(ctAsdu, io);

    IMasterConnection_sendASDU(lastConnection, ctAsdu);

    ASSERT_EQ(10, lastOA);

    Thread_sleep(500);

    // expect ingest callback called two timees:
    //  1. ACT_CON for single command
    //  2. ACT_TERM for single command
    ASSERT_EQ(2, ingestCallbackCalled);


    CS104_Slave_stop(slave);

    CS104_Slave_destroy(slave);
}

TEST_F(ControlCommandsTest, IEC104Client_sendDoubleCommandNotConnected)
{
    asduHandlerCalled = 0;
    clockSyncHandlerCalled = 0;
    lastConnection = NULL;
    ingestCallbackCalled = 0;

    CS104_Slave slave = CS104_Slave_create(10, 10);

    CS104_Slave_setLocalPort(slave, TEST_PORT);

    CS104_Slave_setClockSyncHandler(slave, clockSynchronizationHandler, this);
    CS104_Slave_setASDUHandler(slave, asduHandler, this);

    CS101_AppLayerParameters alParams = CS104_Slave_getAppLayerParameters(slave);

    startIEC104();

    Thread_sleep(500);

    PLUGIN_PARAMETER* params[4];

    PLUGIN_PARAMETER ca = {"ca", "41025"};
    params[0] = &ca;

    // ioa
    PLUGIN_PARAMETER ioa = {"ioa", "2002"};
    params[1] = &ioa;

    // Third value
    PLUGIN_PARAMETER value = {"", "2"};
    params[2] = &value;

    // Third value
    PLUGIN_PARAMETER select = {"", "0"};
    params[3] = &select;

    bool operationResult = iec104->operation("DoubleCommand", 4, params);

    ASSERT_FALSE(operationResult);

    CS104_Slave_destroy(slave);
}