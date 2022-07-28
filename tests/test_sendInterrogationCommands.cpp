#include <gtest/gtest.h>
#include <iec104.h>

#include <string>

using namespace std;
using namespace nlohmann;
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

// TEST(IEC104, PluginsendInterrogationCommmandsTest)
// {
//     IEC104 iec104;
//     json_config a;

//     iec104.setJsonConfig(a.protocol_stack, a.exchanged_data,
//                          a.protocol_translation, a.tls);
//     ASSERT_NO_THROW(iec104.sendInterrogationCommmands());
// }