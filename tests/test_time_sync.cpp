#include <config_category.h>
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
                "gi_enabled": false,
                "gi_time" : 60,
                "gi_cycle" : 0,
                "gi_all_ca" : true,
                "utc_time" : false,
                "cmd_with_timetag" : false,
                "cmd_parallel" : 0,
                "time_sync" : 1
            }
        }
    });


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
                    "label":"TM-B-1",
                    "protocols":[
                       {
                          "name":"iec104",
                          "address":"41026-2001",
                          "typeid":"M_ME_NA_1"
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

class TimeSyncTest : public testing::Test
{
protected:

    void SetUp()
    {
        clockSyncHandlerCalled = 0;

        iec104 = new IEC104TestComp();

        iec104->registerIngest(NULL, ingestCallback);
    }

    void TearDown()
    {
        iec104->stop();

        delete iec104;
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
        storedReading = new Reading(reading);

        ingestCallbackCalled++;
    }

    static bool clockSynchronizationHandler(void* parameter, IMasterConnection connection, CS101_ASDU asdu, CP56Time2a newTime)
    {
        TimeSyncTest* self = (TimeSyncTest*)parameter;

        char addrBuf[100];

        IMasterConnection_getPeerAddress(connection, addrBuf, 100);

        uint64_t ts = CP56Time2a_toMsTimestamp(newTime);

        printf("Clock sync called froom %s: %lu\n", addrBuf, ts);

        self->clockSyncHandlerCalled++;

        return true;
    }

    static bool clockSynchronizationHandlerNegative(void* parameter, IMasterConnection connection, CS101_ASDU asdu, CP56Time2a newTime)
    {
        TimeSyncTest* self = (TimeSyncTest*)parameter;

        char addrBuf[100];

        IMasterConnection_getPeerAddress(connection, addrBuf, 100);

        printf("Clock sync called from %s\n", addrBuf);

        self->clockSyncHandlerCalled++;

        return false;
    }

    IEC104TestComp* iec104;
    static int ingestCallbackCalled;
    static Reading* storedReading;
    int clockSyncHandlerCalled = 0;
};

int TimeSyncTest::ingestCallbackCalled;
Reading* TimeSyncTest::storedReading;

TEST_F(TimeSyncTest, IEC104Client_checkPeriodicTimesync)
{
    iec104->setJsonConfig(protocol_config, exchanged_data, tls_config);

    CS104_Slave slave = CS104_Slave_create(10, 10);

    CS104_Slave_setLocalPort(slave, TEST_PORT);

    CS104_Slave_setClockSyncHandler(slave, clockSynchronizationHandler, this);

    CS104_Slave_start(slave);

    CS101_AppLayerParameters alParams = CS104_Slave_getAppLayerParameters(slave);

    startIEC104();

    Thread_sleep(800);

    ASSERT_EQ(1, clockSyncHandlerCalled);

    Thread_sleep(2000);

    ASSERT_EQ(3, clockSyncHandlerCalled);

    CS104_Slave_stop(slave);

    CS104_Slave_destroy(slave);
}

TEST_F(TimeSyncTest, IEC104Client_checkPeriodicTimesyncDenied)
{
    Datapoint* dp = nullptr;

    dp->parseJson("test");

    iec104->setJsonConfig(protocol_config, exchanged_data, tls_config);

    CS104_Slave slave = CS104_Slave_create(10, 10);

    CS104_Slave_setLocalPort(slave, TEST_PORT);

    CS104_Slave_setClockSyncHandler(slave, clockSynchronizationHandlerNegative, this);

    CS104_Slave_start(slave);

    CS101_AppLayerParameters alParams = CS104_Slave_getAppLayerParameters(slave);

    startIEC104();

    Thread_sleep(800);

    ASSERT_EQ(1, clockSyncHandlerCalled);

    Thread_sleep(2000);

    ASSERT_EQ(1, clockSyncHandlerCalled);

    CS104_Slave_stop(slave);

    CS104_Slave_destroy(slave);
}