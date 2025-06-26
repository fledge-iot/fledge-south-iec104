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

#include <reading.h>

#include <string>
#include <lib60870/hal_time.h>

#include "iec104.h"
#include "iec104_client.h"
#include "iec104_client_redgroup.h"
#include "iec104_client_config.h"
#include "iec104_utility.h"


using namespace std;

/** Constructor for the iec104 plugin */
IEC104::IEC104()
{
    m_config = std::make_shared<IEC104ClientConfig>();
}

void IEC104::setJsonConfig(const std::string& stack_configuration,
                           const std::string& msg_configuration,
                           const std::string& tls_configuration)
{
    m_config = std::make_shared<IEC104ClientConfig>();

    m_config->importProtocolConfig(stack_configuration);
    m_config->importExchangeConfig(msg_configuration);
    m_config->importTlsConfig(tls_configuration);
}

void IEC104::start()
{
    std::string beforeLog = Iec104Utility::PluginName + " - IEC104::start -";
    Iec104Utility::log_info("%s Starting iec104", beforeLog.c_str());

/* NOT NEEDED, LET SOUTH SERVICE MANAGE MIN LOG LEVEL
    switch (m_config->LogLevel())
    {
        case 1:
            Logger::getLogger()->setMinLevel("debug");
            break;
        //LCOV_EXCL_START
        case 2:
            Logger::getLogger()->setMinLevel("info");
            break;
        case 3:
            Logger::getLogger()->setMinLevel("warning");
            break;
        default:
            Logger::getLogger()->setMinLevel("error");
            break;
        //LCOV_EXCL_STOP
    }
    */

    m_client = std::make_shared<IEC104Client>(this, m_config);

    m_client->start();
}

/** Disconnect from the iec104 servers */
void IEC104::stop()
{
    if (m_client != nullptr)
    {
        m_client->stop();
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
    std::string beforeLog = Iec104Utility::PluginName + " - IEC104::ingest -";
    if (!m_ingest) {
        Iec104Utility::log_error("%s Ingest callback is not defined", beforeLog.c_str());
        return;
    }
    Reading reading(assetName, points);
    uint64_t tsInNs = Hal_getTimeInNs();
    std::string tsStrInNs = std::to_string(tsInNs);
    //Iec104Utility::log_info("%s Ingest reading: %s", beforeLog.c_str(), reading.toJSON().c_str());
    Iec104Utility::log_info("%s Ingest reading: %s TimestampInNs: %s", beforeLog.c_str(), reading.toJSON().c_str(), tsStrInNs.c_str());
    m_ingest(m_data, reading);
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
IEC104::m_singleCommandOperation(int count, PLUGIN_PARAMETER** params, bool withTime) const
{
    std::string beforeLog = Iec104Utility::PluginName + " - IEC104::m_singleCommandOperation -";
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

        if(withTime) {
            try {
                time = std::stol(params[TS]->value);
            } catch (const std::invalid_argument &e) {
                Iec104Utility::log_error("%s (CA: %i IOA: %i) Cannot convert time '%s' to integer: %s",
                                        beforeLog.c_str(), ca, ioa, params[TS]->value.c_str(), e.what());
                return false;
            } catch (const std::out_of_range &e) {
                Iec104Utility::log_error("%s (CA: %i IOA: %i) Cannot convert time '%s' to integer: %s",
                                        beforeLog.c_str(), ca, ioa, params[TS]->value.c_str(), e.what());
                return false;
            }
        }

        Iec104Utility::log_debug("%s operate: single command - CA: %i IOA: %i value: %i select: %i timestamp: %ld", beforeLog.c_str(),
                                ca, ioa, value, select, time);

        return m_client->sendSingleCommand(ca, ioa, value, withTime, select, time);
    }
    else {
        Iec104Utility::log_error("%s invalid number of parameters: %d, but expected 9", beforeLog.c_str(), count);
        return false;
    }
}

bool
IEC104::m_doubleCommandOperation(int count, PLUGIN_PARAMETER** params, bool withTime) const
{
    std::string beforeLog = Iec104Utility::PluginName + " - IEC104::m_doubleCommandOperation -";
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

        if(withTime) {
            try {
                time = std::stol(params[TS]->value);
            } catch (const std::invalid_argument &e) {
                Iec104Utility::log_error("%s (CA: %i IOA: %i) Cannot convert time '%s' to integer: %s",
                                        beforeLog.c_str(), ca, ioa, params[TS]->value.c_str(), e.what());
                return false;
            } catch (const std::out_of_range &e) {
                Iec104Utility::log_error("%s (CA: %i IOA: %i) Cannot convert time '%s' to integer: %s",
                                        beforeLog.c_str(), ca, ioa, params[TS]->value.c_str(), e.what());
                return false;
            }
        }

        Iec104Utility::log_debug("%s operate: double command - CA: %i IOA: %i value: %i select: %i timestamp: %ld", beforeLog.c_str(),
                                ca, ioa, value, select, time);

        return m_client->sendDoubleCommand(ca, ioa, value, withTime, select, time);
    }
    else {
        Iec104Utility::log_error("%s invalid number of parameters: %d, but expected 9", beforeLog.c_str(), count);
        return false;
    }
}

