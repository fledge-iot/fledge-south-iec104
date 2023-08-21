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

#include <iec104.h>
#include <iec104_client_redgroup.h>
#include <logger.h>
#include <reading.h>
#include <utils.h>

#include <cmath>
#include <fstream>
#include <iostream>
#include <utility>

using namespace std;

/** Constructor for the iec104 plugin */
IEC104::IEC104() : m_client(nullptr)
{
    m_config = new IEC104ClientConfig();
}

IEC104::~IEC104()
{
    delete m_config;
}

void IEC104::setJsonConfig(const std::string& stack_configuration,
                           const std::string& msg_configuration,
                           const std::string& tls_configuration)
{
    if (m_config != nullptr) {
        delete m_config;
        m_config = new IEC104ClientConfig();
    }

    m_config->importProtocolConfig(stack_configuration);
    m_config->importExchangeConfig(msg_configuration);
    m_config->importTlsConfig(tls_configuration);
}

void IEC104::start()
{
    Logger::getLogger()->info("Starting iec104");

    switch (m_config->LogLevel())
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

    m_client = new IEC104Client(this, m_config);

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

enum CommandParameters{
    TYPE,
    CA,
    IOA,
    COT,
    NEGATIVE,
    SE,
    TEST,
    TS,
    VALUE
};

bool
IEC104::m_singleCommandOperation(int count, PLUGIN_PARAMETER** params, bool withTime)
{
    if (count > 8) {
        // common address of the asdu
        int ca = atoi(params[CA]->value.c_str());

        // information object address
        int32_t ioa = atoi(params[IOA]->value.c_str());

        // command state to send, must be a boolean
        // 0 = off, 1 otherwise
        bool value = static_cast<bool>(atoi(params[VALUE]->value.c_str()));

        // select or execute, must be a boolean
        // 0 = execute, otherwise = select
        bool select = static_cast<bool>(atoi(params[SE]->value.c_str()));

        long time = 0;

        if(withTime)
            time = stol(params[TS]->value);

        Logger::getLogger()->debug("operate: single command - CA: %i IOA: %i value: %i select: %i timestamp: %i", ca, ioa, value, select, time);

        return m_client->sendSingleCommand(ca, ioa, value, withTime, select, time);
    }
    else {
        Logger::getLogger()->error("operation parameter missing");
        return false;
    }
}

bool
IEC104::m_doubleCommandOperation(int count, PLUGIN_PARAMETER** params, bool withTime)
{
    if (count > 8) {
        // common address of the asdu
        int ca = atoi(params[CA]->value.c_str());

        // information object address
        int32_t ioa = atoi(params[IOA]->value.c_str());

        // the command state to send, 4 possible values
        // (0 = not permitted, 1 = off, 2 = on, 3 = not permitted)
        int value = atoi(params[VALUE]->value.c_str());

        // select or execute, must be a boolean
        // 0 = execute, otherwise = select
        bool select = static_cast<bool>(atoi(params[SE]->value.c_str()));

        long time = 0;

        if(withTime)
            time = stol(params[TS]->value);

        Logger::getLogger()->debug("operate: double command - CA: %i IOA: %i value: %i select: %i timestamp: %i", ca, ioa, value, select, time);

        return m_client->sendDoubleCommand(ca, ioa, value, withTime, select, time);
    }
    else {
        Logger::getLogger()->error("operation parameter missing");
        return false;
    }
}

bool
IEC104::m_stepCommandOperation(int count, PLUGIN_PARAMETER** params, bool withTime)
{
    if (count > 8) {
        // common address of the asdu
        int ca = atoi(params[CA]->value.c_str());

        // information object address
        int32_t ioa = atoi(params[IOA]->value.c_str());

        // the command state to send, 4 possible values
        // (0 = invalid_0, 1 = lower, 2 = higher, 3 = invalid_3)
        int value = atoi(params[VALUE]->value.c_str());

        // select or execute, must be a boolean
        // 0 = execute, otherwise = select
        bool select = static_cast<bool>(atoi(params[SE]->value.c_str()));

        long time = 0;

        if(withTime)
            time = stol(params[TS]->value);

        Logger::getLogger()->debug("operate: step command - CA: %i IOA: %i value: %i select: %i timestamp: %i", ca, ioa, value, select, time);

        return m_client->sendStepCommand(ca, ioa, value, withTime, select, time);
    }
    else {
        Logger::getLogger()->error("operation parameter missing");
        return false;
    }
}

bool
IEC104::m_setpointNormalized(int count, PLUGIN_PARAMETER** params, bool withTime)
{
    if (count > 8) {
        // common address of the asdu
        int ca = atoi(params[CA]->value.c_str());

        // information object address
        int32_t ioa = atoi(params[IOA]->value.c_str());

        // normalized value (range -1.0 ... 1.0)
        // TODO check range?
        float value = (float)atof(params[VALUE]->value.c_str());

        long time = 0;

        if(withTime)
            time = stol(params[TS]->value);

        Logger::getLogger()->debug("operate: setpoint command (normalized) - CA: %i IOA: %i value: %i timestamp: %i", ca, ioa, value, time);

        return m_client->sendSetpointNormalized(ca, ioa, value, withTime, time);
    }
    else {
        Logger::getLogger()->error("operation parameter missing");
        return false;
    }
}

bool
IEC104::m_setpointScaled(int count, PLUGIN_PARAMETER** params, bool withTime)
{
    if (count > 8) {
        // common address of the asdu
        int ca = atoi(params[CA]->value.c_str());

        // information object address
        int32_t ioa = atoi(params[IOA]->value.c_str());

        // scaled value (range -32,768 to +32,767)
        // TODO check range
        int value = atoi(params[VALUE]->value.c_str());

        long time = 0;

        if(withTime)
            time = stol(params[TS]->value);

        Logger::getLogger()->debug("operate: setpoint command (scaled) - CA: %i IOA: %i value: %i timestamp: %i", ca, ioa, value, time);

        return m_client->sendSetpointScaled(ca, ioa, value, withTime, time);
    }
    else {
        Logger::getLogger()->error("operation parameter missing");
        return false;
    }
}

bool
IEC104::m_setpointShort(int count, PLUGIN_PARAMETER** params, bool withTime)
{
    if (count > 8) {
        // common address of the asdu
        int ca = atoi(params[CA]->value.c_str());

        // information object address
        int32_t ioa = atoi(params[IOA]->value.c_str());

        // short float value
        float value = (float)atof(params[VALUE]->value.c_str());

        long time = 0;

        if(withTime)
            time = stol(params[TS]->value);

        Logger::getLogger()->debug("operate: setpoint command (short) - CA: %i IOA: %i value: %i timestamp: %i", ca, ioa, value, time);

        return m_client->sendSetpointShort(ca, ioa, value, withTime, time);
    }
    else {
        Logger::getLogger()->error("operation parameter missing");
        return false;
    }
}

/**
 * SetPoint operation.
 * This is the function used to send an ASDU to the control station
 *
 * @param operation     name of the command asdu
 * @param params        data object items of the command to send, composed of a name and a value
 * @param count         number of parameters
 *
 * @return true when the command has been accepted, false otherwise
 */
bool
IEC104::operation(const std::string& operation, int count,
                       PLUGIN_PARAMETER** params)
{
    if (m_client == nullptr) {
        Logger::getLogger()->error("operation called but plugin is not yet initialized");

        return false;
    }

    if (operation == "CS104_Connection_sendInterrogationCommand")
    {
        int ca = atoi(params[0]->value.c_str());

        return m_client->sendInterrogationCommand(ca);
    }
    else if (operation == "CS104_Connection_sendTestCommandWithTimestamp")
    {
        int casdu = atoi(params[0]->value.c_str());

        //TODO implement?

        return false;
    }
    else if (operation == "IEC104Command"){
        std::string type = params[0]->value;

        int typeID = m_config->GetTypeIdByName(type);

        switch (typeID){
            case C_SC_NA_1: return m_singleCommandOperation(count, params, false);
            case C_SC_TA_1: return m_singleCommandOperation(count, params, true);
            case C_DC_NA_1: return m_doubleCommandOperation(count, params, false);
            case C_DC_TA_1: return m_doubleCommandOperation(count, params, true);
            case C_RC_NA_1: return m_stepCommandOperation(count, params, false);
            case C_RC_TA_1: return m_stepCommandOperation(count, params, true);
            case C_SE_NA_1: return m_setpointNormalized(count, params, false);
            case C_SE_TA_1: return m_setpointNormalized(count, params, true);
            case C_SE_NB_1: return m_setpointScaled(count, params, false);
            case C_SE_TB_1: return m_setpointScaled(count, params, true);
            case C_SE_NC_1: return m_setpointShort(count, params, false);
            case C_SE_TC_1: return m_setpointShort(count, params, true);
            default:
                Logger::getLogger()->error("Unrecognised command type %s", type.c_str());
                return false;
        };
    }
    else if (operation == "request_connection_status") {
        Logger::getLogger()->info("received request_connection_status", operation.c_str());
        return m_client->sendConnectionStatus();
    }

    Logger::getLogger()->error("Unrecognised operation %s", operation.c_str());

    return false;
}
