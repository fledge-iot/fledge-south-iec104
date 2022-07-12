#include <config_category.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <iec104.h>
#include <plugin_api.h>
#include <string.h>

#include <boost/thread.hpp>
#include <utility>
#include <vector>

using namespace std;
using namespace nlohmann;

#include "conf_init.h"
struct json_config
{
    string protocol_stack = PROTOCOL_STACK_DEF;

    string exchanged_data = EXCHANGED_DATA_DEF;

    string tls = TLS_DEF;

    string protocol_stack_info = PROTOCOL_STACK_DEF_INFO;

    string protocol_stack_warning = PROTOCOL_STACK_DEF_WARNING;

    string protocol_stack_debug = PROTOCOL_STACK_DEF_DEBUG;
};

TEST(IEC104, JsonConfig_DEF)
{
    json_config config;
    IEC104* iec104 = new IEC104();

    ASSERT_NO_THROW(
        iec104->setJsonConfig(config.protocol_stack, config.exchanged_data, config.tls));

    ASSERT_NO_THROW(
        iec104->setJsonConfig(config.protocol_stack_info, config.exchanged_data, config.tls));

    ASSERT_NO_THROW(iec104->setJsonConfig(
        config.protocol_stack_warning, config.exchanged_data, config.tls));

    ASSERT_NO_THROW(iec104->setJsonConfig(
        config.protocol_stack_debug, config.exchanged_data, config.tls));

    delete iec104;
    iec104 = nullptr;
}

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

    string tls =
        "{\"description\":\"tlsparameters\",\"type\":\"string\","
        "\"displayName\":"
        "\"TLSparameters\",\"order\":\"5\",\"default\":{\"tls_conf:\":{"
        "\"private_"
        "key\":\"server-key.pem\",\"server_cert\":\"server.cer\",\"ca_cert\":"
        "\"root.cer\"}}}";
};

TEST(IEC104, PluginConfigNoThrow)
{
    jc a;
    IEC104* iec104 = new IEC104();

    ASSERT_NO_THROW(iec104->setJsonConfig(a.protocol_stack, a.exchanged_data, a.tls));

    delete iec104;
    iec104 = nullptr;
}

TEST(IEC104, PluginConfigThrowTLS)
{
    jc a;
    // empty string
    a.tls = "}";

    IEC104* iec104 = new IEC104();

    ASSERT_THROW(iec104->setJsonConfig(a.protocol_stack, a.exchanged_data, a.tls),
                 string);

    delete iec104;
    iec104 = nullptr;
}

TEST(IEC104, PluginConfigThrowExchData)
{
    jc a;
    // " forgotten after description
    a.exchanged_data =
        "{\"description:\"exchangeddatalist\",\"type\":\"string\","
        "\"displayName\":\"Exchangeddatalist\",\"order\":\"3\",\"default\":{"
        "\"exchanged_data\":{\"name\":\"iec104client\",\"version\":\"1.0\","
        "\"asdu_list\":[{\"ca\":41025,\"type_id\":\"M_ME_NA_1\",\"label\":\"TM-"
        "1\",\"ioa\":4202832},{\"ca\":41025,\"type_id\":\"M_ME_NA_1\","
        "\"label\":"
        "\"TM-2\",\"ioa\":4202852},{\"ca\":41025,\"type_id\":\"M_SP_TB_1\","
        "\"label\":\"TS-1\",\"ioa\":4206948}]}}}";

    IEC104* iec104 = new IEC104();

    ASSERT_THROW(iec104->setJsonConfig(a.protocol_stack, a.exchanged_data, a.tls),
                 string);

    delete iec104;
    iec104 = nullptr;
}

TEST(IEC104, PluginConfigThrowProtStack)
{
    jc a;
    // } forgotten at the end
    a.protocol_stack =
        "{\"description\":\"tlsparameters\",\"type\":\"string\","
        "\"displayName\":"
        "\"TLSparameters\",\"order\":\"5\",\"default\":{\"tls_conf:\":{"
        "\"private_"
        "key\":\"server-key.pem\",\"server_cert\":\"server.cer\",\"ca_cert\":"
        "\"root.cer\"}}";

    IEC104* iec104 = new IEC104();

    ASSERT_THROW(iec104->setJsonConfig(a.protocol_stack, a.exchanged_data, a.tls),
                 string);

    delete iec104;
    iec104 = nullptr;
}