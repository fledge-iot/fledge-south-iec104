/*
 * Fledge IEC 104 south plugin.
 *
 * Copyright (c) 2020, RTE (https://www.rte-france.com)
 *
 * Released under the Apache 2.0 Licence
 *
 * Author: Michael Zillgith
 * 
 */

#include <config_category.h>
#include <plugin_api.h>
#include <version.h>

#include "iec104.h"
#include "iec104_utility.h"

using namespace std;

typedef void (*INGEST_CB)(void *, Reading);

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
        "type" : "JSON",
        "displayName" : "Protocol stack parameters",
        "order" : "2",
        "default" : QUOTE({
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
                    "gi_cycle" : 60,                
                    "gi_all_ca" : false,                                          
                    "utc_time" : false,                
                    "cmd_wttag" : false,              
                    "cmd_parallel" : 0,                                    
                    "time_sync" : 0                 
                }                 
            }                     
        })
    },
    "exchanged_data" : {
        "description" : "exchanged data list",
        "type" : "JSON",
        "displayName" : "Exchanged data list",
        "order" : "3",
        "default" : QUOTE({
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
                        "label":"TS-1",
                        "protocols":[
                           {
                              "name":"iec104",
                              "address":"41025-4206948",
                              "typeid":"M_SP_TB_1"
                           }
                        ]
                    }                   
                ]
            }
        })
    },
    "tls" : {
        "description" : "tls parameters",
        "type" : "JSON",
        "displayName" : "TLS parameters",
        "order" : "4",
        "default" : QUOTE({      
            "tls_conf" : {
                "private_key" : "iec104_client.key",
                "own_cert" : "iec104_client.cer",
                "ca_certs" : [
                    {
                        "cert_file": "iec104_ca.cer"
                    },
                    {
                        "cert_file": "iec104_ca2.cer"
                    }
                ],
                "remote_certs" : [
                    {
                        "cert_file": "iec104_server.cer"
                    }
                ]
            }      
        })
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
        std::string beforeLog = Iec104Utility::PluginName + " - plugin_info -";
        Iec104Utility::log_info("%s 104 Config is %s", beforeLog.c_str(), info.config);
        return &info;
    }

    /**
     * Initialise the plugin, called to get the plugin handle
     */
    PLUGIN_HANDLE plugin_init(ConfigCategory *config)
    {
        std::string beforeLog = Iec104Utility::PluginName + " - plugin_init -";
        IEC104* iec104 = nullptr;
        Iec104Utility::log_info("%s Initializing the plugin", beforeLog.c_str());

        if (config == nullptr) {
            Iec104Utility::log_warn("%s No config provided for plugin, using default config", beforeLog.c_str());
            auto pluginInfo = plugin_info();
            config = new ConfigCategory("newConfig", pluginInfo->config);
            config->setItemsValueFromDefault();
        }

        iec104 = new IEC104();

        if (iec104) {
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
        }

        Iec104Utility::log_info("%s Plugin initialized", beforeLog.c_str());

        return (PLUGIN_HANDLE)iec104;
    }

    /**
     * Start the Async handling for the plugin
     */
    void plugin_start(PLUGIN_HANDLE *handle)
    {
        std::string beforeLog = Iec104Utility::PluginName + " - plugin_start -";
        if (!handle) return;

        Iec104Utility::log_info("%s Starting the plugin...", beforeLog.c_str());

        auto *iec104 = reinterpret_cast<IEC104 *>(handle);
        iec104->start();
        Iec104Utility::log_info("%s Plugin started", beforeLog.c_str());
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
        throw runtime_error("IEC_104 is an async plugin, poll should not be called");
    }

    /**
     * Reconfigure the plugin
     */
    void plugin_reconfigure(PLUGIN_HANDLE *handle, string &newConfig)
    {
        std::string beforeLog = Iec104Utility::PluginName + " - plugin_reconfigure -";
        Iec104Utility::log_info("%s New config: %s", beforeLog.c_str(), newConfig.c_str());

        ConfigCategory config("newConfig", newConfig);
        auto *iec104 = reinterpret_cast<IEC104 *>(*handle);

        iec104->stop();

        if (config.itemExists("protocol_stack") &&
            config.itemExists("exchanged_data") &&
            config.itemExists("tls"))
            iec104->setJsonConfig(config.getValue("protocol_stack"),
                                  config.getValue("exchanged_data"),
                                  config.getValue("tls"));

        if (config.itemExists("asset"))
        {
            iec104->setAssetName(config.getValue("asset"));
            Iec104Utility::log_info("%s 104 plugin restart after reconfigure asset", beforeLog.c_str());
            iec104->start();
        }
        else {
            Iec104Utility::log_error("%s 104 plugin restart failed", beforeLog.c_str());
        }
    }

    /**
     * Shutdown the plugin
     */
    void plugin_shutdown(PLUGIN_HANDLE *handle)
    {
        std::string beforeLog = Iec104Utility::PluginName + " - plugin_shutdown -";
        Iec104Utility::log_info("%s Shutting down the plugin...", beforeLog.c_str());
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
