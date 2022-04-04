#include <config_category.h>
#include <cs104_slave.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <iec104.h>
#include <plugin_api.h>
#include <string.h>

#include <utility>

using ::testing::NiceMock;
using namespace std;
using namespace nlohmann;

// Define configuration, important part is the exchanged_data
// It contains all asdu to take into account
struct json_config
{
    string tls = "{\"description\":\"tlsparameters\"}";
    string protocol_stack = "{\"description\":\"protocolstackparameters\"}";

    string protocol_translation = QUOTE({
        "exchanged_data" : {
            "name" : "iec104client",
            "version" : "1.0",
            "mapping" : {
                "data_object_header" : {
                    "doh_type" : "type_id",
                    "doh_ca" : "ca",
                    "doh_oa" : "oa",
                    "doh_cot" : "cot",
                    "doh_test" : "istest",
                    "doh_negative" : "isnegative"
                },
                "data_object_item" : {
                    "doi_ioa" : "ioa",
                    "doi_value" : "value",
                    "doi_quality" : "quality_desc",
                    "doi_ts" : "time_marker",
                    "doi_ts_flag1" : "isinvalid",
                    "doi_ts_flag2" : "isSummerTime",
                    "doi_ts_flag3" : "isSubstituted"
                }
            }
        }
    });

    string exchanged_data = QUOTE({
        "exchanged_data" : {
            "name" : "iec104client",
            "version" : "1.0",
            "asdu_list" : [
                {
                    "ca" : 41025,
                    "type_id" : "M_ME_NB_1",
                    "label" : "TM-1",
                    "ioa" : 4202832
                },
                {
                    "ca" : 41025,
                    "type_id" : "M_SP_NA_1",
                    "label" : "TM-1",
                    "ioa" : 4202832
                },
                {
                    "ca" : 41025,
                    "type_id" : "M_SP_TB_1",
                    "label" : "TM-1",
                    "ioa" : 4202832
                },
                {
                    "ca" : 41025,
                    "type_id" : "M_DP_NA_1",
                    "label" : "TM-1",
                    "ioa" : 4202832
                },
                {
                    "ca" : 41025,
                    "type_id" : "M_DP_TB_1",
                    "label" : "TM-1",
                    "ioa" : 4202832
                },
                {
                    "ca" : 41025,
                    "type_id" : "M_ST_NA_1",
                    "label" : "TM-1",
                    "ioa" : 4202832
                },
                {
                    "ca" : 41025,
                    "type_id" : "M_ST_TB_1",
                    "label" : "TM-1",
                    "ioa" : 4202832
                },
                {
                    "ca" : 41025,
                    "type_id" : "M_ME_NA_1",
                    "label" : "TM-1",
                    "ioa" : 4202832
                },
                {
                    "ca" : 41025,
                    "type_id" : "M_ME_TD_1",
                    "label" : "TM-1",
                    "ioa" : 4202832
                },
                {
                    "ca" : 41025,
                    "type_id" : "M_ME_NC_1",
                    "label" : "TM-1",
                    "ioa" : 4202832
                },
                {
                    "ca" : 41025,
                    "type_id" : "M_ME_TE_1",
                    "label" : "TM-1",
                    "ioa" : 4202832
                },
                {
                    "ca" : 41025,
                    "type_id" : "M_ME_TF_1",
                    "label" : "TM-1",
                    "ioa" : 4202832
                },
                {
                    "ca" : 41025,
                    "type_id" : "M_EI_NA_1",
                    "label" : "TM-1",
                    "ioa" : 4202832
                },
                {
                    "ca" : 41025,
                    "type_id" : "C_IC_NA_1",
                    "label" : "TM-1",
                    "ioa" : 4202832
                },
                {
                    "ca" : 41025,
                    "type_id" : "C_TS_TA_1",
                    "label" : "TM-1",
                    "ioa" : 4202832
                },
                {
                    "ca" : 41025,
                    "type_id" : "C_SC_TA_1",
                    "label" : "TM-1",
                    "ioa" : 4202832
                },
                {
                    "ca" : 41025,
                    "type_id" : "C_DC_TA_1",
                    "label" : "TM-1",
                    "ioa" : 4202832
                }
            ]
        }
    });
};

