#include <gtest/gtest.h>

#include <config_category.h>
#include <plugin_api.h>

#include <utility>
#include <vector>
#include <string>

#include "cs104_slave.h"
#include <lib60870/hal_time.h>
#include <lib60870/hal_thread.h>

#include "iec104.h"

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
                "gi_cycle" : 30,
                "gi_all_ca" : false,
                "utc_time" : false,
                "cmd_wttag" : false,
                "cmd_parallel" : 1,
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
                },
                {
                    "label":"C-4",
                    "protocols":[
                       {
                          "name":"iec104",
                          "address":"41025-2003",
                          "typeid":"C_SE_TC_1"
                       }
                    ]
                },
                {
                    "label":"C-5",
                    "protocols":[
                       {
                          "name":"iec104",
                          "address":"41025-2004",
                          "typeid":"C_DC_TA_1"
                       }
                    ]
                },
                {
                    "label":"C-6",
                    "protocols":[
                       {
                          "name":"iec104",
                          "address":"41025-2005",
                          "typeid":"C_RC_NA_1"
                       }
                    ]
                },
                {
                    "label":"C-13",
                    "protocols":[
                       {
                          "name":"iec104",
                          "address":"41025-2012",
                          "typeid":"C_SE_NC_1"
                       }
                    ]
                },
                {
                    "label":"C-7",
                    "protocols":[
                       {
                          "name":"iec104",
                          "address":"41025-2006",
                          "typeid":"C_SE_NA_1"
                       }
                    ]
                },
                {
                    "label":"C-8",
                    "protocols":[
                       {
                          "name":"iec104",
                          "address":"41025-2007",
                          "typeid":"C_SE_TA_1"
                       }
                    ]
                },
                {
                    "label":"C-9",
                    "protocols":[
                       {
                          "name":"iec104",
                          "address":"41025-2008",
                          "typeid":"C_SE_NB_1"
                       }
                    ]
                },
                {
                    "label":"C-10",
                    "protocols":[
                       {
                          "name":"iec104",
                          "address":"41025-2009",
                          "typeid":"C_SE_TB_1"
                       }
                    ]
                },
                {
                    "label":"C-11",
                    "protocols":[
                       {
                          "name":"iec104",
                          "address":"41025-2010",
                          "typeid":"C_SC_TA_1"
                       }
                    ]
                },
                {
                    "label":"C-12",
                    "protocols":[
                       {
                          "name":"iec104",
                          "address":"41025-2011",
                          "typeid":"C_RC_TA_1"
                       }
                    ]
                }
            ]
        }
    });


static string exchanged_data1 = QUOTE({
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
                }
            ]
        }
    });

// PLUGIN DEFAULT TLS CONF
static string tls_config =  QUOTE({
        "tls_conf" : {
            "private_key" : "iec104_client.key",
            "own_cert" : "iec104_client.cer",
            "ca_certs" : [
                {
                    "cert_file": "iec104_ca.cer"
                }
            ],
            "remote_certs" : [
                {
                    "cert_file": "iec104_server.cer"
                }
            ]
        }
    });


class IEC104TestComp : public IEC104
{
public:
    IEC104TestComp() : IEC104()
    {
    }
};

class ControlCommandsTest : public testing::Test
{
protected:

    void SetUp()
    {
        iec104 = new IEC104TestComp();
        iec104->setJsonConfig(protocol_config, exchanged_data, tls_config);

        iec104->registerIngest(NULL, ingestCallback);
    }

    void TearDown()
    {
        iec104->stop();

        delete iec104;

        for (Reading* reading : storedReadings)
        {
            delete reading;
        }

        storedReadings.clear();
    }

    void startIEC104() { iec104->start(); }

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
        storedReadings.push_back(new Reading(reading));

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

        printf("asduHandler: type: %i COT: %s\n", CS101_ASDU_getTypeID(asdu), CS101_CauseOfTransmission_toString(CS101_ASDU_getCOT(asdu)));

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

            if (ca == 41025 && ioa == 2010) {
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
        else if (CS101_ASDU_getTypeID(asdu) == C_SE_TC_1) {
            printf("  C_SE_TC_1 (setpoint command short)\n");

            if (ca == 41025 && ioa == 2003) {
                IMasterConnection_sendACT_CON(connection, asdu, false);
                lastConnection = connection;
            }
        }
         else if (CS101_ASDU_getTypeID(asdu) == C_SE_NC_1) {
            printf("  C_SE_NC_1 (setpoint command short)\n");

            if (ca == 41025 && ioa == 2012) {
                IMasterConnection_sendACT_CON(connection, asdu, false);
                lastConnection = connection;
            }
        }
        else if (CS101_ASDU_getTypeID(asdu) == C_SE_NA_1) {
            printf("  C_SE_NA_1 (setpoint command normalized)\n");

            if (ca == 41025 && ioa == 2006) {
                IMasterConnection_sendACT_CON(connection, asdu, false);
                lastConnection = connection;
            }
        }
        else if (CS101_ASDU_getTypeID(asdu) == C_SE_TA_1) {
            printf("  C_SE_TA_1 (setpoint command normalized)\n");

            if (ca == 41025 && ioa == 2007) {
                IMasterConnection_sendACT_CON(connection, asdu, false);
                lastConnection = connection;
            }
        }
        else if (CS101_ASDU_getTypeID(asdu) == C_SE_NB_1) {
            printf("  C_SE_NB_1 (setpoint command scaled)\n");

            if (ca == 41025 && ioa == 2008) {
                IMasterConnection_sendACT_CON(connection, asdu, false);
                lastConnection = connection;
            }
        }
        else if (CS101_ASDU_getTypeID(asdu) == C_SE_TB_1) {
            printf("  C_SE_TB_1 (setpoint command scaled)\n");

            if (ca == 41025 && ioa == 2009) {
                IMasterConnection_sendACT_CON(connection, asdu, false);
                lastConnection = connection;
            }
        }
        else if (CS101_ASDU_getTypeID(asdu) == C_RC_TA_1) {
            printf("  C_RC_TA_1 (step command)\n");

            if (ca == 41025 && ioa == 2011) {
                IMasterConnection_sendACT_CON(connection, asdu, false);
                lastConnection = connection;
            }
        }
        else if (CS101_ASDU_getTypeID(asdu) == C_DC_TA_1) {
            printf("  C_DC_TA_1 (double-command)\n");

            if (ca == 41025 && ioa == 2004) {
                IMasterConnection_sendACT_CON(connection, asdu, false);
                lastConnection = connection;
            }
        }
        else if (CS101_ASDU_getTypeID(asdu) == C_RC_NA_1) {
            printf("  C_RC_NA_1 (step-command wo time)\n");

            if (ca == 41025 && ioa == 2005) {
                IMasterConnection_sendACT_CON(connection, asdu, false);
                lastConnection = connection;
            }
        }

        InformationObject_destroy(io);

        if (CS101_ASDU_getTypeID(asdu) != C_IC_NA_1)
            asduHandlerCalled++;

        return true;
    }

