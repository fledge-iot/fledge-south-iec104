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

struct json_config
{
    string protocol_stack =
        "{\"description\":\"protocolstackparameters\",\"type\":\"string\","
        "\"displayName\":\"Protocolstackparameters\",\"order\":\"2\","
        "\"default\":"
        "{\"protocol_stack\":{\"name\":\"iec104client\",\"version\":\"1.0\","
        "\"transport_layer\":{\"connection\":{\"path\":[{\"srv_ip\":\"127.0.0."
        "1\",\"clt_ip\":\"\",\"port\":2404},{\"srv_ip\":\"127.0.0.1\",\"clt_"
        "ip\":"
        "\"\",\"port\":2404}],\"tls\":false},\"llevel\":1,\"k_value\":12,\"w_"
        "value\":8,\"t0_timeout\":10,\"t1_timeout\":15,\"t2_timeout\":10,\"t3_"
        "timeout\":20,\"conn_all\":true,\"start_all\":false,\"conn_passv\":"
        "false}"
        ",\"application_layer\":{\"orig_addr\":0,\"ca_asdu_size\":2,\"ioaddr_"
        "size\":3,\"startup_time\":180,\"asdu_size\":0,\"gi_time\":60,\"gi_"
        "cycle\":false,\"gi_all_ca\":false,\"gi_repeat_count\":2,\"disc_qual\":"
        "\"NT\",\"send_iv_time\":0,\"tsiv\":\"REMOVE\",\"utc_time\":false,"
        "\"comm_"
        "wttag\":false,\"comm_parallel\":0,\"exec_cycl_test\":false,\"startup_"
        "state\":true,\"reverse\":false,\"time_sync\":false}}}}";

    string exchanged_data = QUOTE({
        "exchanged_data" : {
            "name" : "iec104client",
            "version" : "1.0",
            "asdu_list" : [ {
                "ca" : 41025,
                "type_id" : "M_ME_NB_1",
                "label" : "TM-1",
                "ioa" : 4202832
            } ]
        }
    });

    string protocol_translation =
        "{\"description\":\"protocoltranslationmapping\",\"type\":"
        "\"string\","
        "\"displayName\":\"Protocoltranslationmapping\",\"order\":\"4\","
        "\"default\":{\"protocol_translation\":{\"name\":\"iec104_to_"
        "pivot\","
        "\"version\":\"1.0\",\"mapping\":{\"data_object_header\":{\"doh_"
        "type\":"
        "\"type_id\",\"doh_ca\":\"ca\",\"doh_oa\":\"oa\",\"doh_cot\":"
        "\"cot\","
        "\"doh_test\":\"istest\",\"doh_negative\":\"isnegative\"},\"data_"
        "object_"
        "item\":{\"doi_ioa\":\"ioa\",\"doi_value\":\"value\",\"doi_"
        "quality\":"
        "\"quality_desc\",\"doi_ts\":\"time_marker\",\"doi_ts_flag1\":"
        "\"isinvalid\",\"doi_ts_flag2\":\"isSummerTime\",\"doi_ts_flag3\":"
        "\"isSubstituted\"}}}}}";

    string tls =
        "{\"description\":\"tlsparameters\",\"type\":\"string\","
        "\"displayName\":"
        "\"TLSparameters\",\"order\":\"5\",\"default\":{\"tls_conf:\":{"
        "\"private_"
        "key\":\"server-key.pem\",\"server_cert\":\"server.cer\",\"ca_cert\":"
        "\"root.cer\"}}}";
};

class MockIEC104Client : public IEC104Client
{
public:
    MockIEC104Client(IEC104* iec104, nlohmann::json* pivot_configuration)
        : IEC104Client(iec104, pivot_configuration)
    {
    }
    MOCK_METHOD(void, addData,
                (std::vector<Datapoint*> & datapoints, int64_t ioa,
                 const std::string& dataname, const int64_t value,
                 QualityDescriptor qd, CP56Time2a ts));
    MOCK_METHOD(void, addData,
                (std::vector<Datapoint*> & datapoints, int64_t ioa,
                 const std::string& dataname, const float value,
                 QualityDescriptor qd, CP56Time2a ts));
};

void ingest_cb(void* data, Reading reading) {}

class AsduHandlerTest : public testing::Test
{
protected:
    void SetUp() override
    {
        iec104 = new IEC104();
        iec104->registerIngest(NULL, ingest_cb);
    }

    void TearDown() override {}

    IEC104* iec104;
};

TEST_F(AsduHandlerTest, AsduReceivedHandlerNominal)
{
    json_config config;

    json m_pivot_configuration = json::parse(config.protocol_translation);

    iec104->setJsonConfig(config.protocol_stack, config.exchanged_data,
                          config.protocol_translation, config.tls);

    MockIEC104Client m_client(iec104, &m_pivot_configuration);

    CS104_Slave slave = CS104_Slave_create(100, 100);
    CS101_AppLayerParameters alParams =
        CS104_Slave_getAppLayerParameters(slave);
    CS101_ASDU asdu = CS101_ASDU_create(alParams, false, CS101_COT_INITIALIZED,
                                        0, 1, false, false);

    CS101_ASDU_setCA(asdu, 41025);
    CS101_ASDU_setTypeID(asdu, M_ME_NB_1);

    InformationObject io = (InformationObject)MeasuredValueScaled_create(
        NULL, 4202832, 100, IEC60870_QUALITY_GOOD);

    CS101_ASDU_addInformationObject(asdu, io);

    // vector<Datapoint*> datapoints;
    // EXPECT_CALL(m_client,
    //             addData(datapoints, (int64_t)4202832, (std::string) "TM_1",
    //                     testing::Matcher<int64_t>(100),
    //                     IEC60870_QUALITY_GOOD, nullptr));

    ASSERT_TRUE(iec104->m_asduReceivedHandlerP(&m_client, 0, asdu));

    InformationObject_destroy(io);
    CS101_ASDU_destroy(asdu);
}
