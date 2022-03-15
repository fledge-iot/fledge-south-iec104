#include <gtest/gtest.h>
#include <iec104.h>
#include <plugin_api.h>
#include <string.h>

using namespace std;
using namespace nlohmann;

struct jc
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

    string exchanged_data =
        "{\"description\":\"exchangeddatalist\",\"type\":\"string\","
        "\"displayName\":\"Exchangeddatalist\",\"order\":\"3\",\"default\":{"
        "\"exchanged_data\":{\"name\":\"iec104client\",\"version\":\"1.0\","
        "\"asdu_list\":[{\"ca\":41025,\"type_id\":\"M_ME_NA_1\",\"label\":\"TM-"
        "1\",\"ioa\":4202832},{\"ca\":41025,\"type_id\":\"M_ME_NA_1\","
        "\"label\":"
        "\"TM-2\",\"ioa\":4202852},{\"ca\":41025,\"type_id\":\"M_SP_TB_1\","
        "\"label\":\"TS-1\",\"ioa\":4206948}]}}}";

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

    string tls =
        "{\"description\":\"tlsparameters\",\"type\":\"string\","
        "\"displayName\":"
        "\"TLSparameters\",\"order\":\"5\",\"default\":{\"tls_conf:\":{"
        "\"private_"
        "key\":\"server-key.pem\",\"server_cert\":\"server.cer\",\"ca_cert\":"
        "\"root.cer\"}}}";
};

/*TEST(IEC104, PluginStartNoThrow)
{
    IEC104 iec104;
    struct jc a;

    iec104.setJsonConfig(a.protocol_stack, a.exchanged_data,
                         a.protocol_translation, a.tls);
    ASSERT_NO_THROW(iec104.start());
}*/
