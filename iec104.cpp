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

#include <iec104.h>
#include <logger.h>
#include <reading.h>

#include <cmath>
#include <fstream>
#include <iostream>
#include <utility>

using namespace std;
using namespace nlohmann;

bool IEC104::m_comm_wttag = false;
string IEC104::m_tsiv;

/** Constructor for the iec104 plugin */
IEC104::IEC104() : m_client(nullptr) {}

void IEC104::setJsonConfig(const std::string& stack_configuration,
                           const std::string& msg_configuration,
                           const std::string& tls_configuration)
{
    Logger::getLogger()->info("Reading json config string...");

    try
    {
        m_stack_configuration =
            json::parse(stack_configuration)["protocol_stack"];
    }
    catch (json::parse_error& e)
    {
        Logger::getLogger()->fatal(
            "Couldn't read protocol_stack json config string : " +
            string(e.what()));
        throw(string("json config error"));
    }

    try
    {
        m_msg_configuration = json::parse(msg_configuration)["exchanged_data"];
    }
    catch (json::parse_error& e)
    {
        Logger::getLogger()->fatal(
            "Couldn't read exchanged_data json config string : " +
            string(e.what()));
        throw(string("json config error"));
    }

    try
    {
        m_tls_configuration = json::parse(tls_configuration)["tls_conf"];
    }
    catch (json::parse_error& e)
    {
        Logger::getLogger()->fatal(
            "Couldn't read tls_conf json config string : " + string(e.what()));
        throw(string("json config error"));
    }
}

void IEC104::prepareParameters(CS104_Connection connection)
{
    // Transport layer initialization
    sCS104_APCIParameters apci_parameters = {12, 8,  10,
                                             15, 10, 20};  // default values
    apci_parameters.k = m_getConfigValue<int>(
        m_stack_configuration, "/transport_layer/k_value"_json_pointer);
    apci_parameters.w = m_getConfigValue<int>(
        m_stack_configuration, "/transport_layer/w_value"_json_pointer);
    apci_parameters.t0 = m_getConfigValue<int>(
        m_stack_configuration, "/transport_layer/t0_timeout"_json_pointer);
    apci_parameters.t1 = m_getConfigValue<int>(
        m_stack_configuration, "/transport_layer/t1_timeout"_json_pointer);
    apci_parameters.t2 = m_getConfigValue<int>(
        m_stack_configuration, "/transport_layer/t2_timeout"_json_pointer);
    apci_parameters.t3 = m_getConfigValue<int>(
        m_stack_configuration, "/transport_layer/t3_timeout"_json_pointer);

    CS104_Connection_setAPCIParameters(connection, &apci_parameters);

    int asdu_size = m_getConfigValue<int>(
        m_stack_configuration, "/application_layer/asdu_size"_json_pointer);

    // If 0 is set in the configuration file, use the maximum value (249 for
    // IEC104)
    if (asdu_size == 0) asdu_size = 249;

    // Application layer initialization
    sCS101_AppLayerParameters app_layer_parameters = {
        1, 1, 2, 0, 2, 3, 249};  // default values
    app_layer_parameters.originatorAddress = m_getConfigValue<int>(
        m_stack_configuration, "/application_layer/orig_addr"_json_pointer);
    app_layer_parameters.sizeOfCA = m_getConfigValue<int>(
        m_stack_configuration,
        "/application_layer/ca_asdu_size"_json_pointer);  // 2
    app_layer_parameters.sizeOfIOA = m_getConfigValue<int>(
        m_stack_configuration,
        "/application_layer/ioaddr_size"_json_pointer);  // 3
    app_layer_parameters.maxSizeOfASDU = asdu_size;

    CS104_Connection_setAppLayerParameters(connection, &app_layer_parameters);

    // m_comm_wttag = m_getConfigValue<bool>(
    //     *m_stack_configuration, "/application_layer/comm_wttag"_json_pointer);
    // m_tsiv = m_getConfigValue<string>(m_stack_configuration,
    //                                   "/application_layer/tsiv"_json_pointer);

    Logger::getLogger()->info("Connection initialized");
}

void IEC104::restart()
{
    stop();
    start();
}

