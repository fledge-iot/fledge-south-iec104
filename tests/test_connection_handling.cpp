#include <gtest/gtest.h>

#include <utility>
#include <vector>

#include <config_category.h>
#include <plugin_api.h>

#include "cs104_slave.h"
#include <lib60870/hal_thread.h>

#include "iec104.h"
#include "iec104_client_config.h"

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
                                "srv_ip" : "127.0.0.2",
                                "clt_ip" : "127.0.0.3",
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
                "asdu_size" : 0,
                "gi_time" : 60,
                "gi_cycle" : 30,
                "gi_all_ca" : false,
                "utc_time" : false,
                "cmd_wttag" : false,
                "cmd_parallel" : 0,
                "time_sync" : 0
            }
        }
    });

static string protocol_config_2 = QUOTE({
        "protocol_stack" : {
            "name" : "iec104client",
            "version" : "1.0",
            "transport_layer" : {
                "redundancy_groups" : [
                    {
                        "connections" : [
                            {
                                "srv_ip" : "127.0.0.1",
                                "port" : 2404,
                                "conn": true,
                                "start": true
                            },
                            {
                                "srv_ip" : "127.0.0.2",
                                "clt_ip" : "127.0.0.3",
                                "port" : 2404,
                                "conn": false,
                                "start": false
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
                "asdu_size" : 0,
                "gi_time" : 60,
                "gi_cycle" : 30,
                "gi_all_ca" : false,
                "utc_time" : false,
                "cmd_wttag" : false,
                "cmd_parallel" : 0,
                "time_sync" : 0
            }
        }
    });

static string protocol_config_3 = QUOTE({
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
                "asdu_size" : 0,
                "gi_time" : 60,
                "gi_cycle" : 30,
                "gi_all_ca" : false,
                "utc_time" : false,
                "cmd_wttag" : false,
                "cmd_parallel" : 0,
                "time_sync" : 0
            }
        }
    });

static string protocol_config_4 = QUOTE({
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
                            }
                        ],
                        "rg_name" : "red-group1",
                        "tls" : true,
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
                "asdu_size" : 0,
                "gi_time" : 60,
                "gi_cycle" : 30,
                "gi_all_ca" : false,
                "utc_time" : false,
                "cmd_wttag" : false,
                "cmd_parallel" : 0,
                "time_sync" : 0
            }
        }
    });

