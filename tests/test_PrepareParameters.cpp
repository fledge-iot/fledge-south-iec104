#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <iec104.h>
#include <plugin_api.h>
#include <string.h>

#include <iostream>
#include <string>

using namespace std;
using namespace nlohmann;

#define PROTOCOL_STACK_DEF
/*
Ordre :
protocol_stack
exchanged_data
protocol_translation
tls

*/
typedef struct
{
    string protocol_stack = QUOTE({
        "protocol_stack" : {
            "name" : "iec104client",
            "version" : "1.0",
            "transport_layer" : {
                "connection" : {
                    "path" : [
                        {"srv_ip" : "127.0.0.1", "clt_ip" : "", "port" : 2404},
                        {"srv_ip" : "127.0.0.1", "clt_ip" : "", "port" : 2404}
                    ],
                    "tls" : false
                },
                "llevel" : 1,
                "k_value" : 12,
                "w_value" : 8,
                "t0_timeout" : 10,
                "t1_timeout" : 15,
                "t2_timeout" : 10,
                "t3_timeout" : 20,
                "conn_all" : true,
                "start_all" : false,
                "conn_passv" : false
            },
            "application_layer" : {
                "orig_addr" : 0,
                "ca_asdu_size" : 2,
                "ioaddr_size" : 3,
                "startup_time" : 180,
                "asdu_size" : 0,
                "gi_time" : 60,
                "gi_cycle" : false,
                "gi_all_ca" : false,
                "gi_repeat_count" : 2,
                "disc_qual" : "NT",
                "send_iv_time" : 0,
                "tsiv" : "REMOVE",
                "utc_time" : false,
                "comm_wttag" : false,
                "comm_parallel" : 0,
                "exec_cycl_test" : false,
                "startup_state" : true,
                "reverse" : false,
                "time_sync" : false
            }
        }
    });
    string protocol_translation = QUOTE({
        "protocol_translation" : {
            "name" : "iec104_to_pivot",
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
    string tls = QUOTE({
        "tls_conf:" : {
            "private_key" : "server-key.pem",
            "server_cert" : "server.cer",
            "ca_cert" : "root.cer"
        }
    });
    string exchanged_data = QUOTE({
        "exchanged_data" : {
            "name" : "iec104client",
            "version" : "1.0",
            "asdu_list" : [
                {
                    "ca" : 41025,
                    "type_id" : "M_ME_NA_1",
                    "label" : "TM-1",
                    "ioa" : 4202832
                },
                {
                    "ca" : 41025,
                    "type_id" : "M_ME_NA_1",
                    "label" : "TM-2",
                    "ioa" : 4202852
                },
                {
                    "ca" : 41025,
                    "type_id" : "M_SP_TB_1",
                    "label" : "TS-1",
                    "ioa" : 4206948
                }
            ]
        }
    });
} json_config;

/**
 * Tests whether ASDU size value is 249 when value in json is 0.
 * Also test whther ASDU size has a value different fron 249 when value in JSON
 * is different from 0.
 */
TEST(IEC104, PluginASDUSizeTest)
{
    IEC104 iec104;
    json_config config;
    iec104.setJsonConfig(config.protocol_stack, config.exchanged_data,
                         config.protocol_translation, config.tls);
    const int PORT = 2404;
    string ip_adress("127.0.0.1");
    CS104_Connection connection =
        CS104_Connection_create(ip_adress.c_str(), PORT);

    iec104.prepareParameters(connection);
    CS101_AppLayerParameters params =
        CS104_Connection_getAppLayerParameters(connection);

    ASSERT_EQ(params->maxSizeOfASDU, 249);
    // ASSERT_NE(params->maxSizeOfASDU, 249);
}

/**
 * @brief Test parameters are correctly parsed from JSON and fill the APCI
 * parameters structures.
 */
TEST(IEC104, PluginAPCIParametersTestsNoThrow)
{
    IEC104 iec104;
    json_config config;
    iec104.setJsonConfig(config.protocol_stack, config.exchanged_data,
                         config.protocol_translation, config.tls);
    const int PORT = 2404;
    string ip_adress("127.0.0.1");
    CS104_Connection connection =
        CS104_Connection_create(ip_adress.c_str(), PORT);

    iec104.prepareParameters(connection);
    CS104_APCIParameters params =
        CS104_Connection_getAPCIParameters(connection);

    ASSERT_EQ(params->k, 12);
    ASSERT_EQ(params->w, 8);
    ASSERT_EQ(params->t0, 10);
    ASSERT_EQ(params->t1, 15);
    ASSERT_EQ(params->t2, 10);
    ASSERT_EQ(params->t3, 20);
}

/**
 * @brief Test parameters are correctly parsed from JSON and fill the Applayer
 * parameters structures.
 */

TEST(IEC104, PluginAppLayerParametersTestsNoThrow)
{
    IEC104 iec104;
    json_config config;
    iec104.setJsonConfig(config.protocol_stack, config.exchanged_data,
                         config.protocol_translation, config.tls);
    const int PORT = 2404;
    string ip_adress("127.0.0.1");
    CS104_Connection connection =
        CS104_Connection_create(ip_adress.c_str(), PORT);

    iec104.prepareParameters(connection);
    CS101_AppLayerParameters params =
        CS104_Connection_getAppLayerParameters(connection);

    ASSERT_EQ(params->originatorAddress, 0);
    ASSERT_EQ(params->sizeOfCA, 2);
    ASSERT_EQ(params->sizeOfIOA, 3);
}