void IEC104::start()
{
    Logger::getLogger()->info("Starting iec104");

    switch (m_getConfigValue<int>(m_stack_configuration,
                                  "/transport_layer/llevel"_json_pointer))
    {
        case 1:
            Logger::getLogger()->setMinLevel("debug");
            break;
        case 2:
            Logger::getLogger()->setMinLevel("info");
            break;
        case 3:
            Logger::getLogger()->setMinLevel("warning");
            break;
        default:
            Logger::getLogger()->setMinLevel("error");
            break;
    }

    m_client = new IEC104Client(this, &m_stack_configuration, &m_msg_configuration);

    m_client->start();
}

/** Disconnect from the iec104 servers */
void IEC104::stop()
{
     if (m_client != nullptr)
    {
        m_client->stop();

        delete m_client;
        m_client = nullptr;
    }
}

/**
 * Called when a data changed event is received. This calls back to the south
 * service and adds the points to the readings queue to send.
 *
 * @param points    The points in the reading we must create
 */
// void IEC104::ingest(Reading& reading) { (*m_ingest)(m_data, reading); }
void IEC104::ingest(std::string assetName, std::vector<Datapoint*>& points)
{
    if (m_ingest)
        m_ingest(m_data, Reading(assetName, points));
}

/**
 * Save the callback function and its data
 * @param data   The Ingest function data
 * @param cb     The callback function to call
 */
void IEC104::registerIngest(void* data, INGEST_CB cb)
{
    m_ingest = cb;
    m_data = data;
}

template <class T>
T IEC104::m_getConfigValue(json configuration, json_pointer<json> path)
{
    T typed_value;

    try
    {
        typed_value = configuration.at(path);
    }
    catch (json::parse_error& e)
    {
        Logger::getLogger()->fatal("Couldn't parse value " + path.to_string() +
                                   " : " + e.what());
    }
    catch (json::out_of_range& e)
    {
        Logger::getLogger()->fatal("Couldn't reach value " + path.to_string() +
                                   " : " + e.what());
    }

    return typed_value;
}

void IEC104::m_sendInterrogationCommmands()
{
    int broadcast_ca = m_getBroadcastCA();

    int gi_repeat_count = m_getConfigValue<int>(
        m_stack_configuration,
        "/application_layer/gi_repeat_count"_json_pointer);
    int gi_time = m_getConfigValue<int>(
        m_stack_configuration, "/application_layer/gi_time"_json_pointer);

    // If we try to send to every ca
    if (m_getConfigValue<bool>(m_stack_configuration,
                               "/application_layer/gi_all_ca"_json_pointer))
    {
        // For every ca
        for (auto& element : m_msg_configuration["asdu_list"])
            m_sendInterrogationCommmandToCA(
                m_getConfigValue<unsigned int>(element, "/ca"_json_pointer),
                gi_repeat_count, gi_time);
    }
    else  // Otherwise, broadcast (causes Segmentation fault)
    {
        // m_sendInterrogationCommmandToCA(broadcast_ca, gi_repeat_count,
        // gi_time);
    }

    Logger::getLogger()->info("Interrogation command sent");
}

void IEC104::m_sendInterrogationCommmandToCA(unsigned int ca,
                                             int gi_repeat_count, int gi_time)
{
    Logger::getLogger()->info("Sending interrogation command to ca = " +
                              to_string(ca));

    // Try gi_repeat_count times if doesn't work
    bool sentWithSuccess = false;
    for (unsigned int count = 0; count < gi_repeat_count; count++)
    {
        for (auto connection : m_connections)
        {
            // If it does work, wait gi_time seconds to complete, and
            // break
            if (CS104_Connection_sendInterrogationCommand(
                    connection, CS101_COT_ACTIVATION, ca, IEC60870_QOI_STATION))
            {
                sentWithSuccess = true;
                Thread_sleep(1000 * gi_time);
                break;
            }
        }
        if (sentWithSuccess) break;
    }
}

int IEC104::m_getBroadcastCA()
{
    int ca_asdu_size = m_getConfigValue<int>(
        m_stack_configuration, "/application_layer/ca_asdu_size"_json_pointer);

    // Broadcast address is the maximum value possible, ie 2^(8*asdu_size) (8 is
    // 1 byte)
    // https://www.openmuc.org/iec-60870-5-104/javadoc/org/openmuc/j60870/ASdu.html
    return pow(2, 8 * ca_asdu_size) - 1;
}