// Dummy fonction to prevent code execution in iec104::ingest during test
void ingest_cb(void* data, Reading reading) {}

// Class to be called in each test, contains fixture to be used in
class AsduHandlerTest : public testing::Test
{
protected:
    CS101_ASDU asdu;       // ASDU used during each test, will contain IO
    IEC104* iec104;        // Object on which we call for tests
    json_config config;    // Will contain JSON defined above
    InformationObject io;  // Contain information object for asdu
    json m_pivot_configuration;
    struct sCP56Time2a testTimestamp;
    IEC104Client* m_client;

    // Setup is ran for every tests, so each variable are reinitialised
    void SetUp() override
    {
        // Get only the mapping part
        m_pivot_configuration =
        json::parse(config.protocol_translation)["exchanged_data"];

        // Init iec object with dummy ingest callback and json configuration
        iec104 = new IEC104();
        iec104->setTsiv("PROCESS");
        iec104->setCommWttag(false);
        iec104->registerIngest(NULL, ingest_cb);
        iec104->setJsonConfig(config.protocol_stack, config.exchanged_data,
                              config.protocol_translation, config.tls);

        // Init used parameters to define create ASDU
        CS104_Slave slave = CS104_Slave_create(100, 100);
        CS101_AppLayerParameters alParams =
            CS104_Slave_getAppLayerParameters(slave);

        // Get current timestamp (used in tests)
        CP56Time2a_createFromMsTimestamp(&testTimestamp, Hal_getTimeInMs());

        m_client = new IEC104Client(iec104, &m_pivot_configuration);

        // Default base ASDU
        asdu = CS101_ASDU_create(alParams, false, CS101_COT_INITIALIZED, 0, 1,
                                 false, false);

        // Set CA, in relation with what is in the description
        CS101_ASDU_setCA(asdu, 41025);

        // Default IO for destroy purpose
        io = (InformationObject)MeasuredValueScaled_create(
            NULL, 4202832, 100, IEC60870_QUALITY_GOOD);
    }

    // TearDown is ran for every tests, so each variable are destroyed again
    void TearDown() override
    {
        CS101_ASDU_destroy(asdu);
        InformationObject_destroy(io);
    }
};

// Test the default case of m_asduReceivedHandler() in iec104
// Should return false
TEST_F(AsduHandlerTest, AsduReceivedHandlerDefault)
{
    // Unknown/not used type
    CS101_ASDU_setTypeID(asdu, F_SC_NB_1);
    ASSERT_FALSE(iec104->m_asduReceivedHandlerP(m_client, 0, asdu));
}

// For all the following
// Tests all cases where we handle the type in m_asduReceivedHandler() in iec104
// Should return true
TEST_F(AsduHandlerTest, AsduReceivedHandlerM_ME_NB_1)
{
    // M_ME_NB_1
    io = (InformationObject)MeasuredValueScaled_create(NULL, 4202832, 100,
                                                       IEC60870_QUALITY_GOOD);
    ASSERT_TRUE(CS101_ASDU_addInformationObject(asdu, io));
    ASSERT_TRUE(iec104->m_asduReceivedHandlerP(m_client, 0, asdu));
}

TEST_F(AsduHandlerTest, AsduReceivedHandlerM_SP_NA_1)
{
    // M_SP_NA_1
    io = (InformationObject)SinglePointInformation_create(
        NULL, 4202832, true, IEC60870_QUALITY_GOOD);
    ASSERT_TRUE(CS101_ASDU_addInformationObject(asdu, io));
    ASSERT_TRUE(iec104->m_asduReceivedHandlerP(m_client, 0, asdu));
}

