#include <config_category.h>
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
    string protocol_translation =
        "{\"description\":\"protocoltranslationmapping\",\"type\":\"string\","
        "\"displayName\":\"Protocoltranslationmapping\",\"order\":\"4\","
        "\"default\":{\"protocol_translation\":{\"name\":\"iec104_to_pivot\","
        "\"version\":\"1.0\",\"mapping\":{\"data_object_header\":{\"doh_type\":"
        "\"type_id\",\"doh_ca\":\"ca\",\"doh_oa\":\"oa\",\"doh_cot\":\"cot\","
        "\"doh_test\":\"istest\",\"doh_negative\":\"isnegative\"},\"data_"
        "object_"
        "item\":{\"doi_ioa\":\"ioa\",\"doi_value\":\"value\",\"doi_quality\":"
        "\"quality_desc\",\"doi_ts\":\"time_marker\",\"doi_ts_flag1\":"
        "\"isinvalid\",\"doi_ts_flag2\":\"isSummerTime\",\"doi_ts_flag3\":"
        "\"isSubstituted\"}}}}}";
};

class MockIEC104Client : public IEC104Client
{
public:
    MOCK_METHOD(void, addData,
                (std::vector<Datapoint*> & datapoints, int64_t ioa,
                 const std::string& dataname, const int64_t value,
                 QualityDescriptor qd, CP56Time2a ts));
    MOCK_METHOD(void, addData,
                (std::vector<Datapoint*> & datapoints, int64_t ioa,
                 const std::string& dataname, const float value,
                 QualityDescriptor qd, CP56Time2a ts));
};

TEST(IEC104, AsduReceivedHandler)
{
    json_config config;
    json m_pivot_configuration = json::parse(config.protocol_translation);

    IEC104* iec104 = new IEC104();

    // Fais une erreur de reference sur iec104
    // NiceMock<MockIEC104Client> m_client(iec104, &m_pivot_configuration);

    CS101_ASDU asdu;
    // Potentiel option pour modifier l'asdu (marche pas pour le moment)
    // CS101_ASDU_setCOT(*asdu, CS101_COT_PERIODIC);
    // CS101_ASDU_setCA(*asdu, 0);
    // CS101_ASDU_setTypeID

    // a mocker car externe si on arrive pas a avoir les bon params pour ASDU
    // CS101_ASDU_getCA
    // CS101_ASDU_getTypeID <- doit renvoyer "M_ME_NB_1" par exemple
    // handleASDU <- pour verifier qu'on l'appel

    // la fonction a tester en priorite 1
    ASSERT_FALSE(iec104->m_asduReceivedHandlerP(NULL, 0, asdu));
}
