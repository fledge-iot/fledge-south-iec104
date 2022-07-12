/*
 * Fledge IEC 104 south plugin.
 *
 * Copyright (c) 2020, RTE (https://www.rte-france.com)
 *
 * Released under the Apache 2.0 Licence
 *
 * Author: Estelle Chigot, Lucas Barret, Chauchadis Rémi, Colin Constans, Akli
 * Rahmoun
 */

#include <config_category.h>
#include <iec104.h>
#include <logger.h>
#include <plugin_api.h>
#include <version.h>

#include <fstream>
#include <iostream>
#include <string>

using namespace std;

typedef void (*INGEST_CB)(void *, Reading);

#define PLUGIN_NAME "iec104"

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

// PLUGIN DEFAULT TLS CONF
#define TLS_DEF                               \
    QUOTE({                                   \
        "tls_conf:" : {                       \
            "private_key" : "server-key.pem", \
            "server_cert" : "server.cer",     \
            "ca_cert" : "root.cer"            \
        }                                     \
    })

/**
 * Default configuration
 */
static const char *default_config = QUOTE({
    "plugin" : {
        "description" : "iec104 south plugin",
        "type" : "string",
        "default" : PLUGIN_NAME,
        "readonly" : "true"
    },

    "asset" : {
        "description" : "Asset name",
        "type" : "string",
        "default" : "iec104",
        "displayName" : "Asset Name",
        "order" : "1",
        "mandatory" : "true"
    },

    "protocol_stack" : {
        "description" : "protocol stack parameters",
        "type" : "string",
        "displayName" : "Protocol stack parameters",
        "order" : "2",
        "default" : PROTOCOL_STACK_DEF
    },

    "exchanged_data" : {
        "description" : "exchanged data list",
        "type" : "string",
        "displayName" : "Exchanged data list",
        "order" : "3",
        "default" : EXCHANGED_DATA_DEF
    },


    "tls" : {
        "description" : "tls parameters",
        "type" : "string",
        "displayName" : "TLS parameters",
        "order" : "5",
        "default" : TLS_DEF
    }
});

/**
 * The 104 plugin interface
 */
extern "C"
{
    static PLUGIN_INFORMATION info = {
        PLUGIN_NAME,            // Name
        VERSION,                // Version (automaticly generated by mkversion)
        SP_ASYNC | SP_CONTROL,  // Flags - added control
        PLUGIN_TYPE_SOUTH,      // Type
        "1.0.0",                // Interface version
        default_config          // Default configuration
    };

    /**
     * Return the information about this plugin
     */
    PLUGIN_INFORMATION *plugin_info()
    {
        Logger::getLogger()->info("104 Config is %s", info.config);
        return &info;
    }

    /**
     * Initialise the plugin, called to get the plugin handle
     */
    PLUGIN_HANDLE plugin_init(ConfigCategory *config)
    {
        IEC104 *iec104;
        Logger::getLogger()->info("Initializing the plugin");

        iec104 = new IEC104();

        if (config->itemExists("asset"))
            iec104->setAssetName(config->getValue("asset"));
        else
            iec104->setAssetName("iec 104");

        if (config->itemExists("protocol_stack") &&
            config->itemExists("exchanged_data") &&
            config->itemExists("tls"))
            iec104->setJsonConfig(config->getValue("protocol_stack"),
                                  config->getValue("exchanged_data"),
                                  config->getValue("tls"));

        return (PLUGIN_HANDLE)iec104;
    }

    /**
     * Start the Async handling for the plugin
     */
    void plugin_start(PLUGIN_HANDLE *handle)
    {
        if (!handle) return;

        Logger::getLogger()->info("Starting the plugin");

        auto *iec104 = reinterpret_cast<IEC104 *>(handle);
        iec104->start();
    }

    /**
     * Register ingest callback
     */
    void plugin_register_ingest(PLUGIN_HANDLE *handle, INGEST_CB cb, void *data)
    {
        if (!handle) throw exception();

        auto *iec104 = reinterpret_cast<IEC104 *>(handle);
        iec104->registerIngest(data, cb);
    }

    /**
     * Poll for a plugin reading
     */
    Reading plugin_poll(PLUGIN_HANDLE *handle)
    {
        throw runtime_error(
            "IEC_104 is an async plugin, poll should not be called");
    }

    /**
     * Reconfigure the plugin
     */
    void plugin_reconfigure(PLUGIN_HANDLE *handle, string &newConfig)
    {
        ConfigCategory config("newConfig", newConfig);
        auto *iec104 = reinterpret_cast<IEC104 *>(*handle);

        if (config.itemExists("protocol_stack") &&
            config.itemExists("exchanged_data") &&
            config.itemExists("tls"))
            iec104->setJsonConfig(config.getValue("protocol_stack"),
                                  config.getValue("exchanged_data"),
                                  config.getValue("tls"));

        if (config.itemExists("asset"))
        {
            iec104->setAssetName(config.getValue("asset"));
            Logger::getLogger()->info(
                "104 plugin restart after reconfigure asset");
            iec104->restart();
        }
    }

    /**
     * Shutdown the plugin
     */
    void plugin_shutdown(PLUGIN_HANDLE *handle)
    {
        auto *iec104 = reinterpret_cast<IEC104 *>(handle);

        iec104->stop();
        delete iec104;
    }

    /**
     * plugin plugin_write entry point
     * NOT USED
     */
    bool plugin_write(PLUGIN_HANDLE *handle, string &name, string &value)
    {
        return false;
    }

    /**
     * plugin plugin_operation entry point
     */
    bool plugin_operation(PLUGIN_HANDLE *handle, string &operation, int count,
                          PLUGIN_PARAMETER **params)
    {
        if (!handle) throw exception();

        auto *iec104 = reinterpret_cast<IEC104 *>(handle);

        return iec104->operation(operation, count, params);
    }

}  // extern "C"