TEST_F(AsduHandlerTest, AsduReceivedHandlerM_SP_TB_1)
{
    // M_SP_TB_1
    io = (InformationObject)SinglePointWithCP56Time2a_create(
        NULL, 4202832, true, IEC60870_QUALITY_GOOD, &testTimestamp);
    ASSERT_TRUE(CS101_ASDU_addInformationObject(asdu, io));
    ASSERT_TRUE(iec104->m_asduReceivedHandlerP(m_client, 0, asdu));

    // Test other part of the handle callback
    iec104->setCommWttag(true);
    ASSERT_TRUE(iec104->m_asduReceivedHandlerP(m_client, 0, asdu));
}

TEST_F(AsduHandlerTest, AsduReceivedHandlerM_DP_NA_1)
{
    // M_DP_NA_1
    io = (InformationObject)DoublePointInformation_create(
        NULL, 4202832, IEC60870_DOUBLE_POINT_ON, IEC60870_QUALITY_GOOD);
    ASSERT_TRUE(CS101_ASDU_addInformationObject(asdu, io));
    ASSERT_TRUE(iec104->m_asduReceivedHandlerP(m_client, 0, asdu));
}

TEST_F(AsduHandlerTest, AsduReceivedHandlerM_DP_TB_1)
{
    // M_DP_TB_1
    io = (InformationObject)DoublePointWithCP56Time2a_create(
        NULL, 4202832, IEC60870_DOUBLE_POINT_ON, IEC60870_QUALITY_GOOD,
        &testTimestamp);
    ASSERT_TRUE(CS101_ASDU_addInformationObject(asdu, io));
    ASSERT_TRUE(iec104->m_asduReceivedHandlerP(m_client, 0, asdu));

    // Test other part of the handle callback
    iec104->setCommWttag(true);
    ASSERT_TRUE(iec104->m_asduReceivedHandlerP(m_client, 0, asdu));
}

TEST_F(AsduHandlerTest, AsduReceivedHandlerM_ST_NA_1)
{
    // M_ST_NA_1
    io = (InformationObject)StepPositionInformation_create(
        NULL, 4202832, 10, true, IEC60870_QUALITY_GOOD);
    ASSERT_TRUE(CS101_ASDU_addInformationObject(asdu, io));
    ASSERT_TRUE(iec104->m_asduReceivedHandlerP(m_client, 0, asdu));
}

TEST_F(AsduHandlerTest, AsduReceivedHandlerM_ST_TB_1)
{
    // M_ST_TB_1
    io = (InformationObject)StepPositionWithCP56Time2a_create(
        NULL, 4202832, 10, true, IEC60870_QUALITY_GOOD, &testTimestamp);
    ASSERT_TRUE(CS101_ASDU_addInformationObject(asdu, io));
    ASSERT_TRUE(iec104->m_asduReceivedHandlerP(m_client, 0, asdu));

    // Test other part of the handle callback
    iec104->setCommWttag(true);
    ASSERT_TRUE(iec104->m_asduReceivedHandlerP(m_client, 0, asdu));
}

TEST_F(AsduHandlerTest, AsduReceivedHandlerM_ME_NA_1)
{
    // M_ME_NA_1
    io = (InformationObject)MeasuredValueNormalized_create(
        NULL, 4202832, 10.0, IEC60870_QUALITY_GOOD);
    ASSERT_TRUE(CS101_ASDU_addInformationObject(asdu, io));
    ASSERT_TRUE(iec104->m_asduReceivedHandlerP(m_client, 0, asdu));
}

