#include <config_category.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <iec104.h>
#include <plugin_api.h>

#include <string>

using namespace std;

typedef void (*INGEST_CB)(void *, Reading);

extern "C"
{
    PLUGIN_HANDLE plugin_init(ConfigCategory *config);
    void plugin_register_ingest(PLUGIN_HANDLE *handle, INGEST_CB cb,
                                void *data);
    Reading plugin_poll(PLUGIN_HANDLE *handle);
    void plugin_reconfigure(PLUGIN_HANDLE *handle, string &newConfig);
    bool plugin_write(PLUGIN_HANDLE *handle, string &name, string &value);
    bool plugin_operation(PLUGIN_HANDLE *handle, string &operation, int count,
                          PLUGIN_PARAMETER **params);
    void plugin_shutdown(PLUGIN_HANDLE *handle);
    void plugin_start(PLUGIN_HANDLE *handle);
};

#define PROTOCOL_TRANSLATION_DEF \
    QUOTE({"protocol_translation" : {"name" : "test_pt"}})

#define PROTOCOL_STACK_DEF QUOTE({"protocol_stack" : {"name" : "test_ps"}})

#define EXCHANGED_DATA_DEF QUOTE({"exchanged_data" : {"name" : "test_ed"}})

#define TLS_DEF QUOTE({"tls_conf" : {"name" : "test_tls"}})

static const char *default_config = QUOTE({
    "plugin" : {
        "description" : "iec104 south plugin",
        "type" : "string",
        "default" : "TEST_PLUGIN",
        "readonly" : "true"
    },

    "asset" : {
        "description" : "Asset name",
        "type" : "string",
        "default" : "iec104_TEST",
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

    "protocol_translation" : {
        "description" : "protocol translation mapping",
        "type" : "string",
        "displayName" : "Protocol translation mapping",
        "order" : "4",
        "default" : PROTOCOL_TRANSLATION_DEF
    },

    "tls" : {
        "description" : "tls parameters",
        "type" : "string",
        "displayName" : "TLS parameters",
        "order" : "5",
        "default" : TLS_DEF
    }
});

TEST(IEC104, PluginInit)
{
    ConfigCategory *config = new ConfigCategory("Test_Config", default_config);
    config->setItemsValueFromDefault();
    ASSERT_NO_THROW(PLUGIN_HANDLE handle = plugin_init(config));

    ConfigCategory *emptyConfig = new ConfigCategory();
    ASSERT_NO_THROW(PLUGIN_HANDLE handle = plugin_init(emptyConfig));
}

void ingestCallback(void *data, Reading reading) {}

TEST(IEC104, PluginRegisterIngest)
{
    ConfigCategory *emptyConfig = new ConfigCategory();
    PLUGIN_HANDLE handle = plugin_init(emptyConfig);

    ASSERT_NO_THROW(
        plugin_register_ingest((PLUGIN_HANDLE *)handle, ingestCallback, NULL));

    handle = nullptr;
    ASSERT_THROW(
        plugin_register_ingest((PLUGIN_HANDLE *)handle, ingestCallback, NULL),
        exception);
}

TEST(IEC104, PluginPoll)
{
    ConfigCategory *emptyConfig = new ConfigCategory();
    PLUGIN_HANDLE handle = plugin_init(emptyConfig);

    ASSERT_THROW(plugin_poll((PLUGIN_HANDLE *)handle), runtime_error);
}

string conf = QUOTE({
    "plugin" : {
        "description" : "iec104 south plugin",
        "type" : "string",
        "value" : "TEST_PLUGIN",
        "readonly" : "true"
    },

    "protocol_stack" : {
        "description" : "protocol stack parameters",
        "type" : "string",
        "displayName" : "Protocol stack parameters",
        "order" : "2",
        "value" : PROTOCOL_STACK_DEF
    },

    "exchanged_data" : {
        "description" : "exchanged data list",
        "type" : "string",
        "displayName" : "Exchanged data list",
        "order" : "3",
        "value" : EXCHANGED_DATA_DEF
    },

    "protocol_translation" : {
        "description" : "protocol translation mapping",
        "type" : "string",
        "displayName" : "Protocol translation mapping",
        "order" : "4",
        "value" : PROTOCOL_TRANSLATION_DEF
    },

    "tls" : {
        "description" : "tls parameters",
        "type" : "string",
        "displayName" : "TLS parameters",
        "order" : "5",
        "value" : TLS_DEF
    }
});

TEST(IEC104, PluginReconfigure)
{
    ConfigCategory *emptyConfig = new ConfigCategory();
    PLUGIN_HANDLE handle = plugin_init(emptyConfig);
    ASSERT_NO_THROW(plugin_reconfigure((PLUGIN_HANDLE *)handle, conf));

    // TODO test when there is an asset name, but it will crash because of the
    // restart
}

TEST(IEC104, PluginWrite)
{
    ConfigCategory *emptyConfig = new ConfigCategory();
    PLUGIN_HANDLE handle = plugin_init(emptyConfig);

    string name("name");
    string value("value");
    ASSERT_FALSE(plugin_write((PLUGIN_HANDLE *)handle, name, value));
}

TEST(IEC104, PluginOperation)
{
    ConfigCategory *emptyConfig = new ConfigCategory();
    PLUGIN_HANDLE handle = plugin_init(emptyConfig);

    string operation("operation_test");
    ASSERT_NO_THROW(
        plugin_operation((PLUGIN_HANDLE *)handle, operation, 10, NULL));
    ASSERT_FALSE(
        plugin_operation((PLUGIN_HANDLE *)handle, operation, 10, NULL));

    handle = nullptr;
    ASSERT_THROW(plugin_operation((PLUGIN_HANDLE *)handle, operation, 10, NULL),
                 exception);
}

TEST(IEC104, PluginStop)
{
    ConfigCategory *emptyConfig = new ConfigCategory();
    PLUGIN_HANDLE handle = plugin_init(emptyConfig);
    ASSERT_NO_THROW(plugin_shutdown((PLUGIN_HANDLE *)handle));
}

class MockIEC104 : public IEC104
{
public:
    MockIEC104() : IEC104(){};
    MOCK_METHOD(void, start, ());
};