static string protocol_config_5 = QUOTE({
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
                    },
                    {
                        "connections" : [
                            {
                                "srv_ip" : "127.0.0.2",
                                "port" : 2405
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
                "asdu_size" : 0,
                "gi_time" : 60,
                "gi_cycle" : 30,
                "gi_all_ca" : false,
                "utc_time" : false,
                "cmd_wttag" : false,
                "cmd_parallel" : 0,
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
        "tls_conf" : {
            "private_key" : "server-key.pem",
            "own_cert" : "server.cer",
            "ca_certs" : [
                {
                    "cert_file": "root.cer"
                }
            ]
        }
    });

static string tls_config_2 =  QUOTE({
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


static string tls_config_3 =  QUOTE({
        "tls_conf" : {
            "private_key" : "\\",
            "own_cert" : "\\",
            "ca_certs" : [
                {
                    "cert_file": "\\"
                }
            ],
            "remote_certs" : [
                {
                    "cert_file": "\\"
                }
            ]
        }
    });

static string tls_config_4 =  QUOTE({
        "tls_conf" : {
            "private_key" : "iec104_client.cer",
            "own_cert" : "iec104_client.key",
            "ca_certs" : [
                {
                    "cert_file": "iec104_ca.key"
                }
            ],
            "remote_certs" : [
                {
                    "cert_file": "iec104_server.key"
                }
            ]
        }
    });

static string tls_config_5 =  QUOTE({
        "tls_conf" : {
            "private_key" : "",
            "own_cert" : "iec104_client.key",
            "ca_certs" : [
                {
                    "cert_file": ""
                }
            ],
            "remote_certs" : [
                {
                    "cert_file": "iec104_server.key"
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

class ConnectionHandlingTest : public testing::Test
{
protected:

    IEC104TestComp* iec104 = nullptr;
    int ingestCallbackCalled = 0;
    Reading* storedReading = nullptr;
    int clockSyncHandlerCalled = 0;
    std::vector<Reading*> storedReadings;

    int asduHandlerCalled = 0;
    IMasterConnection lastConnection = nullptr;
    int lastOA = 0;
    int openConnections = 0;
    int activations = 0;
    int deactivations = 0;
    int maxConnections = 0;

    void SetUp()
    {
        iec104 = new IEC104TestComp();

        iec104->registerIngest(this, ingestCallback);
    }

    void TearDown()
    {
        iec104->stop();

        delete iec104;

        for (auto reading : storedReadings) {
            delete reading;
        }
    }

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
        ConnectionHandlingTest* self = (ConnectionHandlingTest*)parameter;

        printf("ingestCallback called -> asset: (%s)\n", reading.getAssetName().c_str());

        std::vector<Datapoint*> dataPoints = reading.getReadingData();

        for (Datapoint* sdp : dataPoints) {
            printf("name: %s value: %s\n", sdp->getName().c_str(), sdp->getData().toString().c_str());
        }
        self->storedReading = new Reading(reading);

        self->storedReadings.push_back(self->storedReading);

        self->ingestCallbackCalled++;
    }

    static bool clockSynchronizationHandler(void* parameter, IMasterConnection connection, CS101_ASDU asdu, CP56Time2a newTime)
    {
        ConnectionHandlingTest* self = (ConnectionHandlingTest*)parameter;

        self->clockSyncHandlerCalled++;

        return true;
    }

    static bool asduHandler (void* parameter, IMasterConnection connection, CS101_ASDU asdu)
    {
        ConnectionHandlingTest* self = (ConnectionHandlingTest*)parameter;

        auto typeId = CS101_ASDU_getTypeID(asdu);
        auto cot = CS101_ASDU_getCOT(asdu);
        printf("asduHandler: type: %s (%d) COT: %s (%d)\n", IEC104ClientConfig::getStringFromTypeID(typeId).c_str(), typeId,
                CS101_CauseOfTransmission_toString(cot), cot);

        self->lastConnection = NULL;
        self->lastOA = CS101_ASDU_getOA(asdu);

        int ca = CS101_ASDU_getCA(asdu);

        InformationObject io = CS101_ASDU_getElement(asdu, 0);

        if (io) {

            int ioa = InformationObject_getObjectAddress(io);

            if (typeId == C_SC_NA_1) {
                printf("  C_SC_NA_1 (single-command)\n");

                if (ca == 41025 && ioa == 2000) {
                    IMasterConnection_sendACT_CON(connection, asdu, false);
                    self->lastConnection = connection;
                }
            }
            else if (typeId == C_SC_TA_1) {
                printf("  C_SC_TA_1 (single-command w/timetag)\n");

                if (ca == 41025 && ioa == 2001) {
                    IMasterConnection_sendACT_CON(connection, asdu, false);
                    self->lastConnection = connection;
                }
            }
            else if (typeId == C_DC_NA_1) {
                printf("  C_DC_NA_1 (double-command)\n");

                if (ca == 41025 && ioa == 2002) {
                    IMasterConnection_sendACT_CON(connection, asdu, false);
                    self->lastConnection = connection;
                }
            }

            InformationObject_destroy(io);
        }

        if (typeId != C_IC_NA_1)
            self->asduHandlerCalled++;

        return true;
    }

    static void connectionEventHandler(void* parameter, IMasterConnection connection, CS104_PeerConnectionEvent event)
    {
        ConnectionHandlingTest* self = (ConnectionHandlingTest*)parameter;

        if (event == CS104_CON_EVENT_CONNECTION_OPENED) {
            self->openConnections++;
        }
        else if (event == CS104_CON_EVENT_CONNECTION_CLOSED) {
            self->openConnections--;
        }
        else if (event == CS104_CON_EVENT_ACTIVATED) {
            self->activations++;
        }
        else if (event == CS104_CON_EVENT_DEACTIVATED) {
            self->deactivations++;
        }

        if (self->openConnections > self->maxConnections)
            self->maxConnections = self->openConnections;
    }
};

TEST_F(ConnectionHandlingTest, TwoConnectionsSingleRedundancyGroup)
{
    openConnections = 0;
    activations = 0;
    deactivations = 0;

    asduHandlerCalled = 0;
    clockSyncHandlerCalled = 0;
    lastConnection = NULL;
    ingestCallbackCalled = 0;

    iec104->setJsonConfig(protocol_config, exchanged_data, tls_config);

    CS104_Slave slave = CS104_Slave_create(10, 10);
    ASSERT_NE(slave, nullptr);

    CS104_Slave_setLocalPort(slave, TEST_PORT);

    CS104_Slave_setClockSyncHandler(slave, clockSynchronizationHandler, this);
    CS104_Slave_setASDUHandler(slave, asduHandler, this);
    CS104_Slave_setConnectionEventHandler(slave, connectionEventHandler, this);

    CS104_Slave_start(slave);

    CS101_AppLayerParameters alParams = CS104_Slave_getAppLayerParameters(slave);

    ASSERT_EQ(0, openConnections);

    iec104->start();

    Thread_sleep(2000);

    ASSERT_EQ(2, openConnections);
    ASSERT_EQ(2, maxConnections);

    ASSERT_EQ(1, activations);
    ASSERT_EQ(0, deactivations);

    CS104_Slave_stop(slave);

    CS104_Slave_destroy(slave);
}

// TEST_F(ConnectionHandlingTest, TwoConnectionsDifferentPorts)
// {
//     openConnections = 0;
//     activations = 0;
//     deactivations = 0;

//     asduHandlerCalled = 0;
//     clockSyncHandlerCalled = 0;
//     lastConnection = NULL;
//     ingestCallbackCalled = 0;

//     iec104->setJsonConfig(protocol_config_5, exchanged_data, tls_config);

//     CS104_Slave slave1 = CS104_Slave_create(10, 10);
//     ASSERT_NE(slave, nullptr);

//     CS104_Slave_setLocalPort(slave1, TEST_PORT);

//     CS104_Slave_setClockSyncHandler(slave1, clockSynchronizationHandler, this);
//     CS104_Slave_setASDUHandler(slave1, asduHandler, this);
//     CS104_Slave_setConnectionEventHandler(slave1, connectionEventHandler, this);

//     CS104_Slave_start(slave1);

//     CS101_AppLayerParameters alParams1 = CS104_Slave_getAppLayerParameters(slave1);

//     CS104_Slave slave2 = CS104_Slave_create(10, 10);
//     ASSERT_NE(slave, nullptr);

//     CS104_Slave_setLocalPort(slave2, TEST_PORT+1);

//     CS104_Slave_setClockSyncHandler(slave2, clockSynchronizationHandler, this);
//     CS104_Slave_setASDUHandler(slave2, asduHandler, this);
//     CS104_Slave_setConnectionEventHandler(slave2, connectionEventHandler, this);

//     CS104_Slave_start(slave2);

//     CS101_AppLayerParameters alParams2 = CS104_Slave_getAppLayerParameters(slave2);

//     ASSERT_EQ(0, openConnections);

//     iec104->start();

//     Thread_sleep(1000);

//     ASSERT_EQ(2, openConnections);
//     ASSERT_EQ(2, maxConnections);

//     ASSERT_EQ(1, activations);
//     ASSERT_EQ(0, deactivations);

//     CS104_Slave_stop(slave1);

//     CS104_Slave_destroy(slave1);

//     CS104_Slave_stop(slave2);

//     CS104_Slave_destroy(slave2);
// }

TEST_F(ConnectionHandlingTest, TwoConnectionsOnlyOneConfiguredToConnect)
{
    openConnections = 0;
    activations = 0;
    deactivations = 0;

    asduHandlerCalled = 0;
    clockSyncHandlerCalled = 0;
    lastConnection = NULL;
    ingestCallbackCalled = 0;

    iec104->setJsonConfig(protocol_config_2, exchanged_data, tls_config);

    CS104_Slave slave = CS104_Slave_create(10, 10);
    ASSERT_NE(slave, nullptr);

    CS104_Slave_setLocalPort(slave, TEST_PORT);

    CS104_Slave_setClockSyncHandler(slave, clockSynchronizationHandler, this);
    CS104_Slave_setASDUHandler(slave, asduHandler, this);
    CS104_Slave_setConnectionEventHandler(slave, connectionEventHandler, this);

    CS104_Slave_start(slave);

    CS101_AppLayerParameters alParams = CS104_Slave_getAppLayerParameters(slave);

    ASSERT_EQ(0, openConnections);

    iec104->start();

    Thread_sleep(1000);

    ASSERT_EQ(1, openConnections);
    ASSERT_EQ(1, maxConnections);

    ASSERT_EQ(1, activations);
    ASSERT_EQ(0, deactivations);

    CS104_Slave_stop(slave);

    CS104_Slave_destroy(slave);
}

// Was broken
TEST_F(ConnectionHandlingTest, SingleConnectionTLS)
{
    openConnections = 0;
    activations = 0;
    deactivations = 0;

    asduHandlerCalled = 0;
    clockSyncHandlerCalled = 0;
    lastConnection = NULL;
    ingestCallbackCalled = 0;

    iec104->setJsonConfig(protocol_config_4, exchanged_data, tls_config_2);

    setenv("FLEDGE_DATA", "./data", 1);

    TLSConfiguration tlsConfig = TLSConfiguration_create();

    TLSConfiguration_addCACertificateFromFile(tlsConfig, "data/etc/certs/iec104_ca.cer");
    TLSConfiguration_setOwnCertificateFromFile(tlsConfig, "data/etc/certs/iec104_server.cer");
    TLSConfiguration_setOwnKeyFromFile(tlsConfig, "data/etc/certs/iec104_server.key", NULL);
    TLSConfiguration_addAllowedCertificateFromFile(tlsConfig, "data/etc/certs/iec104_client.cer");
    TLSConfiguration_setChainValidation(tlsConfig, true);
    TLSConfiguration_setAllowOnlyKnownCertificates(tlsConfig, true);

    CS104_Slave slave = CS104_Slave_createSecure(10, 10, tlsConfig);
    ASSERT_NE(slave, nullptr);

    CS104_Slave_setLocalPort(slave, TEST_PORT);

    CS104_Slave_setClockSyncHandler(slave, clockSynchronizationHandler, this);
    CS104_Slave_setASDUHandler(slave, asduHandler, this);
    CS104_Slave_setConnectionEventHandler(slave, connectionEventHandler, this);

    CS104_Slave_start(slave);

    CS101_AppLayerParameters alParams = CS104_Slave_getAppLayerParameters(slave);

    ASSERT_EQ(0, openConnections);

    iec104->start();

    Thread_sleep(2000);

    ASSERT_EQ(1, openConnections);
    ASSERT_EQ(1, maxConnections);

    ASSERT_EQ(1, activations);
    ASSERT_EQ(0, deactivations);

    CS104_Slave_stop(slave);

    CS104_Slave_destroy(slave);

    TLSConfiguration_destroy(tlsConfig);
}