TEST_F(AsduHandlerTest, AsduReceivedHandlerM_ME_TD_1)
{
    // M_ME_TD_1
    io = (InformationObject)MeasuredValueNormalizedWithCP56Time2a_create(
        NULL, 4202832, 10.0, IEC60870_QUALITY_GOOD, &testTimestamp);
    ASSERT_TRUE(CS101_ASDU_addInformationObject(asdu, io));
    ASSERT_TRUE(iec104->m_asduReceivedHandlerP(m_client, 0, asdu));

    // Test other part of the handle callback
    iec104->setCommWttag(true);
    ASSERT_TRUE(iec104->m_asduReceivedHandlerP(m_client, 0, asdu));
}

TEST_F(AsduHandlerTest, AsduReceivedHandlerM_ME_TE_1)
{
    // M_ME_TE_1
    io = (InformationObject)MeasuredValueScaledWithCP56Time2a_create(
        NULL, 4202832, 10, IEC60870_QUALITY_GOOD, &testTimestamp);
    ASSERT_TRUE(CS101_ASDU_addInformationObject(asdu, io));
    ASSERT_TRUE(iec104->m_asduReceivedHandlerP(m_client, 0, asdu));

    // Test other part of the handle callback
    iec104->setCommWttag(true);
    ASSERT_TRUE(iec104->m_asduReceivedHandlerP(m_client, 0, asdu));
}

TEST_F(AsduHandlerTest, AsduReceivedHandlerM_ME_NC_1)
{
    // M_ME_NC_1
    io = (InformationObject)MeasuredValueShort_create(NULL, 4202832, 10.0,
                                                      IEC60870_QUALITY_GOOD);
    ASSERT_TRUE(CS101_ASDU_addInformationObject(asdu, io));
    ASSERT_TRUE(iec104->m_asduReceivedHandlerP(m_client, 0, asdu));
}

TEST_F(AsduHandlerTest, AsduReceivedHandlerM_ME_TF_1)
{
    // M_ME_TF_1
    io = (InformationObject)MeasuredValueShortWithCP56Time2a_create(
        NULL, 4202832, 10.0, IEC60870_QUALITY_GOOD, &testTimestamp);
    ASSERT_TRUE(CS101_ASDU_addInformationObject(asdu, io));
    ASSERT_TRUE(iec104->m_asduReceivedHandlerP(m_client, 0, asdu));

    // Test other part of the handle callback
    iec104->setCommWttag(true);
    ASSERT_TRUE(iec104->m_asduReceivedHandlerP(m_client, 0, asdu));
}

TEST_F(AsduHandlerTest, AsduReceivedHandlerM_EI_NA_1)
{
    // M_EI_NA_1
    CS101_ASDU_setTypeID(asdu, M_EI_NA_1);
    ASSERT_TRUE(iec104->m_asduReceivedHandlerP(m_client, 0, asdu));
}

TEST_F(AsduHandlerTest, AsduReceivedHandlerC_IC_NA_1)
{
    // C_IC_NA_1
    CS101_ASDU_setTypeID(asdu, C_IC_NA_1);
    ASSERT_TRUE(iec104->m_asduReceivedHandlerP(m_client, 0, asdu));
}

TEST_F(AsduHandlerTest, AsduReceivedHandlerC_TS_TA_1)
{
    // C_TS_TA_1
    CS101_ASDU_setTypeID(asdu, C_TS_TA_1);
    ASSERT_TRUE(iec104->m_asduReceivedHandlerP(m_client, 0, asdu));
}

TEST_F(AsduHandlerTest, AsduReceivedHandlerC_SC_TA_1)
{
    // C_SC_TA_1
    CS101_ASDU_setTypeID(asdu, C_SC_TA_1);
    ASSERT_TRUE(iec104->m_asduReceivedHandlerP(m_client, 0, asdu));
}

TEST_F(AsduHandlerTest, AsduReceivedHandlerC_DC_TA_1)
{
    // C_DC_TA_1
    CS101_ASDU_setTypeID(asdu, C_DC_TA_1);
    ASSERT_TRUE(iec104->m_asduReceivedHandlerP(m_client, 0, asdu));
}