    IEC104TestComp* iec104 = nullptr;
    static int ingestCallbackCalled;
    static std::vector<Reading*> storedReadings;
    static int clockSyncHandlerCalled;
    static int asduHandlerCalled;
    static IMasterConnection lastConnection;
    static int lastOA;
};

int ControlCommandsTest::ingestCallbackCalled;
std::vector<Reading*> ControlCommandsTest::storedReadings;
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

    CS104_Slave slave = CS104_Slave_create(15, 15);

    CS104_Slave_setLocalPort(slave, TEST_PORT);

    CS104_Slave_setClockSyncHandler(slave, clockSynchronizationHandler, this);
    CS104_Slave_setASDUHandler(slave, asduHandler, this);

    CS104_Slave_start(slave);

    CS101_AppLayerParameters alParams = CS104_Slave_getAppLayerParameters(slave);

    startIEC104();

    Thread_sleep(500);

    PLUGIN_PARAMETER* params[9];

    PLUGIN_PARAMETER type = {"type", "C_SC_NA_1"};
    params[0] = &type;

    PLUGIN_PARAMETER ca = {"ca", "41025"};
    params[1] = &ca;

    // ioa
    PLUGIN_PARAMETER ioa = {"ioa", "2000"};
    params[2] = &ioa;

    // Third value
    PLUGIN_PARAMETER value = {"", "1"};
    params[8] = &value;

    // Third value
    PLUGIN_PARAMETER select = {"", "0"};
    params[5] = &select;

    // quality update for measurement data points
    ASSERT_EQ(3, ingestCallbackCalled);

    bool operationResult = iec104->operation("IEC104Command", 9, params);

    ASSERT_TRUE(operationResult);

    Thread_sleep(500);

    ASSERT_EQ(1, asduHandlerCalled);

    CS101_ASDU ctAsdu = CS101_ASDU_create(IMasterConnection_getApplicationLayerParameters(lastConnection),
        false, CS101_COT_ACTIVATION_TERMINATION,lastOA, 41025, false, false);

    InformationObject io = (InformationObject)SingleCommand_create(NULL, 2000, true, false, 0);

    CS101_ASDU_addInformationObject(ctAsdu, io);

    IMasterConnection_sendASDU(lastConnection, ctAsdu);

    InformationObject_destroy(io);

    CS101_ASDU_destroy(ctAsdu);

    ASSERT_EQ(10, lastOA);

    Thread_sleep(500);

    // expect ingest callback called two more times:
    //  1. ACT_CON for single command
    //  2. ACT_TERM for single command
    ASSERT_EQ(3 + 2, ingestCallbackCalled);

    CS104_Slave_stop(slave);

    CS104_Slave_destroy(slave);
}

TEST_F(ControlCommandsTest, IEC104Client_sendSingleCommandNotInExchangeConfig)
{
    asduHandlerCalled = 0;
    clockSyncHandlerCalled = 0;
    lastConnection = NULL;
    ingestCallbackCalled = 0;

    CS104_Slave slave = CS104_Slave_create(15, 15);

    CS104_Slave_setLocalPort(slave, TEST_PORT);

    CS104_Slave_setClockSyncHandler(slave, clockSynchronizationHandler, this);
    CS104_Slave_setASDUHandler(slave, asduHandler, this);

    CS104_Slave_start(slave);

    CS101_AppLayerParameters alParams = CS104_Slave_getAppLayerParameters(slave);

    startIEC104();

    Thread_sleep(500);

    PLUGIN_PARAMETER* params[9];

    PLUGIN_PARAMETER type = {"type", "C_SC_NA_1"};
    params[0] = &type;

    PLUGIN_PARAMETER ca = {"ca", "41025"};
    params[1] = &ca;

    // ioa
    PLUGIN_PARAMETER ioa = {"ioa", "2022"};
    params[2] = &ioa;

    // Third value
    PLUGIN_PARAMETER value = {"", "1"};
    params[8] = &value;

    // Third value
    PLUGIN_PARAMETER select = {"", "0"};
    params[5] = &select;
    bool operationResult = iec104->operation("SingleCommand", 9, params);

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

    CS104_Slave slave = CS104_Slave_create(15, 15);

    CS104_Slave_setLocalPort(slave, TEST_PORT);

    CS104_Slave_setClockSyncHandler(slave, clockSynchronizationHandler, this);
    CS104_Slave_setASDUHandler(slave, asduHandler, this);

    CS104_Slave_start(slave);

    CS101_AppLayerParameters alParams = CS104_Slave_getAppLayerParameters(slave);

    startIEC104();

    Thread_sleep(500);

    // quality update for measurement data points
    ASSERT_EQ(3, ingestCallbackCalled);

    PLUGIN_PARAMETER* params[9];

    PLUGIN_PARAMETER type = {"type", "C_DC_NA_1"};
    params[0] = &type;

    PLUGIN_PARAMETER ca = {"ca", "41025"};
    params[1] = &ca;

    // ioa
    PLUGIN_PARAMETER ioa = {"ioa", "2002"};
    params[2] = &ioa;

    // Third value
    PLUGIN_PARAMETER value = {"", "2"};
    params[8] = &value;

    // Third value
    PLUGIN_PARAMETER select = {"", "0"};
    params[5] = &select;
    bool operationResult = iec104->operation("IEC104Command", 9, params);

    ASSERT_TRUE(operationResult);

    Thread_sleep(500);

    ASSERT_EQ(1, asduHandlerCalled);

    CS101_ASDU ctAsdu = CS101_ASDU_create(IMasterConnection_getApplicationLayerParameters(lastConnection),
        false, CS101_COT_ACTIVATION_TERMINATION,lastOA, 41025, false, false);

    InformationObject io = (InformationObject)DoubleCommand_create(NULL, 2002, 2, false, 0);

    CS101_ASDU_addInformationObject(ctAsdu, io);

    IMasterConnection_sendASDU(lastConnection, ctAsdu);

    InformationObject_destroy(io);

    CS101_ASDU_destroy(ctAsdu);

    ASSERT_EQ(10, lastOA);

    Thread_sleep(500);

    // expect ingest callback called two more times:
    //  1. ACT_CON for single command
    //  2. ACT_TERM for single command
    ASSERT_EQ(3 + 2, ingestCallbackCalled);

    CS104_Slave_stop(slave);

    CS104_Slave_destroy(slave);
}

