#include <gtest/gtest.h>

#include <config_category.h>
#include <plugin_api.h>

#include <utility>
#include <vector>
#include <string>

#include "cs104_slave.h"
#include <lib60870/hal_thread.h>

#include "iec104.h"

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
                "asdu_size" : 0,
                "gi_time" : 60,
                "gi_cycle" : 30,
                "gi_all_ca" : true,
                "utc_time" : false,
                "cmd_with_timetag" : false,
                "cmd_parallel" : 0,
                "time_sync" : 1
            },
            "south_monitoring" : {
                "asset": "CONSTAT-1",
                "cnx_loss_status_id" : "CNXLOSS-1"
            }
        }
    });


static string protocol_config_no_red = QUOTE({
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
                "gi_cycle" : 0,
                "gi_all_ca" : true,
                "utc_time" : false,
                "cmd_with_timetag" : false,
                "cmd_parallel" : 0,
                "time_sync" : 1
            },
            "south_monitoring" : {
                "asset": "CONSTAT-1",
                "cnx_loss_status_id" : "CNXLOSS-1"
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

static string exchanged_data_2 = QUOTE({
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
                    "label":"CNXLOSS-1",
                    "protocols":[
                       {
                          "name":"iec104",
                          "address":"41026-2001",
                          "typeid":"M_SP_NA_1"
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

class LegacyConnectionHandlingTest : public testing::Test
{
protected:

    void SetUp()
    {
        clockSyncHandlerCalled = 0;

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

        for (auto reading : storedLegacyReadings) {
            delete reading;
        }
    }

    void startIEC104() { iec104->start(); }

    IEC104TestComp* iec104;
    int ingestCallbackCalled = 0;
    int legacyIngestCallbackCalled = 0;
    Reading* storedReading = nullptr;
    Reading* storedLegacyReading = nullptr;
    int clockSyncHandlerCalled = 0;
    std::vector<Reading*> storedReadings;
    std::vector<Reading*> storedLegacyReadings;

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

    static bool IsConnxStatusStarted(Reading* reading)
    {
        Datapoint* southEvent = getObject(*reading, "south_event");

        if (southEvent) {
            Datapoint* connxStatus = getChild(*southEvent, "connx_status");

            if (connxStatus) {
                if (getStrValue(connxStatus) == "started") {
                    return true;
                }
            }
        }

        return false;
    }

    static bool IsGiStatusStarted(Reading* reading)
    {
        Datapoint* southEvent = getObject(*reading, "south_event");

        if (southEvent) {
            Datapoint* connxStatus = getChild(*southEvent, "gi_status");

            if (connxStatus) {
                if (getStrValue(connxStatus) == "started") {
                    return true;
                }
            }
        }

        return false;
    }

    static bool IsGiStatusFailed(Reading* reading)
    {
        Datapoint* southEvent = getObject(*reading, "south_event");

        if (southEvent) {
            Datapoint* connxStatus = getChild(*southEvent, "gi_status");

            if (connxStatus) {
                if (getStrValue(connxStatus) == "failed") {
                    return true;
                }
            }
        }

        return false;
    }

    static bool IsConnxStatusNotConnected(Reading* reading)
    {
        Datapoint* southEvent = getObject(*reading, "south_event");

        if (southEvent) {
            Datapoint* connxStatus = getChild(*southEvent, "connx_status");

            if (connxStatus) {
                if (getStrValue(connxStatus) == "not connected") {
                    return true;
                }
            }
        }

        return false;
    }

    static bool IsReadingWithQualityInvalid(Reading* reading)
    {
        Datapoint* dataObject = getObject(*reading, "data_object");

        if (dataObject) {
            Datapoint* do_quality_iv = getChild(*dataObject, "do_quality_iv");

            if (do_quality_iv) {
                if (getIntValue(do_quality_iv) == 1) {
                    return true;
                }
            }
        }

        return false;
    }

    static bool IsReadingWithQualityNonTopcial(Reading* reading)
    {
        Datapoint* dataObject = getObject(*reading, "data_object");

        if (dataObject) {
            Datapoint* do_quality_iv = getChild(*dataObject, "do_quality_nt");

            if (do_quality_iv) {
                if (getIntValue(do_quality_iv) == 1) {
                    return true;
                }
            }
        }

        return false;
    }

    static void ingestCallback(void* parameter, Reading reading)
    {
        if(reading.getAssetName() != "CONSTAT-1"){
            LegacyConnectionHandlingTest* self = (LegacyConnectionHandlingTest*)parameter;

            printf("ingestCallback called -> asset: (%s)\n", reading.getAssetName().c_str());

            std::vector<Datapoint*> dataPoints = reading.getReadingData();

            for (Datapoint* sdp : dataPoints) {
                printf("name: %s value: %s\n", sdp->getName().c_str(), sdp->getData().toString().c_str());
            }

            self->storedReading = new Reading(reading);

            self->storedReadings.push_back(self->storedReading);

            self->ingestCallbackCalled++;
        }
        else{
            LegacyConnectionHandlingTest* self = (LegacyConnectionHandlingTest*)parameter;

            printf("legacyIngestCallback called -> asset: (%s)\n", reading.getAssetName().c_str());

            std::vector<Datapoint*> dataPoints = reading.getReadingData();

            for (Datapoint* sdp : dataPoints) {
                printf("name: %s value: %s\n", sdp->getName().c_str(), sdp->getData().toString().c_str());
            }

            self->storedLegacyReading = new Reading(reading);

            self->storedLegacyReadings.push_back(self->storedLegacyReading);

            self->legacyIngestCallbackCalled++;
        }
    }

    
    bool
    containsString(vector<string> array, string str){
        return std::find(array.begin(),array.end(),str) != array.end();
    }


    bool 
    containSouthEventsInRightOrder(vector<Reading*> readings, vector<string> expected_unique_events){
        vector<string> unique_events;

        for(Reading* reading : readings){
            Datapoint* south_event = reading->getReadingData().at(0);
            string south_event_value = south_event->getData().toString();

            if(!containsString(unique_events,south_event_value)){
                unique_events.push_back(south_event_value);
            }    
        }

        return unique_events == expected_unique_events;
    }

};


TEST_F(LegacyConnectionHandlingTest, ConnectionLost)
{
    ingestCallbackCalled = 0;

    iec104->setJsonConfig(protocol_config_no_red, exchanged_data, tls_config);

    CS104_Slave slave = CS104_Slave_create(10, 10);

    CS104_Slave_setLocalPort(slave, TEST_PORT);

    printf("Starting CS 104 slave\n");

    CS104_Slave_start(slave);

    printf("Slave started\n");

    startIEC104();

    printf("Client plugin started\n");

    Thread_sleep(500);

    CS104_Slave_stop(slave);

    Thread_sleep(2000);

    CS104_Slave_destroy(slave);

    Thread_sleep(1000);

    ASSERT_EQ(8, ingestCallbackCalled);

    ASSERT_EQ(8, storedReadings.size());

    vector<string> expected_unique_events;

    expected_unique_events.push_back("{\"connx_status\":\"started\"}");

    expected_unique_events.push_back("{\"gi_status\":\"started\"}");

    expected_unique_events.push_back("{\"gi_status\":\"failed\"}");

    expected_unique_events.push_back("{\"connx_status\":\"not connected\"}");

    ASSERT_TRUE(containSouthEventsInRightOrder(storedLegacyReadings,expected_unique_events));

    int i = 0;

    ASSERT_TRUE(IsReadingWithQualityInvalid(storedReadings[i]));
    ASSERT_FALSE(IsReadingWithQualityNonTopcial(storedReadings[i++]));

    ASSERT_TRUE(IsReadingWithQualityInvalid(storedReadings[i]));
    ASSERT_FALSE(IsReadingWithQualityNonTopcial(storedReadings[i++]));

    ASSERT_TRUE(IsReadingWithQualityInvalid(storedReadings[i]));
    ASSERT_FALSE(IsReadingWithQualityNonTopcial(storedReadings[i++]));

    ASSERT_TRUE(IsReadingWithQualityInvalid(storedReadings[i]));
    ASSERT_FALSE(IsReadingWithQualityNonTopcial(storedReadings[i++]));

    ASSERT_TRUE(IsReadingWithQualityNonTopcial(storedReadings[i++]));
    ASSERT_TRUE(IsReadingWithQualityNonTopcial(storedReadings[i++]));
    ASSERT_TRUE(IsReadingWithQualityNonTopcial(storedReadings[i++]));
    ASSERT_TRUE(IsReadingWithQualityNonTopcial(storedReadings[i++]));
}

TEST_F(LegacyConnectionHandlingTest, ConnectionLostReconnect)
{
    ingestCallbackCalled = 0;

    iec104->setJsonConfig(protocol_config_no_red, exchanged_data, tls_config);

    CS104_Slave slave = CS104_Slave_create(10, 10);

    CS104_Slave_setLocalPort(slave, TEST_PORT);

    CS104_Slave_start(slave);
    
    
    startIEC104();
    
    Thread_sleep(500);
    
    CS104_Slave_stop(slave);
  
    Thread_sleep(2000);

    CS104_Slave_start(slave);

    Thread_sleep(12000); /* wait more than 10s - default reconnect delay */

    CS104_Slave_destroy(slave);

    vector<string> expected_unique_events;

    expected_unique_events.push_back("{\"connx_status\":\"started\"}");

    expected_unique_events.push_back("{\"gi_status\":\"started\"}");

    expected_unique_events.push_back("{\"gi_status\":\"failed\"}");

    expected_unique_events.push_back("{\"connx_status\":\"not connected\"}");

    ASSERT_TRUE(containSouthEventsInRightOrder(storedLegacyReadings,expected_unique_events));
    
    ASSERT_TRUE(IsReadingWithQualityInvalid(storedReadings[0]));
    ASSERT_TRUE(IsReadingWithQualityInvalid(storedReadings[1]));
    ASSERT_TRUE(IsReadingWithQualityInvalid(storedReadings[2]));
    ASSERT_TRUE(IsReadingWithQualityInvalid(storedReadings[3]));

    ASSERT_TRUE(IsReadingWithQualityNonTopcial(storedReadings[4]));
    ASSERT_TRUE(IsReadingWithQualityNonTopcial(storedReadings[5]));
    ASSERT_TRUE(IsReadingWithQualityNonTopcial(storedReadings[6]));
    ASSERT_TRUE(IsReadingWithQualityNonTopcial(storedReadings[7]));
}

TEST_F(LegacyConnectionHandlingTest, SendConnectionStatusAfterRequestFromNorth)
{
    ingestCallbackCalled = 0;

    iec104->setJsonConfig(protocol_config_no_red, exchanged_data, tls_config);

    startIEC104();

    Thread_sleep(500);

    bool operationResult = iec104->operation("request_connection_status", 0, nullptr);

    ASSERT_EQ(4, ingestCallbackCalled);

    ASSERT_EQ(4, storedReadings.size());

    vector<string> expected_unique_events;

    expected_unique_events.push_back("{\"connx_status\":\"not connected\", \"gi_status\":\"idle\"}");

    ASSERT_TRUE(containSouthEventsInRightOrder(storedLegacyReadings,expected_unique_events));

    ASSERT_TRUE(IsReadingWithQualityInvalid(storedReadings[0]));
    ASSERT_TRUE(IsReadingWithQualityInvalid(storedReadings[1]));
    ASSERT_TRUE(IsReadingWithQualityInvalid(storedReadings[2]));
    ASSERT_TRUE(IsReadingWithQualityInvalid(storedReadings[3]));

}

TEST_F(LegacyConnectionHandlingTest, ConnectionLostStatus)
{
    ingestCallbackCalled = 0;

    iec104->setJsonConfig(protocol_config_no_red, exchanged_data_2, tls_config);

    CS104_Slave slave = CS104_Slave_create(10, 10);

    CS104_Slave_setLocalPort(slave, TEST_PORT);

    printf("Starting CS 104 slave\n");

    CS104_Slave_start(slave);

    printf("Slave started\n");

    startIEC104();

    printf("Client plugin started\n");

    Thread_sleep(500);

    CS104_Slave_stop(slave);

    Thread_sleep(2000);

    CS104_Slave_destroy(slave);

    vector<string> expected_unique_events;

    expected_unique_events.push_back("{\"connx_status\":\"started\"}");

    expected_unique_events.push_back("{\"gi_status\":\"started\"}");

    expected_unique_events.push_back("{\"gi_status\":\"failed\"}");

    expected_unique_events.push_back("{\"connx_status\":\"not connected\"}");

    ASSERT_TRUE(containSouthEventsInRightOrder(storedLegacyReadings,expected_unique_events));

    ASSERT_EQ(7, ingestCallbackCalled);

    ASSERT_EQ(7, storedReadings.size());

}