#ifndef CONF_INIT_HPP
#define CONF_INIT_HPP
#include <string>

// PLUGIN DEFAULT PROTOCOL STACK CONF
#define PROTOCOL_STACK_DEF                                                     \
    QUOTE({                                                                    \
        "protocol_stack" : {                                                   \
            "name" : "iec104client",                                           \
            "version" : "1.0",                                                 \
            "transport_layer" : {                                              \
                "connection" : {                                               \
                    "path" : [                                                 \
                        {                                                      \
                            "srv_ip" : "127.0.0.1",                            \
                            "clt_ip" : "",                                     \
                            "port" : 2404                                      \
                        },                                                     \
                        {"srv_ip" : "127.0.0.1", "clt_ip" : "", "port" : 2404} \
                    ],                                                         \
                    "tls" : false                                              \
                },                                                             \
                "k_value" : 12,                                                \
                "w_value" : 8,                                                 \
                "t0_timeout" : 10,                                             \
                "t1_timeout" : 15,                                             \
                "t2_timeout" : 10,                                             \
                "t3_timeout" : 20,                                             \
                "conn_all" : true,                                             \
                "start_all" : false,                                           \
                "conn_passv" : false                                           \
            },                                                                 \
            "application_layer" : {                                            \
                "orig_addr" : 0,                                               \
                "ca_asdu_size" : 2,                                            \
                "ioaddr_size" : 3,                                             \
                "startup_time" : 180,                                          \
                "asdu_size" : 0,                                               \
                "gi_time" : 60,                                                \
                "gi_cycle" : false,                                            \
                "gi_all_ca" : false,                                           \
                "gi_repeat_count" : 2,                                         \
                "disc_qual" : "NT",                                            \
                "send_iv_time" : 0,                                            \
                "tsiv" : "REMOVE",                                             \
                "utc_time" : false,                                            \
                "comm_wttag" : false,                                          \
                "comm_parallel" : 0,                                           \
                "exec_cycl_test" : false,                                      \
                "startup_state" : true,                                        \
                "reverse" : false,                                             \
                "time_sync" : false                                            \
            }                                                                  \
        }                                                                      \
    })

// PLUGIN DEFAULT EXCHANGED DATA CONF
#define EXCHANGED_DATA_DEF                   \
    QUOTE({                                  \
        "exchanged_data" : {                 \
            "name" : "iec104client",         \
            "version" : "1.0",               \
            "asdu_list" : [                  \
                {                            \
                    "ca" : 41025,            \
                    "type_id" : "M_ME_NA_1", \
                    "label" : "TM-1",        \
                    "ioa" : 4202832          \
                },                           \
                {                            \
                    "ca" : 41025,            \
                    "type_id" : "M_ME_NA_1", \
                    "label" : "TM-2",        \
                    "ioa" : 4202852          \
                },                           \
                {                            \
                    "ca" : 41025,            \
                    "type_id" : "M_SP_TB_1", \
                    "label" : "TS-1",        \
                    "ioa" : 4206948          \
                }                            \
            ]                                \
        }                                    \
    })

// PLUGIN DEFAULT PROTOCOL TRANSLATION CONF
#define PROTOCOL_TRANSLATION_DEF                     \
    QUOTE({                                          \
        "protocol_translation" : {                   \
            "name" : "iec104_to_pivot",              \
            "version" : "1.0",                       \
            "mapping" : {                            \
                "data_object_header" : {             \
                    "doh_type" : "type_id",          \
                    "doh_ca" : "ca",                 \
                    "doh_oa" : "oa",                 \
                    "doh_cot" : "cot",               \
                    "doh_test" : "istest",           \
                    "doh_negative" : "isnegative"    \
                },                                   \
                "data_object_item" : {               \
                    "doi_ioa" : "ioa",               \
                    "doi_value" : "value",           \
                    "doi_quality" : "quality_desc",  \
                    "doi_ts" : "time_marker",        \
                    "doi_ts_flag1" : "isinvalid",    \
                    "doi_ts_flag2" : "isSummerTime", \
                    "doi_ts_flag3" : "isSubstituted" \
                }                                    \
            }                                        \
        }                                            \
    })