TEST_F(ControlCommandsTest, IEC104Client_sendDoubleCommandWithTimestamp)
{
    asduHandlerCalled = 0;
    clockSyncHandlerCalled = 0;
    lastConnection = NULL;
    ingestCallbackCalled = 0;

    CS104_Slave slave = CS104_Slave_create(15, 15);

    CS104_Slave_setLocalPort(slave, TEST_PORT);

    CS104_Slave_setClockSyncHandler(slave, clockSynchronizationHandler, this);
    CS104_Slave_setASDUHandler(slave, asduHandler, this);

    CS104_Slave_start(slave);

    CS101_AppLayerParameters alParams = CS104_Slave_getAppLayerParameters(slave);

    startIEC104();

    Thread_sleep(500);

    // quality update for measurement data points
    ASSERT_EQ(3, ingestCallbackCalled);

     PLUGIN_PARAMETER* params[9];

    PLUGIN_PARAMETER type = {"type", "C_DC_TA_1"};
    params[0] = &type;

    PLUGIN_PARAMETER ca = {"ca", "41025"};
    params[1] = &ca;

    // ioa
    PLUGIN_PARAMETER ioa = {"ioa", "2004"};
    params[2] = &ioa;

    // Third value
    PLUGIN_PARAMETER value = {"", "1"};
    params[8] = &value;

    // Third value
    PLUGIN_PARAMETER select = {"", "0"};
    params[5] = &select;
    PLUGIN_PARAMETER ts = {"ts", "2515224"};
    params[7] = &ts;

    bool operationResult = iec104->operation("IEC104Command", 9, params);

    ASSERT_TRUE(operationResult);

    Thread_sleep(500);

    ASSERT_EQ(1, asduHandlerCalled);

    CS101_ASDU ctAsdu = CS101_ASDU_create(IMasterConnection_getApplicationLayerParameters(lastConnection),
        false, CS101_COT_ACTIVATION_TERMINATION,lastOA, 41025, false, false);

    CP56Time2a timestamp = CP56Time2a_createFromMsTimestamp(NULL, Hal_getTimeInMs());

    InformationObject io = (InformationObject)DoubleCommandWithCP56Time2a_create(NULL, 2004, 1, false, 0, timestamp);

    CS101_ASDU_addInformationObject(ctAsdu, io);

    IMasterConnection_sendASDU(lastConnection, ctAsdu);

    InformationObject_destroy(io);

    free(timestamp);

    CS101_ASDU_destroy(ctAsdu);

    ASSERT_EQ(10, lastOA);

    Thread_sleep(500);

    // expect ingest callback called two more times:
    //  1. ACT_CON for single command
    //  2. ACT_TERM for single command
    ASSERT_EQ(3 + 2, ingestCallbackCalled);

    CS104_Slave_stop(slave);

    CS104_Slave_destroy(slave);
}

TEST_F(ControlCommandsTest, IEC104Client_sendDoubleCommandNotConnected)
{
    asduHandlerCalled = 0;
    clockSyncHandlerCalled = 0;
    lastConnection = NULL;
    ingestCallbackCalled = 0;

    CS104_Slave slave = CS104_Slave_create(15, 15);

    CS104_Slave_setLocalPort(slave, TEST_PORT);

    CS104_Slave_setClockSyncHandler(slave, clockSynchronizationHandler, this);
    CS104_Slave_setASDUHandler(slave, asduHandler, this);

    CS101_AppLayerParameters alParams = CS104_Slave_getAppLayerParameters(slave);

    startIEC104();

    Thread_sleep(500);

    PLUGIN_PARAMETER* params[9];

    PLUGIN_PARAMETER type = {"type", "C_DC_NA_1"};
    params[0] = &type;

    PLUGIN_PARAMETER ca = {"ca", "41025"};
    params[1] = &ca;

    // ioa
    PLUGIN_PARAMETER ioa = {"ioa", "2002"};
    params[2] = &ioa;

    // Third value
    PLUGIN_PARAMETER value = {"", "2"};
    params[8] = &value;

    // Third value
    PLUGIN_PARAMETER select = {"", "0"};
    params[5] = &select;
    bool operationResult = iec104->operation("IEC104Command", 9, params);

    ASSERT_FALSE(operationResult);

    CS104_Slave_destroy(slave);
}

TEST_F(ControlCommandsTest, IEC104Client_sendSetpointCommandShort)
{
    asduHandlerCalled = 0;
    clockSyncHandlerCalled = 0;
    lastConnection = NULL;
    ingestCallbackCalled = 0;

    CS104_Slave slave = CS104_Slave_create(15, 15);

    CS104_Slave_setLocalPort(slave, TEST_PORT);

    CS104_Slave_setClockSyncHandler(slave, clockSynchronizationHandler, this);
    CS104_Slave_setASDUHandler(slave, asduHandler, this);

    CS104_Slave_start(slave);

    CS101_AppLayerParameters alParams = CS104_Slave_getAppLayerParameters(slave);

    startIEC104();

    Thread_sleep(500);

    PLUGIN_PARAMETER* params[9];

    PLUGIN_PARAMETER type = {"type", "C_SE_TC_1"};
    params[0] = &type;

    PLUGIN_PARAMETER ca = {"ca", "41025"};
    params[1] = &ca;

    // ioa
    PLUGIN_PARAMETER ioa = {"ioa", "2003"};
    params[2] = &ioa;

    // Third value
    PLUGIN_PARAMETER value = {"", "2.1"};
    params[8] = &value;

    // Third value
    PLUGIN_PARAMETER select = {"", "0"};
    params[5] = &select;

    PLUGIN_PARAMETER ts = {"ts", "2515224"};
    params[7] = &ts;

    bool operationResult = iec104->operation("IEC104Command", 9, params);

    ASSERT_TRUE(operationResult);

    Thread_sleep(500);

    ASSERT_EQ(1, asduHandlerCalled);

    CS101_ASDU ctAsdu = CS101_ASDU_create(IMasterConnection_getApplicationLayerParameters(lastConnection),
        false, CS101_COT_ACTIVATION_TERMINATION, lastOA, 41025, false, false);

    CP56Time2a timestamp = CP56Time2a_createFromMsTimestamp(NULL, Hal_getTimeInMs());

    InformationObject io = (InformationObject)SetpointCommandShortWithCP56Time2a_create(NULL, 2003, 1.5f, false, 0, timestamp);

    CS101_ASDU_addInformationObject(ctAsdu, io);

    InformationObject_destroy(io);

    free(timestamp);

    IMasterConnection_sendASDU(lastConnection, ctAsdu);

    CS101_ASDU_destroy(ctAsdu);

    ASSERT_EQ(10, lastOA);

    Thread_sleep(500);

    // expect ingest callback called two more times:
    //  1. ACT_CON for single command
    //  2. ACT_TERM for single command
    ASSERT_EQ(3 + 2, ingestCallbackCalled);

    CS104_Slave_stop(slave);

    CS104_Slave_destroy(slave);
}

