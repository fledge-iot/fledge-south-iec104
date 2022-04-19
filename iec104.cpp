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

// Qualifier of command
#define NoAddDefinition 0
#define ShortPulse 1
#define LongPulse 2
#define PersistentOutput 3
#define Reserved 4  // > 3 value

// (S/E bit) select command
#define Execute false
#define Select true

using namespace std;
using namespace nlohmann;

bool IEC104::m_comm_wttag = false;
string IEC104::m_tsiv;
json IEC104::m_stack_configuration;
json IEC104::m_tls_configuration;
json IEC104::m_msg_configuration;
json IEC104::m_pivot_configuration;

/** Constructor for the iec104 plugin */
IEC104::IEC104() : m_client(nullptr) {}

// Map of all handled ASDU type by the plugin, see m_asduReceivedHandler
map<int, string> mapOfAsdu = {
    {M_ME_NB_1, "M_ME_NB_1"}, {M_SP_NA_1, "M_SP_NA_1"},
    {M_SP_TB_1, "M_SP_TB_1"}, {M_DP_NA_1, "M_DP_NA_1"},
    {M_DP_TB_1, "M_DP_TB_1"}, {M_ST_NA_1, "M_ST_NA_1"},
    {M_ST_TB_1, "M_ST_TB_1"}, {M_ME_NA_1, "M_ME_NA_1"},
    {M_ME_TD_1, "M_ME_TD_1"}, {M_ME_TE_1, "M_ME_TE_1"},
    {M_ME_NC_1, "M_ME_NC_1"}, {M_ME_TF_1, "M_ME_TF_1"},
    {C_SC_TA_1, "C_SC_TA_1"}, {C_DC_TA_1, "C_DC_TA_1"}};

