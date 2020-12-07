#include <iec104.h>
#include <plugin_api.h>
#include <string>
#include <logger.h>
#include <config_category.h>
#include <rapidjson/document.h>
#include <version.h>

typedef void (*INGEST_CB)(void *, Reading);

using namespace std;

#define PLUGIN_NAME "iec104"

/**
 * Default configuration
 */

const char *default_config = QUOTE ({
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

                                        "ip" : {
        "description" : "IP of the 104 Server",
        "type" : "string",
        "default" : "127.0.0.1",
        "displayName" : "104 Server IP",
        "order" : "2"
    },

                                        "port" : {
        "description" : "Port number of the 104 server",
        "type" : "integer",
        "default" : "2404",
        "displayName" : "104 Server port"

    }

                                    });

/**
 * The 104 plugin interface
 */
extern "C" {
static PLUGIN_INFORMATION info = {
        PLUGIN_NAME,              // Name
        VERSION,                  // Version
        SP_ASYNC,          // Flags
        PLUGIN_TYPE_SOUTH,        // Type
        "1.0.0",                  // Interface version
        default_config          // Default configuration
};

/**
 * Return the information about this plugin
 */
PLUGIN_INFORMATION *plugin_info() {
    Logger::getLogger()->info("104 Config is %s", info.config);
    return &info;
}

PLUGIN_HANDLE plugin_init(ConfigCategory *config) {
    IEC104 *iec104;
    Logger::getLogger()->info("Initializing the plugin");

    string ip = "127.0.0.1";
    uint16_t port = IEC_60870_5_104_DEFAULT_PORT;

    if (config->itemExists("ip")){
        ip = config->getValue("ip");
        Logger::getLogger()->info(ip);
    }
    if (config->itemExists("port")){
        port = static_cast<uint16_t>(stoi(config->getValue("port")));
    }

    iec104 = new IEC104(ip.c_str(), port);

    if (config->itemExists("asset")) {
        iec104->setAssetName(config->getValue("asset"));
    } else {
        iec104->setAssetName("iec 104");
    }

    return (PLUGIN_HANDLE) iec104;
}

/**
 * Start the Async handling for the plugin
 */
void plugin_start(PLUGIN_HANDLE *handle) {
    if (!handle)
        return;
    Logger::getLogger()->info("Starting the plugin");
    IEC104 *iec104 = (IEC104 *) handle;
    iec104->start();

}

/**
 * Register ingest callback
 */
void plugin_register_ingest(PLUGIN_HANDLE *handle, INGEST_CB cb, void *data) {
    if (!handle)
        throw new exception();

    IEC104 *iec104 = (IEC104 *) handle;
    iec104->registerIngest(data, cb);
}

/**
 * Poll for a plugin reading
 */
Reading plugin_poll(PLUGIN_HANDLE *handle) {

    throw runtime_error("IEC_104 is an async plugin, poll should not be called");
}

/**
 * Reconfigure the plugin
 *
 */
void plugin_reconfigure(PLUGIN_HANDLE *handle, string &newConfig) {
    ConfigCategory config("new", newConfig);
    auto *iec104 = (IEC104 *) *handle;

    if (config.itemExists("ip") && config.itemExists("port")) {

        string ip = config.getValue("ip");
        uint16_t port = static_cast<uint16_t>(stoi(config.getValue("port")));
        iec104->setPort(port);
        iec104->setIp(ip.c_str());

        Logger::getLogger()->info("104 plugin restart after reconfigure ip/port");
        iec104->restart();
    }

    if (config.itemExists("asset")) {
        iec104->setAssetName(config.getValue("asset"));
        Logger::getLogger()->info("104 plugin restart after reconfigure asset");
        iec104->restart();
    }

}

/**
 * Shutdown the plugin
*/
void plugin_shutdown(PLUGIN_HANDLE *handle) {
    auto *iec104 = (IEC104 *) handle;

    iec104->stop();
    delete iec104;
}

}