TEST_F(ControlCommandsTest, IEC104Client_sendSetpointCommandShortNoTime)
{
    asduHandlerCalled = 0;
    clockSyncHandlerCalled = 0;
    lastConnection = NULL;
    ingestCallbackCalled = 0;

    CS104_Slave slave = CS104_Slave_create(15, 15);

    CS104_Slave_setLocalPort(slave, TEST_PORT);

    CS104_Slave_setClockSyncHandler(slave, clockSynchronizationHandler, this);
    CS104_Slave_setASDUHandler(slave, asduHandler, this);

    CS104_Slave_start(slave);

    CS101_AppLayerParameters alParams = CS104_Slave_getAppLayerParameters(slave);

    startIEC104();

    Thread_sleep(500);

    PLUGIN_PARAMETER* params[9];

    PLUGIN_PARAMETER type = {"type", "C_SE_NC_1"};
    params[0] = &type;

    PLUGIN_PARAMETER ca = {"ca", "41025"};
    params[1] = &ca;

    // ioa
    PLUGIN_PARAMETER ioa = {"ioa", "2012"};
    params[2] = &ioa;

    // Third value
    PLUGIN_PARAMETER value = {"", "1.5"};
    params[8] = &value;

    // Third value
    PLUGIN_PARAMETER select = {"", "0"};
    params[5] = &select;

    bool operationResult = iec104->operation("IEC104Command", 9, params);

    ASSERT_TRUE(operationResult);

    Thread_sleep(500);

    ASSERT_EQ(1, asduHandlerCalled);

    CS101_ASDU ctAsdu = CS101_ASDU_create(IMasterConnection_getApplicationLayerParameters(lastConnection),
        false, CS101_COT_ACTIVATION_TERMINATION, lastOA, 41025, false, false);

    CP56Time2a timestamp = CP56Time2a_createFromMsTimestamp(NULL, Hal_getTimeInMs());

    InformationObject io = (InformationObject)SetpointCommandShort_create(NULL, 2012, 1.5f, false, 0);

    CS101_ASDU_addInformationObject(ctAsdu, io);

    InformationObject_destroy(io);

    IMasterConnection_sendASDU(lastConnection, ctAsdu);

    CS101_ASDU_destroy(ctAsdu);
    free(timestamp);

    ASSERT_EQ(10, lastOA);

    Thread_sleep(500);

    // expect ingest callback called two more times:
    //  1. ACT_CON for single command
    //  2. ACT_TERM for single command
    ASSERT_EQ(3 + 2, ingestCallbackCalled);

    CS104_Slave_stop(slave);

    CS104_Slave_destroy(slave);
}


TEST_F(ControlCommandsTest, IEC104Client_sendSetpointNormalizedTime)
{
    asduHandlerCalled = 0;
    clockSyncHandlerCalled = 0;
    lastConnection = NULL;
    ingestCallbackCalled = 0;

    CS104_Slave slave = CS104_Slave_create(15, 15);

    CS104_Slave_setLocalPort(slave, TEST_PORT);

    CS104_Slave_setClockSyncHandler(slave, clockSynchronizationHandler, this);
    CS104_Slave_setASDUHandler(slave, asduHandler, this);

    CS104_Slave_start(slave);

    CS101_AppLayerParameters alParams = CS104_Slave_getAppLayerParameters(slave);

    startIEC104();

    Thread_sleep(500);

    PLUGIN_PARAMETER* params[9];

    PLUGIN_PARAMETER type = {"type", "C_SE_TA_1"};
    params[0] = &type;

    PLUGIN_PARAMETER ca = {"ca", "41025"};
    params[1] = &ca;

    // ioa
    PLUGIN_PARAMETER ioa = {"ioa", "2007"};
    params[2] = &ioa;

    // Third value
    PLUGIN_PARAMETER value = {"", "1.0"};
    params[8] = &value;

    // Third value
    PLUGIN_PARAMETER select = {"", "0"};
    params[5] = &select;

    // Third value
    PLUGIN_PARAMETER ts = {"", "123553"};
    params[7] = &ts;

    bool operationResult = iec104->operation("IEC104Command", 9, params);

    ASSERT_TRUE(operationResult);

    Thread_sleep(500);

    ASSERT_EQ(1, asduHandlerCalled);

    CS101_ASDU ctAsdu = CS101_ASDU_create(IMasterConnection_getApplicationLayerParameters(lastConnection),
        false, CS101_COT_ACTIVATION_TERMINATION, lastOA, 41025, false, false);

    CP56Time2a timestamp = CP56Time2a_createFromMsTimestamp(NULL, Hal_getTimeInMs());

    InformationObject io = (InformationObject)SetpointCommandNormalizedWithCP56Time2a_create(NULL, 2007, 1.0f, false, 0,timestamp);

    CS101_ASDU_addInformationObject(ctAsdu, io);

    InformationObject_destroy(io);

    free(timestamp);

    IMasterConnection_sendASDU(lastConnection, ctAsdu);

    CS101_ASDU_destroy(ctAsdu);

    ASSERT_EQ(10, lastOA);

    Thread_sleep(500);

    // expect ingest callback called two more times:
    //  1. ACT_CON for single command
    //  2. ACT_TERM for single command
    ASSERT_EQ(3 + 2, ingestCallbackCalled);

    CS104_Slave_stop(slave);

    CS104_Slave_destroy(slave);
}

TEST_F(ControlCommandsTest, IEC104Client_sendSetpointCommandNormalizedNoTime)
{
    asduHandlerCalled = 0;
    clockSyncHandlerCalled = 0;
    lastConnection = NULL;
    ingestCallbackCalled = 0;

    CS104_Slave slave = CS104_Slave_create(15, 15);

    CS104_Slave_setLocalPort(slave, TEST_PORT);

    CS104_Slave_setClockSyncHandler(slave, clockSynchronizationHandler, this);
    CS104_Slave_setASDUHandler(slave, asduHandler, this);

    CS104_Slave_start(slave);

    CS101_AppLayerParameters alParams = CS104_Slave_getAppLayerParameters(slave);

    startIEC104();

    Thread_sleep(500);

    PLUGIN_PARAMETER* params[9];

    PLUGIN_PARAMETER type = {"type", "C_SE_NA_1"};
    params[0] = &type;

    PLUGIN_PARAMETER ca = {"ca", "41025"};
    params[1] = &ca;

    // ioa
    PLUGIN_PARAMETER ioa = {"ioa", "2006"};
    params[2] = &ioa;

    // Third value
    PLUGIN_PARAMETER value = {"", "1.0"};
    params[8] = &value;

    // Third value
    PLUGIN_PARAMETER select = {"", "0"};
    params[5] = &select;

    bool operationResult = iec104->operation("IEC104Command", 9, params);

    ASSERT_TRUE(operationResult);

    Thread_sleep(500);

    ASSERT_EQ(1, asduHandlerCalled);

    CS101_ASDU ctAsdu = CS101_ASDU_create(IMasterConnection_getApplicationLayerParameters(lastConnection),
        false, CS101_COT_ACTIVATION_TERMINATION, lastOA, 41025, false, false);

    CP56Time2a timestamp = CP56Time2a_createFromMsTimestamp(NULL, Hal_getTimeInMs());

    InformationObject io = (InformationObject)SetpointCommandNormalized_create(NULL, 2006, 1.0f, false, 0);

    CS101_ASDU_addInformationObject(ctAsdu, io);

    InformationObject_destroy(io);

    free(timestamp);

    IMasterConnection_sendASDU(lastConnection, ctAsdu);

    CS101_ASDU_destroy(ctAsdu);

    ASSERT_EQ(10, lastOA);

    Thread_sleep(500);

    // expect ingest callback called two more times:
    //  1. ACT_CON for single command
    //  2. ACT_TERM for single command
    ASSERT_EQ(3 + 2, ingestCallbackCalled);

    CS104_Slave_stop(slave);

    CS104_Slave_destroy(slave);
}