// PLUGIN DEFAULT TLS CONF
#define TLS_DEF                               \
    QUOTE({                                   \
        "tls_conf:" : {                       \
            "private_key" : "server-key.pem", \
            "server_cert" : "server.cer",     \
            "ca_cert" : "root.cer"            \
        }                                     \
    })

#define PROTOCOL_STACK_DEF_INFO                                                \
    QUOTE({                                                                    \
        "protocol_stack" : {                                                   \
            "name" : "iec104client",                                           \
            "version" : "1.0",                                                 \
            "transport_layer" : {                                              \
                "connection" : {                                               \
                    "path" : [                                                 \
                        {                                                      \
                            "srv_ip" : "127.0.0.1",                            \
                            "clt_ip" : "",                                     \
                            "port" : 2404                                      \
                        },                                                     \
                        {"srv_ip" : "127.0.0.1", "clt_ip" : "", "port" : 2404} \
                    ],                                                         \
                    "tls" : false                                              \
                },                                                             \
                "llevel" : 2,                                                  \
                "k_value" : 12,                                                \
                "w_value" : 8,                                                 \
                "t0_timeout" : 10,                                             \
                "t1_timeout" : 15,                                             \
                "t2_timeout" : 10,                                             \
                "t3_timeout" : 20,                                             \
                "conn_all" : false,                                            \
                "start_all" : false,                                           \
                "conn_passv" : false                                           \
            },                                                                 \
            "application_layer" : {                                            \
                "orig_addr" : 0,                                               \
                "ca_asdu_size" : 2,                                            \
                "ioaddr_size" : 3,                                             \
                "startup_time" : 180,                                          \
                "asdu_size" : 0,                                               \
                "gi_time" : 60,                                                \
                "gi_cycle" : false,                                            \
                "gi_all_ca" : false,                                           \
                "gi_repeat_count" : 2,                                         \
                "disc_qual" : "NT",                                            \
                "send_iv_time" : 0,                                            \
                "tsiv" : "REMOVE",                                             \
                "utc_time" : false,                                            \
                "comm_wttag" : false,                                          \
                "comm_parallel" : 0,                                           \
                "exec_cycl_test" : false,                                      \
                "startup_state" : true,                                        \
                "reverse" : false,                                             \
                "time_sync" : false                                            \
            }                                                                  \
        }                                                                      \
    })

// PLUGIN DEFAULT PROTOCOL TRANSLATION CONF
#define PROTOCOL_TRANSLATION_DEF                     \
    QUOTE({                                          \
        "protocol_translation" : {                   \
            "name" : "iec104_to_pivot",              \
            "version" : "1.0",                       \
            "mapping" : {                            \
                "data_object_header" : {             \
                    "doh_type" : "type_id",          \
                    "doh_ca" : "ca",                 \
                    "doh_oa" : "oa",                 \
                    "doh_cot" : "cot",               \
                    "doh_test" : "istest",           \
                    "doh_negative" : "isnegative"    \
                },                                   \
                "data_object_item" : {               \
                    "doi_ioa" : "ioa",               \
                    "doi_value" : "value",           \
                    "doi_quality" : "quality_desc",  \
                    "doi_ts" : "time_marker",        \
                    "doi_ts_flag1" : "isinvalid",    \
                    "doi_ts_flag2" : "isSummerTime", \
                    "doi_ts_flag3" : "isSubstituted" \
                }                                    \
            }                                        \
        }                                            \
    })

// PLUGIN DEFAULT TLS CONF
#define TLS_DEF                               \
    QUOTE({                                   \
        "tls_conf:" : {                       \
            "private_key" : "server-key.pem", \
            "server_cert" : "server.cer",     \
            "ca_cert" : "root.cer"            \
        }                                     \
    })

