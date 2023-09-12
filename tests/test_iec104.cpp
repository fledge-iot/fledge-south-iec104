#include <config_category.h>
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
                "gi_time" : 60,  
                "gi_cycle" : 30,                
                "gi_all_ca" : true,                           
                "utc_time" : false,                
                "cmd_with_timetag" : false,              
                "cmd_parallel" : 0,                            
                "time_sync" : 100                 
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
                    "label":"TM-3",
                    "protocols":[
                       {
                          "name":"iec104",
                          "address":"41025-4202853",
                          "typeid":"M_ST_NA_1"
                       }
                    ]
                },
                {
                    "label":"TM-4",
                    "protocols":[
                       {
                          "name":"iec104",
                          "address":"41025-4202854",
                          "typeid":"M_ST_TB_1"
                       }
                    ]
                },
                {
                    "label":"TM-5",
                    "protocols":[
                       {
                          "name":"iec104",
                          "address":"41025-4202855",
                          "typeid":"M_ME_TD_1"
                       }
                    ]
                },
                {
                    "label":"TM-6",
                    "protocols":[
                       {
                          "name":"iec104",
                          "address":"41025-4202856",
                          "typeid":"M_ME_TE_1"
                       }
                    ]
                },
                {
                    "label":"TM-7",
                    "protocols":[
                       {
                          "name":"iec104",
                          "address":"41025-4202857",
                          "typeid":"M_ME_TF_1"
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

class IEC104Test : public testing::Test
{
protected:

    IEC104TestComp* iec104;

    void SetUp()
    {
        ingestCallbackCalled = 0;
        ingestedSpontOrPeriodic = 0;
        ingestedInterrogated = 0;

        iec104 = new IEC104TestComp();
        iec104->setJsonConfig(protocol_config, exchanged_data, tls_config);

        iec104->registerIngest(this, ingestCallback);
    }

    void TearDown()
    {
        iec104->stop();

        for (auto reading : storedReadings) {
            delete reading;
        }
       
        delete iec104;
    }

    void startIEC104() { iec104->start(); }

    int ingestCallbackCalled = 0;
    int ingestedSpontOrPeriodic = 0;
    int ingestedInterrogated = 0;

    Reading* storedReading = nullptr;
    int clockSyncHandlerCalled = 0;
    std::vector<Reading*> storedReadings;
    std::vector<Reading*> storedReadingsInterrogated;
    std::vector<Reading*> storedReadingsSpontOrPeriodic;

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
        IEC104Test* self = (IEC104Test*)parameter;

        std::vector<Datapoint*> dataPoints = reading.getReadingData();

        if (hasObject(reading, "data_object")) {
            Datapoint* data_object = getObject(reading, "data_object");

            self->storedReading = new Reading(reading);

            if (self->storedReading) {
                if (hasChild(*data_object, "do_cot")) {
                    int cot = getIntValue(getChild(*data_object, "do_cot"));

                    if (cot == CS101_COT_SPONTANEOUS || cot == CS101_COT_PERIODIC) {
                        self->storedReadingsSpontOrPeriodic.push_back(self->storedReading);
                        self->ingestedSpontOrPeriodic++;
                    }
                    else if (cot == CS101_COT_INTERROGATED_BY_STATION) {
                        self->storedReadingsInterrogated.push_back(self->storedReading);
                        self->ingestedInterrogated++;
                    }
                }

                self->storedReadings.push_back(self->storedReading);
            }
        }
        else {
            printf("Unexpected reading type\n");
        }

        self->ingestCallbackCalled++;
    }
};

/* Callback handler that is called when an interrogation command is received */
static bool
interrogationHandler(void* parameter, IMasterConnection connection, CS101_ASDU asdu, uint8_t qoi)
{
    if (qoi == 20) { /* only handle station interrogation */

        CS101_AppLayerParameters alParams = IMasterConnection_getApplicationLayerParameters(connection);

        IMasterConnection_sendACT_CON(connection, asdu, false);

        /* The CS101 specification only allows information objects without timestamp in GI responses */

        CS101_ASDU newAsdu = CS101_ASDU_create(alParams, false, CS101_COT_INTERROGATED_BY_STATION,
                0, 41025, false, false);

        struct sCP56Time2a ts;

        uint64_t timestamp = Hal_getTimeInMs();
            
        CP56Time2a_createFromMsTimestamp(&ts, timestamp);

        InformationObject io = (InformationObject)SinglePointWithCP56Time2a_create(NULL, 4206948, true, IEC60870_QUALITY_GOOD, &ts);

        CS101_ASDU_addInformationObject(newAsdu, io);

        IMasterConnection_sendASDU(connection, newAsdu);

        CS101_ASDU_destroy(newAsdu);

        IMasterConnection_sendACT_TERM(connection, asdu);
    }
    else {
        IMasterConnection_sendACT_CON(connection, asdu, true);
    }

    return true;
}

TEST_F(IEC104Test, IEC104_receiveMonitoringAsdus)
{
    ingestCallbackCalled = 0;
    ingestedSpontOrPeriodic = 0;
    ingestedInterrogated = 0;
    storedReading = nullptr;

    CS104_Slave slave = CS104_Slave_create(10, 10);

    CS104_Slave_setInterrogationHandler(slave, interrogationHandler, NULL);

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

    ASSERT_EQ(ingestedSpontOrPeriodic, 9);
    Reading* reading = storedReadingsSpontOrPeriodic[8];
    ASSERT_TRUE(reading != nullptr);
    ASSERT_EQ("TS-1", reading->getAssetName());
    ASSERT_TRUE(hasObject(*reading, "data_object"));
    Datapoint* data_object = getObject(*reading, "data_object");
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
    ASSERT_EQ((int64_t) 1, getIntValue(getChild(*data_object, "do_cot"))); /* CS101_COT_PERIODIC(1) */
    ASSERT_EQ((int64_t) 4206948, getIntValue(getChild(*data_object, "do_ioa")));
    ASSERT_EQ((int64_t) timestamp, getIntValue(getChild(*data_object, "do_ts")));

    CS101_ASDU newAsdu2 = CS101_ASDU_create(alParams, false, CS101_COT_INTERROGATED_BY_STATION, 0, 41025, false, false);

    io = (InformationObject) SinglePointInformation_create(NULL, 4206948, true, IEC60870_QUALITY_GOOD);

    CS101_ASDU_addInformationObject(newAsdu2, io);

    InformationObject_destroy(io);

    /* Add ASDU to slave event queue */
    CS104_Slave_enqueueASDU(slave, newAsdu2);

    CS101_ASDU_destroy(newAsdu2);

    CS101_ASDU newAsdu3 = CS101_ASDU_create(alParams, false, CS101_COT_SPONTANEOUS, 0, 41025, false, false);

    io = (InformationObject) SinglePointInformation_create(NULL, 4206948, true, IEC60870_QUALITY_INVALID | IEC60870_QUALITY_NON_TOPICAL);

    CS101_ASDU_addInformationObject(newAsdu3, io);

    InformationObject_destroy(io);

    /* Add ASDU to slave event queue */
    CS104_Slave_enqueueASDU(slave, newAsdu3);

    CS101_ASDU_destroy(newAsdu3);

    Thread_sleep(500);

    ASSERT_EQ(ingestedSpontOrPeriodic, 10);

    reading = storedReadingsSpontOrPeriodic[9];
    ASSERT_TRUE(reading != nullptr);
    data_object = getObject(*reading, "data_object");
    ASSERT_EQ(1, getIntValue(getChild(*data_object, "do_quality_iv")));
    ASSERT_EQ(0, getIntValue(getChild(*data_object, "do_quality_bl")));
    ASSERT_EQ(0, getIntValue(getChild(*data_object, "do_quality_sb")));
    ASSERT_EQ(1, getIntValue(getChild(*data_object, "do_quality_nt")));

    CS104_Slave_stop(slave);

    CS104_Slave_destroy(slave);
}

static void test_ConnectionEventHandler (void* parameter, IMasterConnection connection, CS104_PeerConnectionEvent event)
{
    int* connectEventCounter = (int*)parameter;

    char addrBuf[100];

    IMasterConnection_getPeerAddress(connection, addrBuf, 100);

    printf("event: %i (from %s)\n", event, addrBuf);

    if (event == CS104_CON_EVENT_CONNECTION_OPENED) {
        *connectEventCounter = *connectEventCounter + 1;
    }
}

TEST_F(IEC104Test, IEC104_reconnectAfterConnectionLoss)
{
    int connectionEventCounter = 0;

    /* start the slave */
    CS104_Slave slave = CS104_Slave_create(10, 10);
    
    CS104_Slave_setConnectionEventHandler(slave, test_ConnectionEventHandler, &connectionEventCounter);

    CS104_Slave_setLocalPort(slave, TEST_PORT);

    CS104_Slave_start(slave);

    CS101_AppLayerParameters alParams = CS104_Slave_getAppLayerParameters(slave);

    /* start the client/plugin */
    startIEC104();

    /* wait for the client to connect */
    Thread_sleep(2000);

    /* Check that the client is connected */
    ASSERT_EQ(1, connectionEventCounter);

    /* stop the slave to simulate a connection loss */
    CS104_Slave_stop(slave);
    CS104_Slave_destroy(slave);

    Thread_sleep(500);

    /* start a new slave */
    slave = CS104_Slave_create(10, 10);
    
    CS104_Slave_setConnectionEventHandler(slave, test_ConnectionEventHandler, &connectionEventCounter);

    CS104_Slave_setLocalPort(slave, TEST_PORT);

    CS104_Slave_start(slave);

    /* wait at least 10 s for the client to reconnect */
    Thread_sleep(12000);

    /* check if the client connected a second time */
    ASSERT_EQ(2, connectionEventCounter);

    CS104_Slave_destroy(slave);
}

TEST_F(IEC104Test, IEC104_connectionFails)
{
    int connectionEventCounter = 0;

    CS104_Slave slave = CS104_Slave_create(10, 10);
    
    CS104_Slave_setConnectionEventHandler(slave, test_ConnectionEventHandler, &connectionEventCounter);

    CS104_Slave_setLocalPort(slave, TEST_PORT);

    CS101_AppLayerParameters alParams = CS104_Slave_getAppLayerParameters(slave);

    startIEC104();

    Thread_sleep(8000);

    ASSERT_EQ(0, connectionEventCounter);

    CS104_Slave_start(slave);

    Thread_sleep(4000);

    ASSERT_EQ(1, connectionEventCounter);

    CS104_Slave_destroy(slave);
}

TEST_F(IEC104Test, IEC104_setJsonConfig_Test)
{
    ASSERT_NO_THROW(
        iec104->setJsonConfig(protocol_config, exchanged_data, tls_config));
}

TEST_F(IEC104Test, IEC104_receiveMonitoringAsdusWithCOT_11)
{
    ingestCallbackCalled = 0;
    storedReading = nullptr;

    CS104_Slave slave = CS104_Slave_create(10, 10);

    CS104_Slave_setLocalPort(slave, TEST_PORT);

    CS104_Slave_start(slave);

    CS101_AppLayerParameters alParams = CS104_Slave_getAppLayerParameters(slave);

    startIEC104();

    CS101_ASDU newAsdu = CS101_ASDU_create(alParams, false, CS101_COT_RETURN_INFO_REMOTE, 0, 41025, false, false);

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

    ASSERT_EQ(ingestCallbackCalled, 9);
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
    ASSERT_EQ((int64_t) CS101_COT_RETURN_INFO_REMOTE, getIntValue(getChild(*data_object, "do_cot")));
    ASSERT_EQ((int64_t) 4206948, getIntValue(getChild(*data_object, "do_ioa")));
    ASSERT_EQ((int64_t) timestamp, getIntValue(getChild(*data_object, "do_ts")));

    CS104_Slave_stop(slave);

    CS104_Slave_destroy(slave);
}

TEST_F(IEC104Test, IEC104_receiveSpont_M_ST_TB_1)
{
    ingestCallbackCalled = 0;
    storedReading = nullptr;

    CS104_Slave slave = CS104_Slave_create(10, 10);

    CS104_Slave_setLocalPort(slave, TEST_PORT);

    CS104_Slave_start(slave);

    CS101_AppLayerParameters alParams = CS104_Slave_getAppLayerParameters(slave);

    startIEC104();

    CS101_ASDU newAsdu = CS101_ASDU_create(alParams, false, CS101_COT_SPONTANEOUS, 0, 41025, false, false);

    struct sCP56Time2a ts;

    uint64_t timestamp = Hal_getTimeInMs();
            
    CP56Time2a_createFromMsTimestamp(&ts, timestamp);

    InformationObject io = (InformationObject) StepPositionWithCP56Time2a_create(NULL, 4202854, 1, true, IEC60870_QUALITY_GOOD, &ts);

    CS101_ASDU_addInformationObject(newAsdu, io);

    InformationObject_destroy(io);

    /* Add ASDU to slave event queue */
    CS104_Slave_enqueueASDU(slave, newAsdu);

    CS101_ASDU_destroy(newAsdu);

    Thread_sleep(500);

    ASSERT_EQ(ingestCallbackCalled, 9);
    ASSERT_EQ("TM-4", storedReading->getAssetName());
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

    ASSERT_EQ("M_ST_TB_1", getStrValue(getChild(*data_object, "do_type")));
    ASSERT_EQ((int64_t) 41025, getIntValue(getChild(*data_object, "do_ca")));
    ASSERT_EQ((int64_t) CS101_COT_SPONTANEOUS, getIntValue(getChild(*data_object, "do_cot")));
    ASSERT_EQ((int64_t) 4202854, getIntValue(getChild(*data_object, "do_ioa")));
    ASSERT_EQ((int64_t) timestamp, getIntValue(getChild(*data_object, "do_ts")));

    CS104_Slave_stop(slave);

    CS104_Slave_destroy(slave);
}

TEST_F(IEC104Test, IEC104_receiveSpont_M_ME_TD_1)
{
    ingestCallbackCalled = 0;
    storedReading = nullptr;

    CS104_Slave slave = CS104_Slave_create(10, 10);

    CS104_Slave_setLocalPort(slave, TEST_PORT);

    CS104_Slave_start(slave);

    CS101_AppLayerParameters alParams = CS104_Slave_getAppLayerParameters(slave);

    startIEC104();

    CS101_ASDU newAsdu = CS101_ASDU_create(alParams, false, CS101_COT_SPONTANEOUS, 0, 41025, false, false);

    struct sCP56Time2a ts;

    uint64_t timestamp = Hal_getTimeInMs();
            
    CP56Time2a_createFromMsTimestamp(&ts, timestamp);

    InformationObject io = (InformationObject) MeasuredValueNormalizedWithCP56Time2a_create(NULL, 4202855, 0.5, IEC60870_QUALITY_GOOD, &ts);

    CS101_ASDU_addInformationObject(newAsdu, io);

    InformationObject_destroy(io);

    /* Add ASDU to slave event queue */
    CS104_Slave_enqueueASDU(slave, newAsdu);

    CS101_ASDU_destroy(newAsdu);

    Thread_sleep(500);

    ASSERT_EQ(ingestCallbackCalled, 9);
    ASSERT_EQ("TM-5", storedReading->getAssetName());
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

    ASSERT_EQ("M_ME_TD_1", getStrValue(getChild(*data_object, "do_type")));
    ASSERT_EQ((int64_t) 41025, getIntValue(getChild(*data_object, "do_ca")));
    ASSERT_EQ((int64_t) CS101_COT_SPONTANEOUS, getIntValue(getChild(*data_object, "do_cot")));
    ASSERT_EQ((int64_t) 4202855, getIntValue(getChild(*data_object, "do_ioa")));
    ASSERT_EQ((int64_t) timestamp, getIntValue(getChild(*data_object, "do_ts")));

    CS104_Slave_stop(slave);

    CS104_Slave_destroy(slave);
}

TEST_F(IEC104Test, IEC104_receiveSpont_M_ME_TE_1)
{
    ingestCallbackCalled = 0;
    storedReading = nullptr;

    CS104_Slave slave = CS104_Slave_create(10, 10);

    CS104_Slave_setLocalPort(slave, TEST_PORT);

    CS104_Slave_start(slave);

    CS101_AppLayerParameters alParams = CS104_Slave_getAppLayerParameters(slave);

    startIEC104();

    CS101_ASDU newAsdu = CS101_ASDU_create(alParams, false, CS101_COT_SPONTANEOUS, 0, 41025, false, false);

    struct sCP56Time2a ts;

    uint64_t timestamp = Hal_getTimeInMs();
            
    CP56Time2a_createFromMsTimestamp(&ts, timestamp);

    InformationObject io = (InformationObject) MeasuredValueScaledWithCP56Time2a_create(NULL, 4202856, 5, IEC60870_QUALITY_GOOD, &ts);

    CS101_ASDU_addInformationObject(newAsdu, io);

    InformationObject_destroy(io);

    /* Add ASDU to slave event queue */
    CS104_Slave_enqueueASDU(slave, newAsdu);

    CS101_ASDU_destroy(newAsdu);

    Thread_sleep(500);

    ASSERT_EQ(ingestCallbackCalled, 9);
    ASSERT_EQ("TM-6", storedReading->getAssetName());
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

    ASSERT_EQ("M_ME_TE_1", getStrValue(getChild(*data_object, "do_type")));
    ASSERT_EQ((int64_t) 41025, getIntValue(getChild(*data_object, "do_ca")));
    ASSERT_EQ((int64_t) CS101_COT_SPONTANEOUS, getIntValue(getChild(*data_object, "do_cot")));
    ASSERT_EQ((int64_t) 4202856, getIntValue(getChild(*data_object, "do_ioa")));
    ASSERT_EQ((int64_t) timestamp, getIntValue(getChild(*data_object, "do_ts")));

    CS104_Slave_stop(slave);

    CS104_Slave_destroy(slave);
}

TEST_F(IEC104Test, IEC104_receiveSpont_M_ME_TF_1)
{
    ingestCallbackCalled = 0;
    storedReading = nullptr;

    CS104_Slave slave = CS104_Slave_create(10, 10);

    CS104_Slave_setLocalPort(slave, TEST_PORT);

    CS104_Slave_start(slave);

    CS101_AppLayerParameters alParams = CS104_Slave_getAppLayerParameters(slave);

    startIEC104();

    CS101_ASDU newAsdu = CS101_ASDU_create(alParams, false, CS101_COT_SPONTANEOUS, 0, 41025, false, false);

    struct sCP56Time2a ts;

    uint64_t timestamp = Hal_getTimeInMs();
            
    CP56Time2a_createFromMsTimestamp(&ts, timestamp);

    InformationObject io = (InformationObject) MeasuredValueShortWithCP56Time2a_create(NULL, 4202857, 50.5, IEC60870_QUALITY_GOOD, &ts);

    CS101_ASDU_addInformationObject(newAsdu, io);

    InformationObject_destroy(io);

    /* Add ASDU to slave event queue */
    CS104_Slave_enqueueASDU(slave, newAsdu);

    CS101_ASDU_destroy(newAsdu);

    Thread_sleep(500);

    ASSERT_EQ(ingestCallbackCalled, 9);
    ASSERT_EQ("TM-7", storedReading->getAssetName());
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

    ASSERT_EQ("M_ME_TF_1", getStrValue(getChild(*data_object, "do_type")));
    ASSERT_EQ((int64_t) 41025, getIntValue(getChild(*data_object, "do_ca")));
    ASSERT_EQ((int64_t) CS101_COT_SPONTANEOUS, getIntValue(getChild(*data_object, "do_cot")));
    ASSERT_EQ((int64_t) 4202857, getIntValue(getChild(*data_object, "do_ioa")));
    ASSERT_EQ((int64_t) timestamp, getIntValue(getChild(*data_object, "do_ts")));

    CS104_Slave_stop(slave);

    CS104_Slave_destroy(slave);
}

TEST_F(IEC104Test, IEC104_receiveGI_M_ST_NA_1)
{
    ingestCallbackCalled = 0;
    storedReading = nullptr;

    CS104_Slave slave = CS104_Slave_create(10, 10);

    CS104_Slave_setLocalPort(slave, TEST_PORT);

    CS104_Slave_start(slave);

    CS101_AppLayerParameters alParams = CS104_Slave_getAppLayerParameters(slave);

    startIEC104();

    CS101_ASDU newAsdu = CS101_ASDU_create(alParams, false, CS101_COT_INTERROGATED_BY_STATION, 0, 41025, false, false);


    InformationObject io = (InformationObject) StepPositionInformation_create(NULL, 4202853, 1, true, IEC60870_QUALITY_GOOD);

    CS101_ASDU_addInformationObject(newAsdu, io);

    InformationObject_destroy(io);

    /* Add ASDU to slave event queue */
    CS104_Slave_enqueueASDU(slave, newAsdu);

    CS101_ASDU_destroy(newAsdu);

    Thread_sleep(500);

    ASSERT_EQ(ingestCallbackCalled, 9);
    ASSERT_EQ("TM-3", storedReading->getAssetName());
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
    ASSERT_FALSE(hasChild(*data_object, "do_ts"));
    ASSERT_FALSE(hasChild(*data_object, "do_ts_iv"));
    ASSERT_FALSE(hasChild(*data_object, "do_ts_su"));
    ASSERT_FALSE(hasChild(*data_object, "do_ts_sub"));

    ASSERT_EQ("M_ST_NA_1", getStrValue(getChild(*data_object, "do_type")));
    ASSERT_EQ((int64_t) 41025, getIntValue(getChild(*data_object, "do_ca")));
    ASSERT_EQ((int64_t) CS101_COT_INTERROGATED_BY_STATION, getIntValue(getChild(*data_object, "do_cot")));
    ASSERT_EQ((int64_t) 4202853, getIntValue(getChild(*data_object, "do_ioa")));

    CS104_Slave_stop(slave);

    CS104_Slave_destroy(slave);
}