TEST_F(ControlCommandsTest, IEC104Client_sendSetpointScaledNoTime)
{
    asduHandlerCalled = 0;
    clockSyncHandlerCalled = 0;
    lastConnection = NULL;
    ingestCallbackCalled = 0;

    CS104_Slave slave = CS104_Slave_create(15, 15);

    CS104_Slave_setLocalPort(slave, TEST_PORT);

    CS104_Slave_setClockSyncHandler(slave, clockSynchronizationHandler, this);
    CS104_Slave_setASDUHandler(slave, asduHandler, this);

    CS104_Slave_start(slave);

    CS101_AppLayerParameters alParams = CS104_Slave_getAppLayerParameters(slave);

    startIEC104();

    Thread_sleep(500);

    PLUGIN_PARAMETER* params[9];

    PLUGIN_PARAMETER type = {"type", "C_SE_NB_1"};
    params[0] = &type;

    PLUGIN_PARAMETER ca = {"ca", "41025"};
    params[1] = &ca;

    // ioa
    PLUGIN_PARAMETER ioa = {"ioa", "2008"};
    params[2] = &ioa;

    // Third value
    PLUGIN_PARAMETER value = {"", "235"};
    params[8] = &value;

    // Third value
    PLUGIN_PARAMETER select = {"", "0"};
    params[5] = &select;

    bool operationResult = iec104->operation("IEC104Command", 9, params);

    ASSERT_TRUE(operationResult);

    Thread_sleep(500);

    ASSERT_EQ(1, asduHandlerCalled);

    CS101_ASDU ctAsdu = CS101_ASDU_create(IMasterConnection_getApplicationLayerParameters(lastConnection),
        false, CS101_COT_ACTIVATION_TERMINATION, lastOA, 41025, false, false);

    CP56Time2a timestamp = CP56Time2a_createFromMsTimestamp(NULL, Hal_getTimeInMs());

    InformationObject io = (InformationObject)SetpointCommandScaled_create(NULL, 2008, 235, false, 0);

    CS101_ASDU_addInformationObject(ctAsdu, io);

    InformationObject_destroy(io);

    free(timestamp);

    IMasterConnection_sendASDU(lastConnection, ctAsdu);

    CS101_ASDU_destroy(ctAsdu);

    ASSERT_EQ(10, lastOA);

    Thread_sleep(500);

    // expect ingest callback called two more times:
    //  1. ACT_CON for single command
    //  2. ACT_TERM for single command
    ASSERT_EQ(3 + 2, ingestCallbackCalled);

    CS104_Slave_stop(slave);

    CS104_Slave_destroy(slave);
}

TEST_F(ControlCommandsTest, IEC104Client_sendSetpointCommandScaledWithTime)
{
    asduHandlerCalled = 0;
    clockSyncHandlerCalled = 0;
    lastConnection = NULL;
    ingestCallbackCalled = 0;

    CS104_Slave slave = CS104_Slave_create(15, 15);

    CS104_Slave_setLocalPort(slave, TEST_PORT);

    CS104_Slave_setClockSyncHandler(slave, clockSynchronizationHandler, this);
    CS104_Slave_setASDUHandler(slave, asduHandler, this);

    CS104_Slave_start(slave);

    CS101_AppLayerParameters alParams = CS104_Slave_getAppLayerParameters(slave);

    startIEC104();

    Thread_sleep(500);

    PLUGIN_PARAMETER* params[9];

    PLUGIN_PARAMETER type = {"type", "C_SE_TB_1"};
    params[0] = &type;

    PLUGIN_PARAMETER ca = {"ca", "41025"};
    params[1] = &ca;

    // ioa
    PLUGIN_PARAMETER ioa = {"ioa", "2009"};
    params[2] = &ioa;

    // Third value
    PLUGIN_PARAMETER value = {"", "235"};
    params[8] = &value;

    // Third value
    PLUGIN_PARAMETER select = {"", "0"};
    params[5] = &select;
    // Third value
    PLUGIN_PARAMETER ts = {"", "12141"};
    params[7] = &ts;

    bool operationResult = iec104->operation("IEC104Command", 9, params);

    ASSERT_TRUE(operationResult);

    Thread_sleep(500);

    ASSERT_EQ(1, asduHandlerCalled);

    CS101_ASDU ctAsdu = CS101_ASDU_create(IMasterConnection_getApplicationLayerParameters(lastConnection),
        false, CS101_COT_ACTIVATION_TERMINATION, lastOA, 41025, false, false);

    CP56Time2a timestamp = CP56Time2a_createFromMsTimestamp(NULL, Hal_getTimeInMs());

    InformationObject io = (InformationObject)SetpointCommandScaledWithCP56Time2a_create(NULL, 2009, 235, false, 0,timestamp);

    CS101_ASDU_addInformationObject(ctAsdu, io);

    InformationObject_destroy(io);

    free(timestamp);

    IMasterConnection_sendASDU(lastConnection, ctAsdu);

    CS101_ASDU_destroy(ctAsdu);

    ASSERT_EQ(10, lastOA);

    Thread_sleep(500);

    // expect ingest callback called two more times:
    //  1. ACT_CON for single command
    //  2. ACT_TERM for single command
    ASSERT_EQ(3 + 2, ingestCallbackCalled);

    CS104_Slave_stop(slave);

    CS104_Slave_destroy(slave);
}