bool
IEC104::m_stepCommandOperation(int count, PLUGIN_PARAMETER** params, bool withTime) const
{
    std::string beforeLog = Iec104Utility::PluginName + " - IEC104::m_stepCommandOperation -";
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

        if(withTime) {
            try {
                time = std::stol(params[TS]->value);
            } catch (const std::invalid_argument &e) {
                Iec104Utility::log_error("%s (CA: %i IOA: %i) Cannot convert time '%s' to integer: %s",
                                        beforeLog.c_str(), ca, ioa, params[TS]->value.c_str(), e.what());
                return false;
            } catch (const std::out_of_range &e) {
                Iec104Utility::log_error("%s (CA: %i IOA: %i) Cannot convert time '%s' to integer: %s",
                                        beforeLog.c_str(), ca, ioa, params[TS]->value.c_str(), e.what());
                return false;
            }
        }

        Iec104Utility::log_debug("%s operate: step command - CA: %i IOA: %i value: %i select: %i timestamp: %ld", beforeLog.c_str(),
                                ca, ioa, value, select, time);

        return m_client->sendStepCommand(ca, ioa, value, withTime, select, time);
    }
    else {
        Iec104Utility::log_error("%s invalid number of parameters: %d, but expected 9", beforeLog.c_str(), count);
        return false;
    }
}

bool
IEC104::m_setpointNormalized(int count, PLUGIN_PARAMETER** params, bool withTime) const
{
    std::string beforeLog = Iec104Utility::PluginName + " - IEC104::m_setpointNormalized -";
    if (count > 8) {
        // common address of the asdu
        int ca = atoi(params[CA]->value.c_str());

        // information object address
        int32_t ioa = atoi(params[IOA]->value.c_str());

        // normalized value (range -1.0 ... 1.0)
        // TODO check range?
        float value = (float)atof(params[VALUE]->value.c_str());

        long time = 0;

        if(withTime) {
            try {
                time = std::stol(params[TS]->value);
            } catch (const std::invalid_argument &e) {
                Iec104Utility::log_error("%s (CA: %i IOA: %i) Cannot convert time '%s' to integer: %s",
                                        beforeLog.c_str(), ca, ioa, params[TS]->value.c_str(), e.what());
                return false;
            } catch (const std::out_of_range &e) {
                Iec104Utility::log_error("%s (CA: %i IOA: %i) Cannot convert time '%s' to integer: %s",
                                        beforeLog.c_str(), ca, ioa, params[TS]->value.c_str(), e.what());
                return false;
            }
        }

        Iec104Utility::log_debug("%s operate: setpoint command (normalized) - CA: %i IOA: %i value: %i timestamp: %ld", beforeLog.c_str(),
                                ca, ioa, value, time);

        return m_client->sendSetpointNormalized(ca, ioa, value, withTime, time);
    }
    else {
        Iec104Utility::log_error("%s invalid number of parameters: %d, but expected 9", beforeLog.c_str(), count);
        return false;
    }
}

bool
IEC104::m_setpointScaled(int count, PLUGIN_PARAMETER** params, bool withTime) const
{
    std::string beforeLog = Iec104Utility::PluginName + " - IEC104::m_setpointScaled -";
    if (count > 8) {
        // common address of the asdu
        int ca = atoi(params[CA]->value.c_str());

        // information object address
        int32_t ioa = atoi(params[IOA]->value.c_str());

        // scaled value (range -32,768 to +32,767)
        // TODO check range
        int value = atoi(params[VALUE]->value.c_str());

        long time = 0;

        if(withTime) {
            try {
                time = std::stol(params[TS]->value);
            } catch (const std::invalid_argument &e) {
                Iec104Utility::log_error("%s (CA: %i IOA: %i) Cannot convert time '%s' to integer: %s",
                                        beforeLog.c_str(), ca, ioa, params[TS]->value.c_str(), e.what());
                return false;
            } catch (const std::out_of_range &e) {
                Iec104Utility::log_error("%s (CA: %i IOA: %i) Cannot convert time '%s' to integer: %s",
                                        beforeLog.c_str(), ca, ioa, params[TS]->value.c_str(), e.what());
                return false;
            }
        }

        Iec104Utility::log_debug("%s operate: setpoint command (scaled) - CA: %i IOA: %i value: %i timestamp: %ld", beforeLog.c_str(),
                                ca, ioa, value, time);

        return m_client->sendSetpointScaled(ca, ioa, value, withTime, time);
    }
    else {
        Iec104Utility::log_error("%s invalid number of parameters: %d, but expected 9", beforeLog.c_str(), count);
        return false;
    }
}