#define PROTOCOL_STACK_DEF_WARNING                                             \
    QUOTE({                                                                    \
        "protocol_stack" : {                                                   \
            "name" : "iec104client",                                           \
            "version" : "1.0",                                                 \
            "transport_layer" : {                                              \
                "connection" : {                                               \
                    "path" : [                                                 \
                        {                                                      \
                            "srv_ip" : "127.0.0.1",                            \
                            "clt_ip" : "",                                     \
                            "port" : 2404                                      \
                        },                                                     \
                        {"srv_ip" : "127.0.0.1", "clt_ip" : "", "port" : 2404} \
                    ],                                                         \
                    "tls" : false                                              \
                },                                                             \
                "llevel" : 3,                                                  \
                "k_value" : 12,                                                \
                "w_value" : 8,                                                 \
                "t0_timeout" : 10,                                             \
                "t1_timeout" : 15,                                             \
                "t2_timeout" : 10,                                             \
                "t3_timeout" : 20,                                             \
                "conn_all" : true,                                             \
                "start_all" : false,                                           \
                "conn_passv" : false                                           \
            },                                                                 \
            "application_layer" : {                                            \
                "orig_addr" : 0,                                               \
                "ca_asdu_size" : 2,                                            \
                "ioaddr_size" : 3,                                             \
                "startup_time" : 180,                                          \
                "asdu_size" : 0,                                               \
                "gi_time" : 60,                                                \
                "gi_cycle" : false,                                            \
                "gi_all_ca" : false,                                           \
                "gi_repeat_count" : 2,                                         \
                "disc_qual" : "NT",                                            \
                "send_iv_time" : 0,                                            \
                "tsiv" : "REMOVE",                                             \
                "utc_time" : false,                                            \
                "comm_wttag" : false,                                          \
                "comm_parallel" : 0,                                           \
                "exec_cycl_test" : false,                                      \
                "startup_state" : true,                                        \
                "reverse" : false,                                             \
                "time_sync" : false                                            \
            }                                                                  \
        }                                                                      \
    })

#define PROTOCOL_STACK_DEF_DEBUG                                               \
    QUOTE({                                                                    \
        "protocol_stack" : {                                                   \
            "name" : "iec104client",                                           \
            "version" : "1.0",                                                 \
            "transport_layer" : {                                              \
                "connection" : {                                               \
                    "path" : [                                                 \
                        {                                                      \
                            "srv_ip" : "127.0.0.1",                            \
                            "clt_ip" : "",                                     \
                            "port" : 2404                                      \
                        },                                                     \
                        {"srv_ip" : "127.0.0.1", "clt_ip" : "", "port" : 2404} \
                    ],                                                         \
                    "tls" : false                                              \
                },                                                             \
                "llevel" : 1,                                                  \
                "k_value" : 12,                                                \
                "w_value" : 8,                                                 \
                "t0_timeout" : 10,                                             \
                "t1_timeout" : 15,                                             \
                "t2_timeout" : 10,                                             \
                "t3_timeout" : 20,                                             \
                "conn_all" : true,                                             \
                "start_all" : false,                                           \
                "conn_passv" : false                                           \
            },                                                                 \
            "application_layer" : {                                            \
                "orig_addr" : 0,                                               \
                "ca_asdu_size" : 2,                                            \
                "ioaddr_size" : 3,                                             \
                "startup_time" : 180,                                          \
                "asdu_size" : 0,                                               \
                "gi_time" : 60,                                                \
                "gi_cycle" : false,                                            \
                "gi_all_ca" : false,                                           \
                "gi_repeat_count" : 2,                                         \
                "disc_qual" : "NT",                                            \
                "send_iv_time" : 0,                                            \
                "tsiv" : "REMOVE",                                             \
                "utc_time" : false,                                            \
                "comm_wttag" : false,                                          \
                "comm_parallel" : 0,                                           \
                "exec_cycl_test" : false,                                      \
                "startup_state" : true,                                        \
                "reverse" : false,                                             \
                "time_sync" : false                                            \
            }                                                                  \
        }                                                                      \
    })

#endif