TEST_F(ControlCommandsTest, IEC104Client_sendSinglePointCommandWithTime)
{
    asduHandlerCalled = 0;
    clockSyncHandlerCalled = 0;
    lastConnection = NULL;
    ingestCallbackCalled = 0;

    CS104_Slave slave = CS104_Slave_create(15, 15);

    CS104_Slave_setLocalPort(slave, TEST_PORT);

    CS104_Slave_setClockSyncHandler(slave, clockSynchronizationHandler, this);
    CS104_Slave_setASDUHandler(slave, asduHandler, this);

    CS104_Slave_start(slave);

    CS101_AppLayerParameters alParams = CS104_Slave_getAppLayerParameters(slave);

    startIEC104();

    Thread_sleep(500);

    PLUGIN_PARAMETER* params[9];

    PLUGIN_PARAMETER type = {"type", "C_SC_TA_1"};
    params[0] = &type;

    PLUGIN_PARAMETER ca = {"ca", "41025"};
    params[1] = &ca;

    // ioa
    PLUGIN_PARAMETER ioa = {"ioa", "2010"};
    params[2] = &ioa;

    // Third value
    PLUGIN_PARAMETER value = {"", "1"};
    params[8] = &value;

    // Third value
    PLUGIN_PARAMETER select = {"", "0"};
    params[5] = &select;

    // Third value
    PLUGIN_PARAMETER ts = {"", "346643"};
    params[7] = &ts;

    bool operationResult = iec104->operation("IEC104Command", 9, params);

    ASSERT_TRUE(operationResult);

    Thread_sleep(500);

    ASSERT_EQ(1, asduHandlerCalled);
    CS101_ASDU ctAsdu = CS101_ASDU_create(IMasterConnection_getApplicationLayerParameters(lastConnection),
        false, CS101_COT_ACTIVATION_TERMINATION, lastOA, 41025, false, false);

    CP56Time2a timestamp = CP56Time2a_createFromMsTimestamp(NULL, Hal_getTimeInMs());

    InformationObject io = (InformationObject)SingleCommandWithCP56Time2a_create(NULL, 2010, false, false, 0,timestamp);

    CS101_ASDU_addInformationObject(ctAsdu, io);

    InformationObject_destroy(io);

    free(timestamp);

    IMasterConnection_sendASDU(lastConnection, ctAsdu);

    CS101_ASDU_destroy(ctAsdu);

    ASSERT_EQ(10, lastOA);

    Thread_sleep(500);

    // expect ingest callback called two more times:
    //  1. ACT_CON for single command
    //  2. ACT_TERM for single command
    ASSERT_EQ(3 + 2, ingestCallbackCalled);

    CS104_Slave_stop(slave);

    CS104_Slave_destroy(slave);
}

TEST_F(ControlCommandsTest, IEC104Client_sendStepCommandTime)
{
    asduHandlerCalled = 0;
    clockSyncHandlerCalled = 0;
    lastConnection = NULL;
    ingestCallbackCalled = 0;

    CS104_Slave slave = CS104_Slave_create(15, 15);

    CS104_Slave_setLocalPort(slave, TEST_PORT);

    CS104_Slave_setClockSyncHandler(slave, clockSynchronizationHandler, this);
    CS104_Slave_setASDUHandler(slave, asduHandler, this);

    CS104_Slave_start(slave);

    CS101_AppLayerParameters alParams = CS104_Slave_getAppLayerParameters(slave);

    startIEC104();

    Thread_sleep(500);

    PLUGIN_PARAMETER* params[9];

    PLUGIN_PARAMETER type = {"type", "C_RC_TA_1"};
    params[0] = &type;

    PLUGIN_PARAMETER ca = {"ca", "41025"};
    params[1] = &ca;

    // ioa
    PLUGIN_PARAMETER ioa = {"ioa", "2011"};
    params[2] = &ioa;

    // Third value
    PLUGIN_PARAMETER value = {"", "2"};
    params[8] = &value;

    // Third value
    PLUGIN_PARAMETER select = {"", "0"};
    params[5] = &select;

    // Third value
    PLUGIN_PARAMETER ts = {"", "3535"};
    params[7] = &ts;

    bool operationResult = iec104->operation("IEC104Command", 9, params);

    ASSERT_TRUE(operationResult);

    Thread_sleep(500);

    ASSERT_EQ(1, asduHandlerCalled);

    CS101_ASDU ctAsdu = CS101_ASDU_create(IMasterConnection_getApplicationLayerParameters(lastConnection),
        false, CS101_COT_ACTIVATION_TERMINATION, lastOA, 41025, false, false);

    CP56Time2a timestamp = CP56Time2a_createFromMsTimestamp(NULL, Hal_getTimeInMs());

    InformationObject io = (InformationObject)StepCommandWithCP56Time2a_create(NULL, 2011, IEC60870_STEP_HIGHER, false, 0,timestamp);

    CS101_ASDU_addInformationObject(ctAsdu, io);

    InformationObject_destroy(io);

    free(timestamp);

    IMasterConnection_sendASDU(lastConnection, ctAsdu);

    CS101_ASDU_destroy(ctAsdu);

    ASSERT_EQ(10, lastOA);

    Thread_sleep(500);

    // expect ingest callback called two more times:
    //  1. ACT_CON for single command
    //  2. ACT_TERM for single command
    ASSERT_EQ(3 + 2, ingestCallbackCalled);

    CS104_Slave_stop(slave);

    CS104_Slave_destroy(slave);
}


TEST_F(ControlCommandsTest, IEC104Client_sendTwoSingleCommands)
{
    asduHandlerCalled = 0;
    clockSyncHandlerCalled = 0;
    lastConnection = NULL;
    ingestCallbackCalled = 0;

    CS104_Slave slave = CS104_Slave_create(15, 15);

    CS104_Slave_setLocalPort(slave, TEST_PORT);

    CS104_Slave_setClockSyncHandler(slave, clockSynchronizationHandler, this);
    CS104_Slave_setASDUHandler(slave, asduHandler, this);

    CS104_Slave_start(slave);

    CS101_AppLayerParameters alParams = CS104_Slave_getAppLayerParameters(slave);

    startIEC104();

    Thread_sleep(500);

    PLUGIN_PARAMETER* params[9];

    PLUGIN_PARAMETER type = {"type", "C_SC_NA_1"};
    params[0] = &type;

    PLUGIN_PARAMETER ca = {"ca", "41025"};
    params[1] = &ca;

    // ioa
    PLUGIN_PARAMETER ioa = {"ioa", "2000"};
    params[2] = &ioa;

    // Third value
    PLUGIN_PARAMETER value = {"", "1"};
    params[8] = &value;

    // Third value
    PLUGIN_PARAMETER select = {"", "0"};
    params[5] = &select;
    // quality update for measurement data points
    ASSERT_EQ(3, ingestCallbackCalled);

    bool operationResult = iec104->operation("IEC104Command", 9, params);

    ASSERT_TRUE(operationResult);

    params[1]->value = "2001";

    operationResult = iec104->operation("IEC104Command", 9, params);

    ASSERT_FALSE(operationResult);

    Thread_sleep(500);

    ASSERT_EQ(1, asduHandlerCalled);

    CS101_ASDU ctAsdu = CS101_ASDU_create(IMasterConnection_getApplicationLayerParameters(lastConnection),
        false, CS101_COT_ACTIVATION_TERMINATION,lastOA, 41025, false, false);

    InformationObject io = (InformationObject)SingleCommand_create(NULL, 2000, true, false, 0);

    CS101_ASDU_addInformationObject(ctAsdu, io);

    IMasterConnection_sendASDU(lastConnection, ctAsdu);

    InformationObject_destroy(io);

    CS101_ASDU_destroy(ctAsdu);

    ASSERT_EQ(10, lastOA);

    Thread_sleep(500);

    // expect ingest callback called two more times:
    //  1. ACT_CON for single command
    //  2. ACT_TERM for single command
    ASSERT_EQ(3 + 2, ingestCallbackCalled);

    CS104_Slave_stop(slave);

    CS104_Slave_destroy(slave);
}