bool
IEC104::m_setpointShort(int count, PLUGIN_PARAMETER** params, bool withTime) const
{
    std::string beforeLog = Iec104Utility::PluginName + " - IEC104::m_setpointShort -";
    if (count > 8) {
        // common address of the asdu
        int ca = atoi(params[CA]->value.c_str());

        // information object address
        int32_t ioa = atoi(params[IOA]->value.c_str());

        // short float value
        float value = (float)atof(params[VALUE]->value.c_str());

        long time = 0;

        if(withTime) {
            try {
                time = std::stol(params[TS]->value);
            } catch (const std::invalid_argument &e) {
                Iec104Utility::log_error("%s (CA: %i IOA: %i) Cannot convert time '%s' to integer: %s",
                                        beforeLog.c_str(), ca, ioa, params[TS]->value.c_str(), e.what());
                return false;
            } catch (const std::out_of_range &e) {
                Iec104Utility::log_error("%s (CA: %i IOA: %i) Cannot convert time '%s' to integer: %s",
                                        beforeLog.c_str(), ca, ioa, params[TS]->value.c_str(), e.what());
                return false;
            }
        }

        Iec104Utility::log_debug("%s operate: setpoint command (short) - CA: %i IOA: %i value: %i timestamp: %ld", beforeLog.c_str(),
                                ca, ioa, value, time);

        return m_client->sendSetpointShort(ca, ioa, value, withTime, time);
    }
    else {
        Iec104Utility::log_error("%s invalid number of parameters: %d, but expected 9", beforeLog.c_str(), count);
        return false;
    }
}

// Utility function for logging
static std::pair<std::string, std::string> paramsToStr(PLUGIN_PARAMETER** params, int count) {
	std::string namesStr("[");
    std::string paramsStr("[");
	for(int i=0;i<count;i++){
		if(i>0) {
			namesStr += ", ";
            paramsStr += ", ";
		}
        namesStr += "\"";
        paramsStr += "\"";
        if (params[i]) {
            namesStr += params[i]->name;
            paramsStr += params[i]->value;
        }
        namesStr += "\"";
        paramsStr += "\"";
	}
	namesStr += "]";
    paramsStr += "]";
	return {namesStr, paramsStr};
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
IEC104::operation(const std::string& operation, int count, PLUGIN_PARAMETER** params) const
{
    std::string beforeLog = Iec104Utility::PluginName + " - IEC104::operation -";
    if (m_client == nullptr) {
        Iec104Utility::log_error("%s operation called but plugin is not yet initialized", beforeLog.c_str());
        return false;
    }

    auto namesParamsPair = paramsToStr(params, count);
    Iec104Utility::log_info("%s Received operation: {type: \"%s\", nbParams=%d, names=%s, parameters=%s}",
                            beforeLog.c_str(), operation.c_str(), count, namesParamsPair.first.c_str(), namesParamsPair.second.c_str());

    if (operation == "CS104_Connection_sendInterrogationCommand")
    {
        int ca = atoi(params[0]->value.c_str());

        return m_client->sendInterrogationCommand(ca);
    }
    //LCOV_EXCL_START
    else if (operation == "CS104_Connection_sendTestCommandWithTimestamp")
    {
        int casdu = atoi(params[0]->value.c_str());

        //TODO implement?

        return false;
    }
    //LCOV_EXCL_STOP
    else if (operation == "IEC104Command"){
        std::string type = params[0]->value;

        if(type[0] == '"'){
            type = type.substr(1,type.length()-2);
        }

        int typeID = m_config->getTypeIdFromString(type);

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
                Iec104Utility::log_error("%s Unrecognised command type %s", beforeLog.c_str(), type.c_str());
                return false;
        }
    }
    else if (operation == "request_connection_status") {
        return m_client->sendConnectionStatus();
    }
    else if (operation == "north_status") {
        std::string north_status_type = params[0]->value;
        if(north_status_type[0] == '"'){
            north_status_type = north_status_type.substr(1,north_status_type.length()-2);
        }
        if (north_status_type == "init_socket_finished"){
            // schedule a Interrogation command to the control station
            if(m_client->scheduleGI()){
                Iec104Utility::log_info("init_socket_finished operation received, scheduling GI");
            }else{
                Iec104Utility::log_info("Too many GI schedule GI ignored");
            }
                return true;

        }
        Iec104Utility::log_error("%s Unrecognised operation %s type=%s", beforeLog.c_str(), operation.c_str(), north_status_type.c_str());
    }

    Iec104Utility::log_error("%s Unrecognised operation %s", beforeLog.c_str(), operation.c_str());

    return false;
}