void IEC104::setJsonConfig(const std::string& stack_configuration,
                           const std::string& msg_configuration,
                           const std::string& pivot_configuration,
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
        m_pivot_configuration =
            json::parse(pivot_configuration)["protocol_translation"];
    }
    catch (json::parse_error& e)
    {
        Logger::getLogger()->fatal(
            "Couldn't read protocol_translation json config string : " +
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

void IEC104::m_connectionHandler(void* parameter, CS104_Connection connection,
                                 CS104_ConnectionEvent event)
{
    if (event == CS104_CONNECTION_CLOSED)
    {
        Logger::getLogger()->warn("CONNECTION LOST... Reconnecting");
        auto iec104 = reinterpret_cast<IEC104*>(parameter);

        auto connection_it = find(iec104->m_connections.begin(),
                                  iec104->m_connections.end(), connection);
        *connection_it = nullptr;
        iec104->restart();
    }
}

bool IEC104::m_asduReceivedHandlerP(void* parameter, int address,
                                    CS101_ASDU asdu)
{
    return m_asduReceivedHandler(parameter, address, asdu);
}

/**
 * @brief Callback handler function to handle an ASDU
 *
 * @param parameter an IEC104 Client
 * @param address for CS104 it has to be ignored
 * @param asdu the asdu to handle
 * @return true if type is supported
 * @return false if type is not supported
 */
bool IEC104::m_asduReceivedHandler(void* parameter, int address,
                                   CS101_ASDU asdu)
{
    vector<Datapoint*> datapoints;
    vector<string> labels;
    auto mclient = static_cast<IEC104Client*>(parameter);

    switch (CS101_ASDU_getTypeID(asdu))
    {
        case M_ME_NB_1:
            handleASDU(labels, datapoints, mclient, asdu, handleM_ME_NB_1);
            break;
        case M_SP_NA_1:
            handleASDU(labels, datapoints, mclient, asdu, handleM_SP_NA_1);
            break;
        case M_SP_TB_1:
            handleASDU(labels, datapoints, mclient, asdu, handleM_SP_TB_1);
            break;
        case M_DP_NA_1:
            handleASDU(labels, datapoints, mclient, asdu, handleM_DP_NA_1);
            break;
        case M_DP_TB_1:
            handleASDU(labels, datapoints, mclient, asdu, handleM_DP_TB_1);
            break;
        case M_ST_NA_1:
            handleASDU(labels, datapoints, mclient, asdu, handleM_ST_NA_1);
            break;
        case M_ST_TB_1:
            handleASDU(labels, datapoints, mclient, asdu, handleM_ST_TB_1);
            break;
        case M_ME_NA_1:
            handleASDU(labels, datapoints, mclient, asdu, handleM_ME_NA_1);
            break;
        case M_ME_TD_1:
            handleASDU(labels, datapoints, mclient, asdu, handleM_ME_TD_1);
            break;
        case M_ME_TE_1:
            handleASDU(labels, datapoints, mclient, asdu, handleM_ME_TE_1);
            break;
        case M_ME_NC_1:
            handleASDU(labels, datapoints, mclient, asdu, handleM_ME_NC_1);
            break;
        case M_ME_TF_1:
            handleASDU(labels, datapoints, mclient, asdu, handleM_ME_TF_1);
            break;
        case M_EI_NA_1:
            Logger::getLogger()->info("Received end of initialization");
            break;
        case C_IC_NA_1:
            Logger::getLogger()->info("General interrogation command");
            break;
        case C_TS_TA_1:
            Logger::getLogger()->info("Test command with time tag CP56Time2a");
            break;
        case C_SC_TA_1:
            handleASDU(labels, datapoints, mclient, asdu, handleC_SC_TA_1);
            break;
        case C_DC_TA_1:
            handleASDU(labels, datapoints, mclient, asdu, handleC_DC_TA_1);
            break;
        default:
            Logger::getLogger()->error("Type of message not supported");
            return false;
    }
    if (!datapoints.empty()) mclient->sendData(asdu, datapoints, labels);

    return true;
}

/**
 * @brief Generic function that will handle information related to the received
 * ASDU and call the callback function that is dependent of a specific type
 *
 * @param labels reference to a vector that will contain label for each handled
 * information object the asdu passed in parameter
 * @param datapoints reference to a vector of datapoints
 * @param mclient IEC104 client
 * @param asdu the asdu to handle
 * @param callback the prefered function to handle a specific ASDU type
 */
void IEC104::handleASDU(vector<string>& labels, vector<Datapoint*>& datapoints,
                        IEC104Client* mclient, CS101_ASDU& asdu,
                        IEC104_ASDUHandler callback)
{
    IEC60870_5_TypeID asduID = CS101_ASDU_getTypeID(asdu);
    Logger::getLogger()->debug("Received " + mapOfAsdu[asduID]);
    unsigned int ca = CS101_ASDU_getCA(asdu);
    string label;

    for (int i = 0; i < CS101_ASDU_getNumberOfElements(asdu); i++)
    {
        InformationObject io = CS101_ASDU_getElement(asdu, i);
        uint64_t ioa = InformationObject_getObjectAddress(io);
        if (!(label =
                  IEC104::m_checkExchangedDataLayer(ca, mapOfAsdu[asduID], ioa))
                 .empty())
        {
            callback(datapoints, label, mclient, ca, asdu, io, ioa);
            labels.push_back(label);
        }
    }
}

// Each of the following function handle a specific type of ASDU. They cast the
// contained IO into a specific object that is strictly linked to the type
// for example a MeasuredValueScaled is type M_ME_NB_1
void IEC104::handleM_ME_NB_1(vector<Datapoint*>& datapoints, string& label,
                             IEC104Client* mclient, unsigned int& ca,
                             CS101_ASDU& asdu, InformationObject& io,
                             uint64_t& ioa)
{
    auto io_casted = (MeasuredValueScaled)io;
    int64_t value =
        MeasuredValueScaled_getValue((MeasuredValueScaled)io_casted);
    QualityDescriptor qd = MeasuredValueScaled_getQuality(io_casted);
    mclient->addData(datapoints, ioa, label, value, qd);

    MeasuredValueScaled_destroy(io_casted);
}

void IEC104::handleM_SP_NA_1(vector<Datapoint*>& datapoints, string& label,
                             IEC104Client* mclient, unsigned int& ca,
                             CS101_ASDU& asdu, InformationObject& io,
                             uint64_t& ioa)
{
    auto io_casted = (SinglePointInformation)io;
    int64_t value =
        SinglePointInformation_getValue((SinglePointInformation)io_casted);
    QualityDescriptor qd =
        SinglePointInformation_getQuality((SinglePointInformation)io_casted);
    mclient->addData(datapoints, ioa, label, value, qd);

    SinglePointInformation_destroy(io_casted);
}

void IEC104::handleM_SP_TB_1(vector<Datapoint*>& datapoints, string& label,
                             IEC104Client* mclient, unsigned int& ca,
                             CS101_ASDU& asdu, InformationObject& io,
                             uint64_t& ioa)
{
    auto io_casted = (SinglePointWithCP56Time2a)io;
    int64_t value =
        SinglePointInformation_getValue((SinglePointInformation)io_casted);
    QualityDescriptor qd =
        SinglePointInformation_getQuality((SinglePointInformation)io_casted);
    if (m_comm_wttag)
    {
        CP56Time2a ts = SinglePointWithCP56Time2a_getTimestamp(io_casted);
        bool is_invalid = CP56Time2a_isInvalid(ts);
        if (m_tsiv == "PROCESS" || !is_invalid)
            mclient->addData(datapoints, ioa, label, value, qd, ts);
    }
    else
        mclient->addData(datapoints, ioa, label, value, qd);

    SinglePointWithCP56Time2a_destroy(io_casted);
}

void IEC104::handleM_DP_NA_1(vector<Datapoint*>& datapoints, string& label,
                             IEC104Client* mclient, unsigned int& ca,
                             CS101_ASDU& asdu, InformationObject& io,
                             uint64_t& ioa)
{
    auto io_casted = (DoublePointInformation)io;
    int64_t value =
        DoublePointInformation_getValue((DoublePointInformation)io_casted);
    QualityDescriptor qd =
        DoublePointInformation_getQuality((DoublePointInformation)io_casted);
    mclient->addData(datapoints, ioa, label, value, qd);

    DoublePointInformation_destroy(io_casted);
}

void IEC104::handleM_DP_TB_1(vector<Datapoint*>& datapoints, string& label,
                             IEC104Client* mclient, unsigned int& ca,
                             CS101_ASDU& asdu, InformationObject& io,
                             uint64_t& ioa)
{
    auto io_casted = (DoublePointWithCP56Time2a)io;
    int64_t value =
        DoublePointInformation_getValue((DoublePointInformation)io_casted);
    QualityDescriptor qd =
        DoublePointInformation_getQuality((DoublePointInformation)io_casted);
    if (m_comm_wttag)
    {
        CP56Time2a ts = DoublePointWithCP56Time2a_getTimestamp(io_casted);
        bool is_invalid = CP56Time2a_isInvalid(ts);
        if (m_tsiv == "PROCESS" || !is_invalid)
            mclient->addData(datapoints, ioa, label, value, qd, ts);
    }
    else
        mclient->addData(datapoints, ioa, label, value, qd);

    DoublePointWithCP56Time2a_destroy(io_casted);
}

void IEC104::handleM_ST_NA_1(vector<Datapoint*>& datapoints, string& label,
                             IEC104Client* mclient, unsigned int& ca,
                             CS101_ASDU& asdu, InformationObject& io,
                             uint64_t& ioa)
{
    auto io_casted = (StepPositionInformation)io;
    int64_t value =
        StepPositionInformation_getValue((StepPositionInformation)io_casted);
    QualityDescriptor qd =
        StepPositionInformation_getQuality((StepPositionInformation)io_casted);
    mclient->addData(datapoints, ioa, label, value, qd);

    StepPositionInformation_destroy(io_casted);
}

void IEC104::handleM_ST_TB_1(vector<Datapoint*>& datapoints, string& label,
                             IEC104Client* mclient, unsigned int& ca,
                             CS101_ASDU& asdu, InformationObject& io,
                             uint64_t& ioa)
{
    auto io_casted = (StepPositionWithCP56Time2a)io;
    int64_t value =
        StepPositionInformation_getValue((StepPositionInformation)io_casted);
    QualityDescriptor qd =
        StepPositionInformation_getQuality((StepPositionInformation)io_casted);
    if (m_comm_wttag)
    {
        CP56Time2a ts = StepPositionWithCP56Time2a_getTimestamp(io_casted);
        bool is_invalid = CP56Time2a_isInvalid(ts);
        if (m_tsiv == "PROCESS" || !is_invalid)
            mclient->addData(datapoints, ioa, label, value, qd, ts);
    }
    else
        mclient->addData(datapoints, ioa, label, value, qd);

    StepPositionWithCP56Time2a_destroy(io_casted);
}

void IEC104::handleM_ME_NA_1(vector<Datapoint*>& datapoints, string& label,
                             IEC104Client* mclient, unsigned int& ca,
                             CS101_ASDU& asdu, InformationObject& io,
                             uint64_t& ioa)
{
    auto io_casted = (MeasuredValueNormalized)io;
    float value =
        MeasuredValueNormalized_getValue((MeasuredValueNormalized)io_casted);
    QualityDescriptor qd =
        MeasuredValueNormalized_getQuality((MeasuredValueNormalized)io_casted);
    mclient->addData(datapoints, ioa, label, value, qd);

    MeasuredValueNormalized_destroy(io_casted);
}

void IEC104::handleM_ME_TD_1(vector<Datapoint*>& datapoints, string& label,
                             IEC104Client* mclient, unsigned int& ca,
                             CS101_ASDU& asdu, InformationObject& io,
                             uint64_t& ioa)
{
    auto io_casted = (MeasuredValueNormalizedWithCP56Time2a)io;
    float value =
        MeasuredValueNormalized_getValue((MeasuredValueNormalized)io_casted);
    QualityDescriptor qd =
        MeasuredValueNormalized_getQuality((MeasuredValueNormalized)io_casted);
    if (m_comm_wttag)
    {
        CP56Time2a ts =
            MeasuredValueNormalizedWithCP56Time2a_getTimestamp(io_casted);
        bool is_invalid = CP56Time2a_isInvalid(ts);
        if (m_tsiv == "PROCESS" || !is_invalid)
            mclient->addData(datapoints, ioa, label, value, qd, ts);
    }
    else
        mclient->addData(datapoints, ioa, label, value, qd);

    MeasuredValueNormalizedWithCP56Time2a_destroy(io_casted);
}

void IEC104::handleM_ME_TE_1(vector<Datapoint*>& datapoints, string& label,
                             IEC104Client* mclient, unsigned int& ca,
                             CS101_ASDU& asdu, InformationObject& io,
                             uint64_t& ioa)
{
    auto io_casted = (MeasuredValueScaledWithCP56Time2a)io;
    int64_t value =
        MeasuredValueScaled_getValue((MeasuredValueScaled)io_casted);
    QualityDescriptor qd =
        MeasuredValueScaled_getQuality((MeasuredValueScaled)io_casted);
    if (m_comm_wttag)
    {
        CP56Time2a ts =
            MeasuredValueScaledWithCP56Time2a_getTimestamp(io_casted);
        bool is_invalid = CP56Time2a_isInvalid(ts);
        if (m_tsiv == "PROCESS" || !is_invalid)
            mclient->addData(datapoints, ioa, label, value, qd, ts);
    }
    else
        mclient->addData(datapoints, ioa, label, value, qd);

    MeasuredValueScaledWithCP56Time2a_destroy(io_casted);
}

void IEC104::handleM_ME_NC_1(vector<Datapoint*>& datapoints, string& label,
                             IEC104Client* mclient, unsigned int& ca,
                             CS101_ASDU& asdu, InformationObject& io,
                             uint64_t& ioa)
{
    auto io_casted = (MeasuredValueShort)io;
    float value = MeasuredValueShort_getValue((MeasuredValueShort)io_casted);
    QualityDescriptor qd =
        MeasuredValueShort_getQuality((MeasuredValueShort)io_casted);
    mclient->addData(datapoints, ioa, label, value, qd);

    MeasuredValueShort_destroy(io_casted);
}

void IEC104::handleM_ME_TF_1(vector<Datapoint*>& datapoints, string& label,
                             IEC104Client* mclient, unsigned int& ca,
                             CS101_ASDU& asdu, InformationObject& io,
                             uint64_t& ioa)
{
    auto io_casted = (MeasuredValueShortWithCP56Time2a)io;
    float value = MeasuredValueShort_getValue((MeasuredValueShort)io_casted);
    QualityDescriptor qd =
        MeasuredValueShort_getQuality((MeasuredValueShort)io_casted);
    if (m_comm_wttag)
    {
        CP56Time2a ts =
            MeasuredValueShortWithCP56Time2a_getTimestamp(io_casted);
        bool is_invalid = CP56Time2a_isInvalid(ts);
        if (m_tsiv == "PROCESS" || !is_invalid)
            mclient->addData(datapoints, ioa, label, value, qd, ts);
    }
    else
        mclient->addData(datapoints, ioa, label, value, qd);

    MeasuredValueShortWithCP56Time2a_destroy(io_casted);
}

void IEC104::handleC_SC_TA_1(vector<Datapoint*>& datapoints, string& label,
                             IEC104Client* mclient, unsigned int& ca,
                             CS101_ASDU& asdu, InformationObject& io,
                             uint64_t& ioa)
{
    auto io_casted = (SingleCommandWithCP56Time2a)io;
    int64_t state = SingleCommand_getState((SingleCommand)io_casted);

    QualifierOfCommand qu = SingleCommand_getQU((SingleCommand)io_casted);

    if (m_comm_wttag)
    {
        CP56Time2a ts = SingleCommandWithCP56Time2a_getTimestamp(io_casted);
        bool is_invalid = CP56Time2a_isInvalid(ts);
        if (m_tsiv == "PROCESS" || !is_invalid)
            mclient->addData(datapoints, ioa, label, state, qu, ts);
    }
    else
        mclient->addData(datapoints, ioa, label, state, qu);

    SingleCommandWithCP56Time2a_destroy(io_casted);
}

void IEC104::handleC_DC_TA_1(vector<Datapoint*>& datapoints, string& label,
                             IEC104Client* mclient, unsigned int& ca,
                             CS101_ASDU& asdu, InformationObject& io,
                             uint64_t& ioa)
{
    auto io_casted = (DoubleCommandWithCP56Time2a)io;
    int64_t state = DoubleCommand_getState((DoubleCommand)io_casted);

    QualifierOfCommand qu = DoubleCommand_getQU((DoubleCommand)io_casted);

    if (m_comm_wttag)
    {
        CP56Time2a ts = DoubleCommandWithCP56Time2a_getTimestamp(io_casted);
        bool is_invalid = CP56Time2a_isInvalid(ts);
        if (m_tsiv == "PROCESS" || !is_invalid)
            mclient->addData(datapoints, ioa, label, state, qu, ts);
    }
    else
        mclient->addData(datapoints, ioa, label, state, qu);

    DoubleCommandWithCP56Time2a_destroy(io_casted);
}

void IEC104::restart()
{
    stop();
    start();
}

void IEC104::prepareParameters(CS104_Connection& connection)
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

    m_comm_wttag = m_getConfigValue<bool>(
        m_stack_configuration, "/application_layer/comm_wttag"_json_pointer);
    m_tsiv = m_getConfigValue<string>(m_stack_configuration,
                                      "/application_layer/tsiv"_json_pointer);

    Logger::getLogger()->info("Connection initialized");
}

void IEC104::connect(unsigned int connection_index)
{
    CS104_Connection connection = m_connections[connection_index];
    prepareParameters(connection);
    // Connection
    if (m_getConfigValue<bool>(
            m_stack_configuration,
            "/application_layer/startup_state"_json_pointer) ||
        m_getConfigValue<bool>(m_stack_configuration,
                               "/transport_layer/conn_passv"_json_pointer))
    {
        while (!CS104_Connection_connect(connection))
        {
        }

        Logger::getLogger()->info("Connection started");

        // If conn_all = false, only start dt with the first connection
        if (connection_index == 0 ||
            m_getConfigValue<bool>(m_stack_configuration,
                                   "/transport_layer/conn_all"_json_pointer))
            CS104_Connection_sendStartDT(connection);

        m_sendInterrogationCommmands();

        if (m_getConfigValue<bool>(m_stack_configuration,
                                   "/application_layer/time_sync"_json_pointer))
        {
            Logger::getLogger()->info("Sending clock sync command");
            sCP56Time2a currentTime{};
            CP56Time2a_createFromMsTimestamp(&currentTime, Hal_getTimeInMs());
            CS104_Connection_sendClockSyncCommand(
                connection, m_getBroadcastCA(), &currentTime);
        }
    }
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

    m_startup_done = false;
    std::thread startupWatchdog(
        m_watchdog,
        m_getConfigValue<int>(m_stack_configuration,
                              "/application_layer/startup_time"_json_pointer),
        1000, &m_startup_done, "Startup");
    startupWatchdog.detach();
    m_client = new IEC104Client(this, &m_pivot_configuration);

    for (auto& path_element :
         m_stack_configuration["transport_layer"]["connection"]["path"])
    {
        string ip =
            m_getConfigValue<string>(path_element, "/srv_ip"_json_pointer);
        int port = m_getConfigValue<int>(path_element, "/port"_json_pointer);

        CS104_Connection new_connection = nullptr;

        if (m_getConfigValue<bool>(
                m_stack_configuration,
                "/transport_layer/connection/tls"_json_pointer))
        {
            Logger::getLogger()->error("TLS not supported yet");
        }
        else
        {
            new_connection = CS104_Connection_create(ip.c_str(), port);
            Logger::getLogger()->info("Connection created");
        }

        CS104_Connection_setConnectionHandler(
            new_connection, m_connectionHandler, static_cast<void*>(this));
        CS104_Connection_setASDUReceivedHandler(new_connection,
                                                m_asduReceivedHandler,
                                                static_cast<void*>(m_client));

        m_connections.push_back(new_connection);

        connect(m_connections.size() - 1);

        // If conn_all == false, only use the first path
        if (!m_getConfigValue<bool>(m_stack_configuration,
                                    "/transport_layer/conn_all"_json_pointer))
        {
            break;
        }
    }

    m_startup_done = true;

    int gi_cycle = m_getConfigValue<int>(
        m_stack_configuration, "/application_layer/gi_cycle"_json_pointer);
    int gi_time = m_getConfigValue<int>(
        m_stack_configuration, "/application_layer/gi_time"_json_pointer);
    bool exec_cycl_test = m_getConfigValue<bool>(
        m_stack_configuration,
        "/application_layer/exec_cycl_test"_json_pointer);
    // Test commands are sent at same rate as gi, with default of
    //"cycl_test_delay" if gi is not activated
    while (true)
    {
        if (exec_cycl_test) m_sendTestCommmands();

        if (gi_cycle)
        {
            m_sendInterrogationCommmands();
            Thread_sleep(1000 * gi_time);
        }

        Thread_sleep(1000);
    }
}

/** Disconnect from the iec104 servers */
void IEC104::stop()
{
    if (m_client != nullptr)
    {
        delete m_client;
        m_client = nullptr;
    }

    for (auto& connection : m_connections)
    {
        // if the connection has already ended, we don't send another close
        // request
        if (connection)
        {
            CS104_Connection_destroy(connection);
            connection = nullptr;
            Logger::getLogger()->info("Connection stopped");
        }
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
    (*m_ingest)(m_data, Reading(assetName, points));
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

void IEC104::m_sendTestCommmands()
{
    Logger::getLogger()->info("Sending interrogation command...");

    // For every ca
    for (auto& element : m_msg_configuration["asdu_list"])
    {
        int ca = m_getConfigValue<unsigned int>(element, "/ca"_json_pointer);
        for (auto connection : m_connections)
        {
            if (m_comm_wttag)
            {
                uint16_t tsc = 0;  // Test sequence counter
                CP56Time2a currentTime = CP56Time2a_createFromMsTimestamp(
                    nullptr, Hal_getTimeInMs());
                CS104_Connection_sendTestCommandWithTimestamp(connection, ca,
                                                              tsc, currentTime);
            }
            else
                CS104_Connection_sendTestCommand(connection, ca);
        }
    }
    Logger::getLogger()->info("Test command sent");
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

std::string IEC104::m_checkExchangedDataLayer(unsigned int ca,
                                              const string& type_id,
                                              uint64_t ioa)
{
    bool known_ca = false, known_type_id = false;

    // Logger::getLogger()->warn("Checking " + to_string(ca) + " " + type_id + "
    // "
    // + to_string(ioa));
    for (auto& element : m_msg_configuration["asdu_list"])
    {
        if (m_getConfigValue<unsigned int>(element, "/ca"_json_pointer) == ca)
        {
            known_ca = true;
            if (m_getConfigValue<string>(element, "/type_id"_json_pointer) ==
                type_id)
            {
                known_type_id = true;
                if (m_getConfigValue<unsigned int>(element,
                                                   "/ioa"_json_pointer) == ioa)
                    // Logger::getLogger()->warn("Ok for " +
                    // to_string(ca) + " " + type_id
                    // + " " + to_string(ioa));
                    return element["label"];
            }
        }
    }

    if (!known_ca)
        Logger::getLogger()->warn("Unknown CA (" + to_string(ca) +
                                  ") for ASDU");
    else if (!known_type_id)
        Logger::getLogger()->warn("Unknown type_id (" + type_id + ") for ASDU");
    else
        Logger::getLogger()->warn("Unknown IOA (" + to_string(ioa) +
                                  ") for ASDU " + type_id);

    return "";
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

void IEC104Client::sendData(CS101_ASDU asdu, vector<Datapoint*> datapoints,
                            const vector<std::string> labels)
{
    auto* data_header = new vector<Datapoint*>;
    for (auto& feature :
         (*m_pivot_configuration)["mapping"]["data_object_header"].items())
    {
        if (feature.value() == "type_id")
            data_header->push_back(m_createDatapoint(
                feature.key(), (int64_t)CS101_ASDU_getTypeID(asdu)));
        else if (feature.value() == "ca")
            data_header->push_back(m_createDatapoint(
                feature.key(), (int64_t)CS101_ASDU_getCA(asdu)));
        else if (feature.value() == "oa")
            data_header->push_back(m_createDatapoint(
                feature.key(), (int64_t)CS101_ASDU_getOA(asdu)));
        else if (feature.value() == "cot")
            data_header->push_back(m_createDatapoint(
                feature.key(), (int64_t)CS101_ASDU_getCOT(asdu)));
        else if (feature.value() == "istest")
            data_header->push_back(m_createDatapoint(
                feature.key(), (int64_t)CS101_ASDU_isTest(asdu)));
        else if (feature.value() == "isnegative")
            data_header->push_back(m_createDatapoint(
                feature.key(), (int64_t)CS101_ASDU_isNegative(asdu)));
    }

    DatapointValue header_dpv(data_header, true);

    // We send as many pivot format objects as information objects in the
    // source ASDU
    int i = 0;

    for (Datapoint* item_dp : datapoints)
    {
        std::vector<Datapoint*> points;
        points.push_back(new Datapoint("data_object_header", header_dpv));
        points.push_back(item_dp);
        m_iec104->ingest(labels.at(i), points);
        i++;
    }
}

template <class T>
void IEC104Client::m_addData(vector<Datapoint*>& datapoints, int64_t ioa,
                             const std::string& dataname, const T value,
                             QualityDescriptor qd, CP56Time2a ts)
{
    auto* measure_features = new vector<Datapoint*>;

    for (auto& feature :
         (*m_pivot_configuration)["mapping"]["data_object_item"].items())
    {
        if (feature.value() == "ioa")
            measure_features->push_back(m_createDatapoint(feature.key(), ioa));

        else if (feature.value() == "value")
            measure_features->push_back(
                m_createDatapoint(feature.key(), value));

        // else if (feature.value() == "value")
        // {
        //     if (typeid(value) == typeid(bool))
        //     {
        //         measure_features->push_back(m_createDatapoint(
        //             feature.key(), static_cast<long>(value)));
        //     }
        //     else
        //     {
        //         measure_features->push_back(
        //             m_createDatapoint(feature.key(), value));
        //     }
        // }
        else if (feature.value() == "quality_desc")
            measure_features->push_back(
                m_createDatapoint(feature.key(), (int64_t)qd));
        else if (feature.value() == "time_marker")
            measure_features->push_back(m_createDatapoint(
                feature.key(),
                (ts != nullptr ? CP56Time2aToString(ts) : "not_populated")));
        else if (feature.value() == "isinvalid")
            measure_features->push_back(m_createDatapoint(
                feature.key(),
                (ts != nullptr ? (int64_t)CP56Time2a_isInvalid(ts) : -1)));
        else if (feature.value() == "isSummerTime")
            measure_features->push_back(m_createDatapoint(
                feature.key(),
                (ts != nullptr ? (int64_t)CP56Time2a_isSummerTime(ts) : -1)));
        else if (feature.value() == "isSubstituted")
            measure_features->push_back(m_createDatapoint(
                feature.key(),
                (ts != nullptr ? (int64_t)CP56Time2a_isSubstituted(ts) : -1)));
    }

    DatapointValue dpv(measure_features, true);

    datapoints.push_back(new Datapoint("data_object_item", dpv));
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
    for (auto connection : m_connections)
    {
        if (operation.compare("CS104_Connection_sendInterrogationCommand") == 0)
        {
            int casdu = atoi(params[0]->value.c_str());
            CS104_Connection_sendInterrogationCommand(
                connection, CS101_COT_ACTIVATION, casdu, IEC60870_QOI_STATION);
            Logger::getLogger()->info("InterrogationCommand send");
            return true;
        }
        else if (operation.compare(
                     "CS104_Connection_sendTestCommandWithTimestamp") == 0)
        {
            int casdu = atoi(params[0]->value.c_str());
            struct sCP56Time2a testTimestamp;
            CP56Time2a_createFromMsTimestamp(&testTimestamp, Hal_getTimeInMs());
            CS104_Connection_sendTestCommandWithTimestamp(
                connection, casdu, 0x4938, &testTimestamp);
            Logger::getLogger()->info("TestCommandWithTimestamp send");
            return true;
        }
        else if (operation.compare("SingleCommandWithCP56Time2a") == 0)
        {
            // common adress of the asdu
            int casdu = atoi(params[0]->value.c_str());
            // information object adress
            int64_t ioa = atoi(params[1]->value.c_str());
            // command state to send, must be a boolean
            // 0 = off, 1 otherwise
            bool value = static_cast<bool>(atoi(params[2]->value.c_str()));
            struct sCP56Time2a testTimestamp;

            CP56Time2a_createFromMsTimestamp(&testTimestamp, Hal_getTimeInMs());

            InformationObject sc =
                (InformationObject)SingleCommandWithCP56Time2a_create(
                    NULL, ioa, value, Execute, NoAddDefinition, &testTimestamp);

            bool isSent = CS104_Connection_sendProcessCommandEx(
                connection, CS101_COT_ACTIVATION, casdu, sc);

            if (isSent)
                Logger::getLogger()->info("SingleCommandWithCP56Time2a sent");
            else
                Logger::getLogger()->info(
                    "SingleCommandWithCP56Time2a not sent");

            InformationObject_destroy(sc);
            return true;
        }
        else if (operation.compare("DoubleCommandWithCP56Time2a") == 0)
        {
            int casdu = atoi(params[0]->value.c_str());
            int64_t ioa = atoi(params[1]->value.c_str());
            // the command state to send, 4 possible values
            // (0 = not permitted, 1 = off, 2 = on, 3 = not permitted)
            int value = atoi(params[2]->value.c_str());

            struct sCP56Time2a testTimestamp;

            CP56Time2a_createFromMsTimestamp(&testTimestamp, Hal_getTimeInMs());

            InformationObject dc =
                (InformationObject)DoubleCommandWithCP56Time2a_create(
                    NULL, ioa, value, Execute, NoAddDefinition, &testTimestamp);

            bool isSent = CS104_Connection_sendProcessCommandEx(
                connection, CS101_COT_ACTIVATION, casdu, dc);

            if (isSent)
                Logger::getLogger()->info("DoubleCommandWithCP56Time2a sent");
            else
                Logger::getLogger()->info(
                    "DoubleCommandWithCP56Time2a not sent");
            InformationObject_destroy(dc);

            return true;
        }
        else if (operation.compare("StepCommandWithCP56Time2a") == 0)
        {
            int casdu = atoi(params[0]->value.c_str());
            int64_t ioa = atoi(params[1]->value.c_str());
            // the command state to send, 4 possible values
            // (0 = invalid_0, 1 = lower, 2 = higher, 3 = invalid_3)
            StepCommandValue value =
                static_cast<StepCommandValue>(atoi(params[2]->value.c_str()));

            struct sCP56Time2a testTimestamp;

            CP56Time2a_createFromMsTimestamp(&testTimestamp, Hal_getTimeInMs());

            InformationObject stpc =
                (InformationObject)StepCommandWithCP56Time2a_create(
                    NULL, ioa, value, Execute, NoAddDefinition, &testTimestamp);

            bool isSent = CS104_Connection_sendProcessCommandEx(
                connection, CS101_COT_ACTIVATION, casdu, stpc);

            if (isSent)
                Logger::getLogger()->info("StepCommandWithCP56Time2a sent");
            else
                Logger::getLogger()->info("StepCommandWithCP56Time2a not sent");
            InformationObject_destroy(stpc);

            return true;
        }

        Logger::getLogger()->error("Unrecognised operation %s",
                                   operation.c_str());
        return false;
    }
    Logger::getLogger()->error("No current connections");
    return false;
}

void IEC104::sendInterrogationCommmands() { m_sendInterrogationCommmands(); }

void IEC104::sendInterrogationCommmandToCA(unsigned int ca, int gi_repeat_count,
                                           int gi_time)
{
    m_sendInterrogationCommmandToCA(ca, gi_repeat_count, gi_time);
}