TEST_F(ControlCommandsTest, IEC104Client_sendStepCommand)
{
    asduHandlerCalled = 0;
    clockSyncHandlerCalled = 0;
    lastConnection = NULL;
    ingestCallbackCalled = 0;

    CS104_Slave slave = CS104_Slave_create(15, 15);

    CS104_Slave_setLocalPort(slave, TEST_PORT);

    CS104_Slave_setClockSyncHandler(slave, clockSynchronizationHandler, this);
    CS104_Slave_setASDUHandler(slave, asduHandler, this);

    CS104_Slave_start(slave);

    CS101_AppLayerParameters alParams = CS104_Slave_getAppLayerParameters(slave);

    startIEC104();

    Thread_sleep(500);

    PLUGIN_PARAMETER* params[9];

    PLUGIN_PARAMETER type = {"type", "C_RC_NA_1"};
    params[0] = &type;

    PLUGIN_PARAMETER ca = {"ca", "41025"};
    params[1] = &ca;

    // ioa
    PLUGIN_PARAMETER ioa = {"ioa", "2005"};
    params[2] = &ioa;

    // Third value
    PLUGIN_PARAMETER value = {"", "2"};
    params[8] = &value;

    // Third value
    PLUGIN_PARAMETER select = {"", "0"};
    params[5] = &select;

    // quality update for measurement data points
    ASSERT_EQ(3, ingestCallbackCalled);

    bool operationResult = iec104->operation("IEC104Command", 9, params);

    ASSERT_TRUE(operationResult);

    Thread_sleep(500);

    ASSERT_EQ(1, asduHandlerCalled);

    CS101_ASDU ctAsdu = CS101_ASDU_create(IMasterConnection_getApplicationLayerParameters(lastConnection),
        false, CS101_COT_ACTIVATION_TERMINATION,lastOA, 41025, false, false);

    InformationObject io = (InformationObject)StepCommand_create(NULL, 2005, IEC60870_STEP_HIGHER, false, 0);

    CS101_ASDU_addInformationObject(ctAsdu, io);

    IMasterConnection_sendASDU(lastConnection, ctAsdu);

    InformationObject_destroy(io);

    CS101_ASDU_destroy(ctAsdu);

    ASSERT_EQ(10, lastOA);

    Thread_sleep(500);

    // expect ingest callback called two more times:
    //  1. ACT_CON for single command
    //  2. ACT_TERM for single command
    ASSERT_EQ(3 + 2, ingestCallbackCalled);

    CS104_Slave_stop(slave);

    CS104_Slave_destroy(slave);
}

TEST_F(ControlCommandsTest, IEC104Client_sendInterrogationCommand)
{
    asduHandlerCalled = 0;
    clockSyncHandlerCalled = 0;
    lastConnection = NULL;
    ingestCallbackCalled = 0;

    CS104_Slave slave = CS104_Slave_create(15, 15);

    CS104_Slave_setLocalPort(slave, TEST_PORT);

    CS104_Slave_setClockSyncHandler(slave, clockSynchronizationHandler, this);
    CS104_Slave_setASDUHandler(slave, asduHandler, this);

    CS104_Slave_start(slave);

    CS101_AppLayerParameters alParams = CS104_Slave_getAppLayerParameters(slave);

    startIEC104();

    Thread_sleep(500);

    PLUGIN_PARAMETER* params[1];
    PLUGIN_PARAMETER ca = {"ca", "41025"};
    params[0] = &ca;

    // quality update for measurement data points
    ASSERT_EQ(3, ingestCallbackCalled);

    bool operationResult = iec104->operation("CS104_Connection_sendInterrogationCommand", 1, params);

    ASSERT_TRUE(operationResult);

    CS104_Slave_stop(slave);

    CS104_Slave_destroy(slave);
}

TEST_F(ControlCommandsTest, IEC104Client_sendBrokenCommands1)
{
    asduHandlerCalled = 0;
    clockSyncHandlerCalled = 0;
    lastConnection = NULL;
    ingestCallbackCalled = 0;

    CS104_Slave slave = CS104_Slave_create(15, 15);

    CS104_Slave_setLocalPort(slave, TEST_PORT);

    CS104_Slave_setClockSyncHandler(slave, clockSynchronizationHandler, this);
    CS104_Slave_setASDUHandler(slave, asduHandler, this);

    CS104_Slave_start(slave);

    CS101_AppLayerParameters alParams = CS104_Slave_getAppLayerParameters(slave);

    startIEC104();

    Thread_sleep(500);

    PLUGIN_PARAMETER* params1[1];

    PLUGIN_PARAMETER type1 = {"type", "C_SC_NA_1"};
    params1[0] = &type1;

    PLUGIN_PARAMETER* params2[1];

    PLUGIN_PARAMETER type2 = {"type", "C_SC_TA_1"};
    params2[0] = &type2;

    PLUGIN_PARAMETER* params3[1];

    PLUGIN_PARAMETER type3 = {"type", "C_DC_NA_1"};
    params3[0] = &type3;

    PLUGIN_PARAMETER* params4[1];

    PLUGIN_PARAMETER type4 = {"type", "C_DC_TA_1"};
    params4[0] = &type4;

    PLUGIN_PARAMETER* params5[1];

    PLUGIN_PARAMETER type5 = {"type", "C_RC_NA_1"};
    params5[0] = &type5;

    PLUGIN_PARAMETER* params6[1];

    PLUGIN_PARAMETER type6 = {"type", "C_RC_TA_1"};
    params6[0] = &type6;

    PLUGIN_PARAMETER* params7[1];

    PLUGIN_PARAMETER type7 = {"type", "C_SE_NA_1"};
    params7[0] = &type7;

    PLUGIN_PARAMETER* params8[1];

    PLUGIN_PARAMETER type8 = {"type", "C_SE_TA_1"};
    params8[0] = &type8;

    PLUGIN_PARAMETER* params9[1];

    PLUGIN_PARAMETER type9 = {"type", "C_SE_NB_1"};
    params9[0] = &type9;

    PLUGIN_PARAMETER* params10[1];

    PLUGIN_PARAMETER type10 = {"type", "C_SE_TB_1"};
    params10[0] = &type10;

    PLUGIN_PARAMETER* params11[1];

    PLUGIN_PARAMETER type11 = {"type", "C_SE_NC_1"};
    params11[0] = &type11;

    PLUGIN_PARAMETER* params12[1];

    PLUGIN_PARAMETER type12 = {"type", "C_SE_TC_1"};
    params12[0] = &type12;

    PLUGIN_PARAMETER* params13[1];

    PLUGIN_PARAMETER type13 = {"type", "C_SE_AA_1"};
    params13[0] = &type13;


    // quality update for measurement data points
    ASSERT_EQ(3, ingestCallbackCalled);

    bool operationResult = iec104->operation("IEC104Command", 1, params1);
    ASSERT_FALSE(operationResult);
    operationResult = iec104->operation("IEC104Command", 1, params2);
    ASSERT_FALSE(operationResult);
    operationResult = iec104->operation("IEC104Command", 1, params3);
    ASSERT_FALSE(operationResult);
    operationResult = iec104->operation("IEC104Command", 1, params4);
    ASSERT_FALSE(operationResult);
    operationResult = iec104->operation("IEC104Command", 1, params5);
    ASSERT_FALSE(operationResult);
    operationResult = iec104->operation("IEC104Command", 1, params6);
    ASSERT_FALSE(operationResult);
    operationResult = iec104->operation("IEC104Command", 1, params7);
    ASSERT_FALSE(operationResult);
    operationResult = iec104->operation("IEC104Command", 1, params8);
    ASSERT_FALSE(operationResult);
    operationResult = iec104->operation("IEC104Command", 1, params9);
    ASSERT_FALSE(operationResult);
    operationResult = iec104->operation("IEC104Command", 1, params10);
    ASSERT_FALSE(operationResult);
    operationResult = iec104->operation("IEC104Command", 1, params11);
    ASSERT_FALSE(operationResult);
    operationResult = iec104->operation("IEC104Command", 1, params12);
    ASSERT_FALSE(operationResult);
    operationResult = iec104->operation("IEC104Command", 1, params13);
    ASSERT_FALSE(operationResult);


    CS104_Slave_stop(slave);

    CS104_Slave_destroy(slave);
}