/*
CS104_Connection IEC104::m_createTlsConnection(const char* ip, int port)
{
    TLSConfiguration TLSConfig =
        TLSConfiguration_create();  // TLSConfiguration_create makes plugin
                                    // unresponsive, without giving any
                                    // log/exception
    Logger::getLogger()->debug("Af TLSConf create");

    std::string private_key =
        "$FLEDGE_ROOT/data/etc/certs/" +
        m_getConfigValue<string>(m_tls_configuration,
                                 "/private_key"_json_pointer);
    std::string server_cert =
        "$FLEDGE_ROOT/data/etc/certs/" +
        m_getConfigValue<string>(m_tls_configuration,
                                 "/server_cert"_json_pointer);
    std::string ca_cert =
        "$FLEDGE_ROOT/data/etc/certs/" +
        m_getConfigValue<string>(m_tls_configuration, "/ca_cert"_json_pointer);

    TLSConfiguration_setOwnCertificateFromFile(TLSConfig, server_cert.c_str());
    TLSConfiguration_setOwnKeyFromFile(TLSConfig, private_key.c_str(), nullptr);
    TLSConfiguration_addCACertificateFromFile(TLSConfig, ca_cert.c_str());

    return CS104_Connection_createSecure(ip, port, TLSConfig);
}
*/

int IEC104::m_watchdog(int delay, int checkRes, bool* flag, std::string id)
{
    using namespace std::chrono;
    high_resolution_clock::time_point beginning_time =
        high_resolution_clock::now();
    while (true)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(checkRes));
        high_resolution_clock::time_point current_time =
            high_resolution_clock::now();
        duration<double, std::milli> time_span = current_time - beginning_time;
        if (*flag)  // Operation has been validated
        {
            Logger::getLogger()->info(id + " completed in " +
                                      to_string(time_span.count() / 1000) +
                                      " seconds.");
            return 0;
        }
        else  // Operation has not been validated yet
        {
            if (time_span.count() > 1000 * delay)
            {  // Time has expired
                Logger::getLogger()->warn(
                    id + " is taking to int64_t, restarting ...");
                return 1;
            }
        }
    }
}

bool IEC104::m_singleCommandOperation(int count, PLUGIN_PARAMETER** params, bool withTime)
{
    if (count > 3) {
        // common address of the asdu
        int ca = atoi(params[0]->value.c_str());

        // information object address
        int32_t ioa = atoi(params[1]->value.c_str());

        // command state to send, must be a boolean
        // 0 = off, 1 otherwise
        bool value = static_cast<bool>(atoi(params[2]->value.c_str()));

        // select or execute, must be a boolean
        // 0 = execute, otherwise = select
        bool select = static_cast<bool>(atoi(params[2]->value.c_str()));

        return m_client->sendSingleCommand(ca, ioa, value, withTime, select);
    }
    else {
        Logger::getLogger()->error("operation parameter missing");
        return false;
    }
}

bool IEC104::m_doubleCommandOperation(int count, PLUGIN_PARAMETER** params, bool withTime)
{
    if (count > 3) {
        // common address of the asdu
        int ca = atoi(params[0]->value.c_str());

        // information object address
        int32_t ioa = atoi(params[1]->value.c_str());

        // the command state to send, 4 possible values
        // (0 = not permitted, 1 = off, 2 = on, 3 = not permitted)
        int value = atoi(params[2]->value.c_str());

        // select or execute, must be a boolean
        // 0 = execute, otherwise = select
        bool select = static_cast<bool>(atoi(params[2]->value.c_str()));

        return m_client->sendDoubleCommand(ca, ioa, value, withTime, select);
    }
    else {
        Logger::getLogger()->error("operation parameter missing");
        return false;
    }
}

bool IEC104::m_stepCommandOperation(int count, PLUGIN_PARAMETER** params, bool withTime)
{
    if (count > 3) {
        // common address of the asdu
        int ca = atoi(params[0]->value.c_str());

        // information object address
        int32_t ioa = atoi(params[1]->value.c_str());

        // the command state to send, 4 possible values
        // (0 = invalid_0, 1 = lower, 2 = higher, 3 = invalid_3)
        int value = atoi(params[2]->value.c_str());

        // select or execute, must be a boolean
        // 0 = execute, otherwise = select
        bool select = static_cast<bool>(atoi(params[2]->value.c_str()));

        return m_client->sendStepCommand(ca, ioa, value, withTime, select);
    }
    else {
        Logger::getLogger()->error("operation parameter missing");
        return false;
    }
}

bool IEC104::m_setpointNormalized(int count, PLUGIN_PARAMETER** params, bool withTime)
{
    if (count > 2) {
        // common address of the asdu
        int ca = atoi(params[0]->value.c_str());

        // information object address
        int32_t ioa = atoi(params[1]->value.c_str());

        // normalized value (range -1.0 ... 1.0)
        // TODO check range?
        float value = (float)atof(params[2]->value.c_str());

        return m_client->sendSetpointNormalized(ca, ioa, value, withTime);
    }
    else {
        Logger::getLogger()->error("operation parameter missing");
        return false;
    }
}

bool IEC104::m_setpointScaled(int count, PLUGIN_PARAMETER** params, bool withTime)
{
    if (count > 2) {
        // common address of the asdu
        int ca = atoi(params[0]->value.c_str());

        // information object address
        int32_t ioa = atoi(params[1]->value.c_str());

        // scaled value (range -32,768 to +32,767)
        // TODO check range
        int value = atoi(params[2]->value.c_str());

        return m_client->sendSetpointScaled(ca, ioa, value, withTime);
    }
    else {
        Logger::getLogger()->error("operation parameter missing");
        return false;
    }
}

bool IEC104::m_setpointShort(int count, PLUGIN_PARAMETER** params, bool withTime)
{
    if (count > 2) {
        // common address of the asdu
        int ca = atoi(params[0]->value.c_str());

        // information object address
        int32_t ioa = atoi(params[1]->value.c_str());

        // short float value
        float value = (float)atof(params[2]->value.c_str());

        return m_client->sendSetpointShort(ca, ioa, value, withTime);
    }
    else {
        Logger::getLogger()->error("operation parameter missing");
        return false;
    }
}

/**
 * SetPoint operation.
 * This is the function used to send an ASDU to the control station
 * @param operation     name of the command asdu
 * @param params        data object items of the command to send, composed of a
 * name and a value
 * @param count         number of parameters
 */
bool IEC104::operation(const std::string& operation, int count,
                       PLUGIN_PARAMETER** params)
{
    printf("IEC104::operation(%s)\n", operation.c_str());

    if (operation.compare("CS104_Connection_sendInterrogationCommand") == 0)
    {
        int ca = atoi(params[0]->value.c_str());

        return m_client->sendInterrogationCommand(ca);
    }
    else if (operation.compare(
                    "CS104_Connection_sendTestCommandWithTimestamp") == 0)
    {
        int casdu = atoi(params[0]->value.c_str());

        //TODO implement?

        return false;
    }
    else if (operation.compare("SingleCommandWithCP56Time2a") == 0)
    {
        return m_singleCommandOperation(count, params, true);
    }
    else if (operation.compare("SingleCommand") == 0)
    {
        return m_singleCommandOperation(count, params, false);
    }
    else if (operation.compare("DoubleCommandWithCP56Time2a") == 0)
    {
        return m_doubleCommandOperation(count, params, true);
    }
    else if (operation.compare("DoubleCommand") == 0)
    {
        return m_doubleCommandOperation(count, params, false);
    }
    else if (operation.compare("StepCommandWithCP56Time2a") == 0)
    {
        return m_stepCommandOperation(count, params, true);
    }
    else if (operation.compare("StepCommand") == 0)
    {
        return m_stepCommandOperation(count, params, true);
    }
    else if (operation.compare("SetpointNormalizedWithCP56Time2a") == 0)
    {
        return m_setpointNormalized(count, params, true);
    }
    else if (operation.compare("SetpointNormalized") == 0)
    {
        return m_setpointNormalized(count, params, false);
    }
    else if (operation.compare("SetpointScaledWithCP56Time2a") == 0)
    {
        return m_setpointScaled(count, params, true);
    }
    else if (operation.compare("SetpointScaled") == 0)
    {
        return m_setpointScaled(count, params, false);
    }
    else if (operation.compare("SetpointShortWithCP56Time2a") == 0)
    {
        return m_setpointShort(count, params, true);
    }
    else if (operation.compare("SetpointShort") == 0)
    {
        return m_setpointShort(count, params, false);
    }

    Logger::getLogger()->error("Unrecognised operation %s", operation.c_str());
    return false;
}