TEST_F(ControlCommandsTest, IEC104Client_sendBrokenCommands2)
{
    asduHandlerCalled = 0;
    clockSyncHandlerCalled = 0;
    lastConnection = NULL;
    ingestCallbackCalled = 0;

    iec104->setJsonConfig(protocol_config, exchanged_data1, tls_config);

    CS104_Slave slave = CS104_Slave_create(15, 15);

    CS104_Slave_setLocalPort(slave, TEST_PORT);

    CS104_Slave_setClockSyncHandler(slave, clockSynchronizationHandler, this);
    CS104_Slave_setASDUHandler(slave, asduHandler, this);

    CS104_Slave_start(slave);

    CS101_AppLayerParameters alParams = CS104_Slave_getAppLayerParameters(slave);

    startIEC104();

    Thread_sleep(500);

    PLUGIN_PARAMETER* params1[9];

    PLUGIN_PARAMETER type1 = {"type", "C_SC_NA_1"};
    params1[0] = &type1;

    PLUGIN_PARAMETER ca1 = {"ca", "41025"};
    params1[1] = &ca1;

    // ioa
    PLUGIN_PARAMETER ioa1 = {"ioa", "2000"};
    params1[2] = &ioa1;

    // Third value
    PLUGIN_PARAMETER value1 = {"", "1"};
    params1[8] = &value1;

    // Third value
    PLUGIN_PARAMETER select1 = {"", "0"};
    params1[5] = &select1;

    PLUGIN_PARAMETER* params3[9];

    PLUGIN_PARAMETER type3 = {"type", "C_DC_NA_1"};
    params3[0] = &type3;

    PLUGIN_PARAMETER ca3 = {"ca", "41025"};
    params3[1] = &ca3;

    // ioa
    PLUGIN_PARAMETER ioa3 = {"ioa", "2000"};
    params3[2] = &ioa3;

    // Third value
    PLUGIN_PARAMETER value3 = {"", "1"};
    params3[8] = &value3;

    // Third value
    PLUGIN_PARAMETER select3 = {"", "0"};
    params3[5] = &select3;

    PLUGIN_PARAMETER* params5[9];

    PLUGIN_PARAMETER type5 = {"type", "C_RC_NA_1"};
    params5[0] = &type5;

    PLUGIN_PARAMETER ca5 = {"ca", "41025"};
    params5[1] = &ca5;

    // ioa
    PLUGIN_PARAMETER ioa5 = {"ioa", "2000"};
    params5[2] = &ioa5;

    // Third value
    PLUGIN_PARAMETER value5 = {"", "1"};
    params5[8] = &value5;

    // Third value
    PLUGIN_PARAMETER select5 = {"", "0"};
    params5[5] = &select5;

    PLUGIN_PARAMETER* params7[9];

    PLUGIN_PARAMETER type7 = {"type", "C_SE_NA_1"};
    params7[0] = &type7;

    PLUGIN_PARAMETER ca7 = {"ca", "41025"};
    params7[1] = &ca7;

    // ioa
    PLUGIN_PARAMETER ioa7 = {"ioa", "2000"};
    params7[2] = &ioa7;

    // Third value
    PLUGIN_PARAMETER value7 = {"", "1"};
    params7[8] = &value7;

    // Third value
    PLUGIN_PARAMETER select7 = {"", "0"};
    params7[5] = &select7;

    PLUGIN_PARAMETER* params9[9];

    PLUGIN_PARAMETER type9 = {"type", "C_SE_NB_1"};
    params9[0] = &type9;

    PLUGIN_PARAMETER ca9 = {"ca", "41025"};
    params9[1] = &ca9;

    // ioa
    PLUGIN_PARAMETER ioa9 = {"ioa", "2000"};
    params9[2] = &ioa9;

    // Third value
    PLUGIN_PARAMETER value9 = {"", "1"};
    params9[8] = &value9;

    // Third value
    PLUGIN_PARAMETER select9 = {"", "0"};
    params9[5] = &select9;

    PLUGIN_PARAMETER* params11[9];

    PLUGIN_PARAMETER type11 = {"type", "C_SE_NC_1"};
    params11[0] = &type11;

    PLUGIN_PARAMETER ca11 = {"ca", "41025"};
    params11[1] = &ca11;

    // ioa
    PLUGIN_PARAMETER ioa11 = {"ioa", "2000"};
    params11[2] = &ioa11;

    // Third value
    PLUGIN_PARAMETER value11 = {"", "1"};
    params11[8] = &value11;

    // Third value
    PLUGIN_PARAMETER select11 = {"", "0"};
    params11[5] = &select11;


    // quality update for measurement data points
    ASSERT_EQ(3, ingestCallbackCalled);

    bool operationResult = iec104->operation("IEC104Command", 9, params1);
    ASSERT_FALSE(operationResult);

    operationResult = iec104->operation("IEC104Command", 9, params3);
    ASSERT_FALSE(operationResult);

    operationResult = iec104->operation("IEC104Command", 9, params5);
    ASSERT_FALSE(operationResult);

    operationResult = iec104->operation("IEC104Command", 9, params7);
    ASSERT_FALSE(operationResult);

    operationResult = iec104->operation("IEC104Command", 9, params9);
    ASSERT_FALSE(operationResult);

    operationResult = iec104->operation("IEC104Command", 9, params11);
    ASSERT_FALSE(operationResult);


    CS104_Slave_stop(slave);

    CS104_Slave_destroy(slave);
}

