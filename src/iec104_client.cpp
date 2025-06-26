/*
 * Fledge IEC 104 south plugin.
 *
 * Copyright (c) 2022, RTE (https://www.rte-france.com)
 *
 * Released under the Apache 2.0 Licence
 *
 * Author: Michael Zillgith
 */

#include <ctime>
#include <algorithm>
#include <map>

#include <lib60870/hal_time.h>
#include <lib60870/hal_thread.h>

#include "iec104.h"
#include "iec104_client.h"
#include "iec104_client_config.h"
#include "iec104_client_redgroup.h"
#include "iec104_client_connection.h"
#include "iec104_utility.h"

using namespace std;

#define BACKUP_CONNECTION_TIMEOUT 5000 /* 5 seconds */

static uint64_t
getMonotonicTimeInMs()
{
    uint64_t timeVal = 0;

    struct timespec ts;

    if (clock_gettime(CLOCK_MONOTONIC, &ts) == 0) {
        timeVal = ((uint64_t) ts.tv_sec * 1000LL) + (ts.tv_nsec / 1000000);
    }

    return timeVal;
}

static bool isTypeIdSingleSP(IEC60870_5_TypeID typeId) {
    return typeId == M_SP_NA_1 || typeId == M_SP_TA_1 || typeId == M_SP_TB_1;
}

static bool isTypeIdDoubleSP(IEC60870_5_TypeID typeId) {
    return typeId == M_DP_NA_1 || typeId == M_DP_TA_1 || typeId == M_DP_TB_1;
}

static bool isTypeIdSP(IEC60870_5_TypeID typeId) {
     return isTypeIdSingleSP(typeId) || isTypeIdDoubleSP(typeId);
}

template <class T>
Datapoint* IEC104Client::m_createDatapoint(const std::string& dataname, const T value)
{
    DatapointValue dp_value = DatapointValue(value);
    return new Datapoint(dataname, dp_value);
}

Datapoint* IEC104Client::m_createQualityUpdateForDataObject(std::shared_ptr<DataExchangeDefinition> dataDefinition, const QualityDescriptor* qd, CP56Time2a ts)
{
    auto* attributes = new vector<Datapoint*>;

    attributes->push_back(m_createDatapoint("do_type", IEC104ClientConfig::getStringFromTypeID(dataDefinition->typeId)));

    attributes->push_back(m_createDatapoint("do_ca", (long)dataDefinition->ca));

    attributes->push_back(m_createDatapoint("do_oa", (long)0));

    attributes->push_back(m_createDatapoint("do_cot", (long)CS101_COT_SPONTANEOUS));

    attributes->push_back(m_createDatapoint("do_test", (long)0));

    attributes->push_back(m_createDatapoint("do_negative", (long)0));

    attributes->push_back(m_createDatapoint("do_ioa", (long)dataDefinition->ioa));

    if (qd) {
        attributes->push_back(m_createDatapoint("do_quality_iv", (*qd & IEC60870_QUALITY_INVALID) ? 1L : 0L));

        attributes->push_back(m_createDatapoint("do_quality_bl", (*qd & IEC60870_QUALITY_BLOCKED) ? 1L : 0L));

        attributes->push_back(m_createDatapoint("do_quality_ov", (*qd & IEC60870_QUALITY_OVERFLOW) ? 1L : 0L));

        attributes->push_back(m_createDatapoint("do_quality_sb", (*qd & IEC60870_QUALITY_SUBSTITUTED) ? 1L : 0L));

        attributes->push_back(m_createDatapoint("do_quality_nt", (*qd & IEC60870_QUALITY_NON_TOPICAL) ? 1L : 0L));
    }

    if (ts) {
        attributes->push_back(m_createDatapoint("do_ts", (long)CP56Time2a_toMsTimestamp(ts)));

        attributes->push_back(m_createDatapoint("do_ts_iv", (CP56Time2a_isInvalid(ts)) ? 1L : 0L));

        attributes->push_back(m_createDatapoint("do_ts_su", (CP56Time2a_isSummerTime(ts)) ? 1L : 0L));

        attributes->push_back(m_createDatapoint("do_ts_sub", (CP56Time2a_isSubstituted(ts)) ? 1L : 0L));
    }

    DatapointValue dpv(attributes, true);

    return new Datapoint("data_object", dpv);
}

static bool isDataPointInMonitoringDirection(std::shared_ptr<DataExchangeDefinition> dp)
{
    if (dp->typeId < 41) {
        return true;
    }
    else {
        return false;
    }
}

static bool isSupportedCommand(int typeId)
{
    if ((typeId >= 45) && (typeId <= 51))
        return true;

    if ((typeId >= 58) && (typeId <= 64))
        return true;

    return false;
}

std::shared_ptr<IEC104Client::OutstandingCommand> IEC104Client::checkForOutstandingCommand(int typeId, int ca, int ioa, const IEC104ClientConnection* connection)
{
    std::lock_guard<std::mutex> lock(m_outstandingCommandsMtx);

    for (std::shared_ptr<OutstandingCommand> command : m_outstandingCommands) {
        if ((command->typeId == typeId) && (command->ca == ca) && (command->ioa == ioa) && (command->clientCon.get() == connection)) {
            return command;
        }
    }

    return nullptr;
}

void IEC104Client::checkOutstandingCommandTimeouts()
{
    std::string beforeLog = Iec104Utility::PluginName + " - IEC104Client::checkOutstandingCommandTimeouts -";
    std::lock_guard<std::mutex> lock(m_outstandingCommandsMtx);
    uint64_t currentTime = getMonotonicTimeInMs();

    std::vector<std::shared_ptr<OutstandingCommand>> listOfTimedoutCommands;

    for (std::shared_ptr<OutstandingCommand> command : m_outstandingCommands)
    {
        if (command->actConReceived) {
            if (command->timeout + m_config->CmdExecTimeout() < currentTime) {
                Iec104Utility::log_warn("%s ACT-TERM timeout for outstanding command - type: %s (%i) ca: %i ioa: %i", beforeLog.c_str(),
                                        IEC104ClientConfig::getStringFromTypeID(command->typeId).c_str(), command->typeId,
                                        command->ca, command->ioa);
                listOfTimedoutCommands.push_back(command);
            }
        }
        else {
            if (command->timeout + m_config->CmdExecTimeout() < currentTime) {
                Iec104Utility::log_warn("%s ACT-CON timeout for outstanding command - type: %s (%i) ca: %i ioa: %i", beforeLog.c_str(),
                                        IEC104ClientConfig::getStringFromTypeID(command->typeId).c_str(), command->typeId,
                                        command->ca, command->ioa);
                listOfTimedoutCommands.push_back(command);
            }
        }
    }

    // remove all timed out commands from the list of outstanding commands
    for (std::shared_ptr<OutstandingCommand> commandToRemove : listOfTimedoutCommands) {
        // remove object command from m_outstandingCommands
        m_outstandingCommands.erase(std::remove(m_outstandingCommands.begin(), m_outstandingCommands.end(), commandToRemove), m_outstandingCommands.end());
    }
}

void IEC104Client::removeOutstandingCommand(std::shared_ptr<OutstandingCommand> command)
{
    std::lock_guard<std::mutex> lock(m_outstandingCommandsMtx);

    // remove object command from m_outstandingCommands
    m_outstandingCommands.erase(std::remove(m_outstandingCommands.begin(), m_outstandingCommands.end(), command), m_outstandingCommands.end());
}

void IEC104Client::updateQualityForAllDataObjects(QualityDescriptor qd)
{
    vector<Datapoint*> datapoints;
    vector<string> labels;

    for (auto const& exchangeDefintions : m_config->ExchangeDefinition()) {
        for (auto const& dpPair : exchangeDefintions.second) {
            std::shared_ptr<DataExchangeDefinition> dp = dpPair.second;

            if (dp) {
                if (isDataPointInMonitoringDirection(dp))
                {
                    //TODO also add timestamp?

                    Datapoint* qualityUpdateDp = m_createQualityUpdateForDataObject(dp, &qd, nullptr);

                    if (qualityUpdateDp) {
                        datapoints.push_back(qualityUpdateDp);
                        labels.push_back(dp->label);
                    }
                }
            }
        }
    }

    if (datapoints.empty() == false) {
        sendData(datapoints, labels);
    }
}

static bool isInStationGroup(std::shared_ptr<DataExchangeDefinition> dp)
{
    if (dp->giGroups & 1) {
        return true;
    }
    else {
        return false;
    }
}

//LCOV_EXCL_START
void IEC104Client::updateQualityForAllDataObjectsInStationGroup(QualityDescriptor qd)
{
    vector<Datapoint*> datapoints;
    vector<string> labels;

    for (auto const& exchangeDefintions : m_config->ExchangeDefinition()) {
        for (auto const& dpPair : exchangeDefintions.second) {
            std::shared_ptr<DataExchangeDefinition> dp = dpPair.second;

            if (dp && isInStationGroup(dp)) {

                if (isDataPointInMonitoringDirection(dp))
                {
                    Datapoint* qualityUpdateDp = m_createQualityUpdateForDataObject(dp, &qd, nullptr);

                    if (qualityUpdateDp) {
                        datapoints.push_back(qualityUpdateDp);
                        labels.push_back(dp->label);
                    }
                }
            }
        }
    }

    if (datapoints.empty() == false) {
        sendData(datapoints, labels);
    }
}
//LCOV_EXCL_STOP

void IEC104Client::updateQualityForDataObjectsNotReceivedInGIResponse(QualityDescriptor qd)
{
    vector<Datapoint*> datapoints;
    vector<string> labels;

    for (auto dp : m_listOfStationGroupDatapoints) {
        Datapoint* qualityUpdateDp = m_createQualityUpdateForDataObject(dp, &qd, nullptr);

        if (qualityUpdateDp) {
            datapoints.push_back(qualityUpdateDp);
            labels.push_back(dp->label);
        }
    }

    if (datapoints.empty() == false) {
        sendData(datapoints, labels);
    }
}

void IEC104Client::removeFromListOfDatapoints(std::shared_ptr<DataExchangeDefinition> toRemove)
{
    auto& list = m_listOfStationGroupDatapoints;
    list.erase(std::remove(list.begin(), list.end(), toRemove), list.end());
}

void IEC104Client::createListOfDatapointsInStationGroup()
{
    m_listOfStationGroupDatapoints.clear();

    for (auto const& exchangeDefintions : m_config->ExchangeDefinition()) {
        for (auto const& dpPair : exchangeDefintions.second) {
            std::shared_ptr<DataExchangeDefinition> dp = dpPair.second;

            if (dp && isInStationGroup(dp)) {
                m_listOfStationGroupDatapoints.push_back(dp);
            }
        }
    }
}

template <class T>
Datapoint* IEC104Client::m_createDataObject(CS101_ASDU asdu, int64_t ioa, const std::string& dataname, const T value,
    QualityDescriptor* qd, CP56Time2a ts)
{
    auto* attributes = new vector<Datapoint*>;

    attributes->push_back(m_createDatapoint("do_type", IEC104ClientConfig::getStringFromTypeID(CS101_ASDU_getTypeID(asdu))));

    attributes->push_back(m_createDatapoint("do_ca", (long)CS101_ASDU_getCA(asdu)));

    attributes->push_back(m_createDatapoint("do_oa", (long)CS101_ASDU_getOA(asdu)));

    attributes->push_back(m_createDatapoint("do_cot", (long)CS101_ASDU_getCOT(asdu)));

    attributes->push_back(m_createDatapoint("do_test", (long)CS101_ASDU_isTest(asdu)));

    attributes->push_back(m_createDatapoint("do_negative", (long)CS101_ASDU_isNegative(asdu)));

    attributes->push_back(m_createDatapoint("do_ioa", (long)ioa));

    attributes->push_back(m_createDatapoint("do_value", value));

    if (qd) {
        attributes->push_back(m_createDatapoint("do_quality_iv", (*qd & IEC60870_QUALITY_INVALID) ? 1L : 0L));

        attributes->push_back(m_createDatapoint("do_quality_bl", (*qd & IEC60870_QUALITY_BLOCKED) ? 1L : 0L));

        attributes->push_back(m_createDatapoint("do_quality_ov", (*qd & IEC60870_QUALITY_OVERFLOW) ? 1L : 0L));

        attributes->push_back(m_createDatapoint("do_quality_sb", (*qd & IEC60870_QUALITY_SUBSTITUTED) ? 1L : 0L));

        attributes->push_back(m_createDatapoint("do_quality_nt", (*qd & IEC60870_QUALITY_NON_TOPICAL) ? 1L : 0L));
    }

    if (ts) {
         attributes->push_back(m_createDatapoint("do_ts", (long)CP56Time2a_toMsTimestamp(ts)));

         attributes->push_back(m_createDatapoint("do_ts_iv", (CP56Time2a_isInvalid(ts)) ? 1L : 0L));

         attributes->push_back(m_createDatapoint("do_ts_su", (CP56Time2a_isSummerTime(ts)) ? 1L : 0L));

         attributes->push_back(m_createDatapoint("do_ts_sub", (CP56Time2a_isSubstituted(ts)) ? 1L : 0L));
    }

    DatapointValue dpv(attributes, true);

    return new Datapoint("data_object", dpv);
}

void
IEC104Client::sendData(vector<Datapoint*> datapoints,
                            const vector<std::string> labels)
{
    int i = 0;

    for (Datapoint* item_dp : datapoints)
    {
        std::vector<Datapoint*> points;
        points.push_back(item_dp);

        m_iec104->ingest(labels.at(i), points);
        i++;
    }
}

IEC104Client::OutstandingCommand::OutstandingCommand(int typeId, int ca, int ioa, std::shared_ptr<IEC104ClientConnection> con):
    typeId(typeId), ca(ca), ioa(ioa), clientCon(con)
{
    std::string beforeLog = Iec104Utility::PluginName + " - IEC104Client::OutstandingCommand::OutstandingCommand -";
    timeout = getMonotonicTimeInMs();
    Iec104Utility::log_debug("%s Created outstanding command: typeId=%s, CA=%d, IOA=%d, timeout=%d", beforeLog.c_str(),
                            IEC104ClientConfig::getStringFromTypeID(typeId).c_str(), ca, ioa, timeout);
}

void
IEC104Client::sendSouthMonitoringEvent(bool connxStatus, bool giStatus)
{
    std::string beforeLog = Iec104Utility::PluginName + " - IEC104Client::sendSouthMonitoringEvent -";
    if (m_config == nullptr) {
        Iec104Utility::log_error("%s Cannot send south event: Config is not defined", beforeLog.c_str());
        return;
    }

    if (m_config->GetConnxStatusSignal().empty()) {
        Iec104Utility::log_warn("%s Cannot send south event: Connexion status signal is not defined", beforeLog.c_str());
        return;
    }

    if ((connxStatus == false) && (giStatus == false)) {
        Iec104Utility::log_debug("%s No data requested for south event", beforeLog.c_str());
        return;
    }

    auto* attributes = new vector<Datapoint*>;

    if (connxStatus) {
        Datapoint* eventDp = nullptr;

        switch (m_connStatus)
        {
            case ConnectionStatus::NOT_CONNECTED:
                eventDp = m_createDatapoint("connx_status", "not connected");
                break;

            case ConnectionStatus::STARTED:
                eventDp = m_createDatapoint("connx_status", "started");
                break;
        }

        if (eventDp) {
            attributes->push_back(eventDp);
        }
    }

    if (giStatus) {
        Datapoint* eventDp = nullptr;

        switch(m_giStatus)
        {
            case GiStatus::STARTED:
                eventDp = m_createDatapoint("gi_status", "started");
                break;

            case GiStatus::IN_PROGRESS:
                eventDp = m_createDatapoint("gi_status", "in progress");
                break;

            case GiStatus::FAILED:
                eventDp = m_createDatapoint("gi_status", "failed");
                break;

            case GiStatus::FINISHED:
                eventDp = m_createDatapoint("gi_status", "finished");
                break;

            case GiStatus::IDLE:
                eventDp = m_createDatapoint("gi_status", "idle");
                break;
        }

        if (eventDp) {
            attributes->push_back(eventDp);
        }
    }

    DatapointValue dpv(attributes, true);

    Datapoint* southEvent = new Datapoint("south_event", dpv);

    vector<Datapoint*> datapoints;
    vector<string> labels;

    datapoints.push_back(southEvent);

    labels.push_back(m_config->GetConnxStatusSignal());

    sendData(datapoints, labels);
}

void
IEC104Client::updateConnectionStatus(ConnectionStatus newState)
{
    if (m_connStatus == newState)
        return;

    m_connStatus = newState;

    sendSouthMonitoringEvent(true, false);

    // Send audit for connection status
    if (m_connStatus == ConnectionStatus::STARTED) {
        Iec104Utility::audit_success("SRVFL", m_iec104->getServiceName() + "-connected");
    }
    else {
        Iec104Utility::audit_fail("SRVFL", m_iec104->getServiceName() + "-disconnected");
    }
}

void
IEC104Client::updateGiStatus(GiStatus newState)
{
    if (m_giStatus == newState)
        return;

    m_giStatus = newState;

    sendSouthMonitoringEvent(false, true);

    #ifdef UNIT_TEST
        // Simulated longer GI
        if(newState == GiStatus::STARTED) {
            Thread_sleep(200);
        }
    #endif
}

IEC104Client::GiStatus
IEC104Client::getGiStatus()
{
    return m_giStatus;
}

IEC104Client::IEC104Client(IEC104* iec104, std::shared_ptr<IEC104ClientConfig> config)
        : m_iec104(iec104),
          m_config(config)
{
}

IEC104Client::~IEC104Client()
{
    stop();
}

static bool
isInterrogationResponse(CS101_ASDU asdu)
{
    if (CS101_ASDU_getCOT(asdu) == CS101_COT_INTERROGATED_BY_STATION)
        return true;
    else
        return false;
}

bool
IEC104Client::handleASDU(const IEC104ClientConnection* connection, CS101_ASDU asdu)
{
    std::string beforeLog = Iec104Utility::PluginName + " - IEC104Client::handleASDU -";
    bool handledAsdu = true;

    vector<Datapoint*> datapoints;
    vector<string> labels;

    IEC60870_5_TypeID typeId = CS101_ASDU_getTypeID(asdu);
    int ca = CS101_ASDU_getCA(asdu);

    bool isResponse = isInterrogationResponse(asdu);
    if (isResponse) {
        if (getGiStatus() == GiStatus::STARTED)
            updateGiStatus(GiStatus::IN_PROGRESS);
    }

    Iec104Utility::log_debug("%s Received ASDU with CA: %i, interrogation response: %s", beforeLog.c_str(), ca, isResponse?"true":"false");

    for (int i = 0; i < CS101_ASDU_getNumberOfElements(asdu); i++)
    {
        InformationObject io = CS101_ASDU_getElement(asdu, i);

        if (io)
        {
            int ioa = InformationObject_getObjectAddress(io);

            std::string* label = m_config->checkExchangeDataLayer(typeId, ca, ioa);

            std::shared_ptr<OutstandingCommand> outstandingCommand;

            if (isSupportedCommand(typeId)) {
                outstandingCommand = checkForOutstandingCommand(typeId, ca, ioa, connection);
                Iec104Utility::log_debug("%s Found supported command type: %s (%d) (CA: %i IOA: %i)", beforeLog.c_str(),
                                        IEC104ClientConfig::getStringFromTypeID(typeId).c_str(), typeId, ca, ioa);
            }

            if ((label != nullptr) && isResponse) {
                std::shared_ptr<DataExchangeDefinition> exgDef = m_config->ExchangeDefinition()[ca][ioa];

                if (exgDef) {
                    if (isInStationGroup(exgDef)) {
                        removeFromListOfDatapoints(exgDef);
                        Iec104Utility::log_debug("%s Removed station group datapoint for type %s (%d) with CA: %i IOA: %i", beforeLog.c_str(),
                                                IEC104ClientConfig::getStringFromTypeID(typeId).c_str(), typeId, label->c_str(), ca, ioa);
                    }
                }
            }

            switch (typeId)
            {
                case M_ME_NB_1:
                    if (label)
                        handle_M_ME_NB_1(datapoints, *label, ca, asdu, io, ioa);
                    break;

                case M_SP_NA_1:
                    if (label)
                        handle_M_SP_NA_1(datapoints, *label, ca, asdu, io, ioa);
                    break;

                case M_SP_TB_1:
                    if (label)
                        handle_M_SP_TB_1(datapoints, *label, ca, asdu, io, ioa);
                    break;

                case M_DP_NA_1:
                    if (label)
                        handle_M_DP_NA_1(datapoints, *label, ca, asdu, io, ioa);
                    break;

                case M_DP_TB_1:
                    if (label)
                        handle_M_DP_TB_1(datapoints, *label, ca, asdu, io, ioa);
                    break;

                case M_ST_NA_1:
                    if (label)
                        handle_M_ST_NA_1(datapoints, *label, ca, asdu, io, ioa);
                    break;

                case M_ST_TB_1:
                    if (label)
                        handle_M_ST_TB_1(datapoints, *label, ca, asdu, io, ioa);
                    break;

                case M_ME_NA_1:
                    if (label)
                        handle_M_ME_NA_1(datapoints, *label, ca, asdu, io, ioa);
                    break;

                case M_ME_TD_1:
                    if (label)
                        handle_M_ME_TD_1(datapoints, *label, ca, asdu, io, ioa);
                    break;

                case M_ME_TE_1:
                    if (label)
                        handle_M_ME_TE_1(datapoints, *label, ca, asdu, io, ioa);
                    break;

                case M_ME_NC_1:
                    if (label)
                        handle_M_ME_NC_1(datapoints, *label, ca, asdu, io, ioa);
                    break;

                case M_ME_TF_1:
                    if (label)
                        handle_M_ME_TF_1(datapoints, *label, ca, asdu, io, ioa);
                    break;

                case C_SC_NA_1:
                case C_SC_TA_1:
                    if (label)
                        handle_C_SC_NA_1(datapoints, *label, ca, asdu, io, ioa, outstandingCommand);
                    break;

                case C_DC_TA_1:
                case C_DC_NA_1:
                    if (label)
                        handle_C_DC_NA_1(datapoints, *label, ca, asdu, io, ioa, outstandingCommand);
                    break;

                case C_RC_NA_1:
                case C_RC_TA_1:
                    if (label)
                        handle_C_RC_NA_1(datapoints, *label, ca, asdu, io, ioa, outstandingCommand);
                    break;

                case C_SE_NA_1:
                case C_SE_TA_1:
                    if (label)
                        handle_C_SE_NA_1(datapoints, *label, ca, asdu, io, ioa, outstandingCommand);
                    break;

                case C_SE_NB_1:
                case C_SE_TB_1:
                    if (label)
                        handle_C_SE_NB_1(datapoints, *label, ca, asdu, io, ioa, outstandingCommand);
                    break;

                case C_SE_NC_1:
                case C_SE_TC_1:
                    if (label)
                        handle_C_SE_NC_1(datapoints, *label, ca, asdu, io, ioa, outstandingCommand);
                    break;

                default:
                    handledAsdu = false;
                    break;
            }

            if (label) {
                if (handledAsdu) {
                    labels.push_back(*label);
                    Iec104Utility::log_info("%s Created data object for ASDU of type %s (%d) with CA: %i IOA: %i", beforeLog.c_str(),
                                            IEC104ClientConfig::getStringFromTypeID(typeId).c_str(), typeId, ca, ioa);
                    if(isAsduTriggerGi(datapoints, ca, asdu, ioa, typeId)){
                        m_activeConnection->setGiRequested(true);
                    }
                }
                else {
                    Iec104Utility::log_warn("%s ASDU type %s (%d) not supported for %s (CA: %i IOA: %i)", beforeLog.c_str(),
                                            IEC104ClientConfig::getStringFromTypeID(typeId).c_str(), typeId, label->c_str(), ca, ioa);
                }
            }
            else {
                if (handledAsdu) {
                    Iec104Utility::log_debug("%s No data point found in exchange configuration for type %s (%d) with CA: %i IOA: %i",
                                            beforeLog.c_str(), IEC104ClientConfig::getStringFromTypeID(typeId).c_str(), typeId, ca, ioa);
                }
            }

            InformationObject_destroy(io);
        }
        else {
            Iec104Utility::log_error("%s Received ASDU with invalid or unknown information object for type %s (%d) CA: %i", beforeLog.c_str(),
                                    IEC104ClientConfig::getStringFromTypeID(typeId).c_str(), typeId, ca);
        }
    }

    if (labels.empty() == false)
    {
        sendData(datapoints, labels);
    }

    return handledAsdu;
}

bool IEC104Client::isAsduTriggerGi(vector<Datapoint*>& datapoints,
                            unsigned int ca,
                            CS101_ASDU asdu,
                            uint64_t ioa,
                            IEC60870_5_TypeID typeId) {
    std::string beforeLog = Iec104Utility::PluginName + " - IEC104Client::isAsduTriggerGi -";
    if (m_config->isTsAddressCgTriggering(ca, ioa) && isTypeIdSP(typeId) && (CS101_ASDU_getCOT(asdu) != CS101_CauseOfTransmission::CS101_COT_INTERROGATED_BY_STATION)) {
        int valueTriggering = isTypeIdSingleSP(typeId) ? 0 : 1; // if it is a simple TS 0 is 0 if it is a double 0 is 1 because 01 is 0, 10 is 1, 11 is transient
        for (auto datapoint : *(datapoints.back()->getData().getDpVec())) {
            if (datapoint->getName() == "do_value" && datapoint->getData().toInt() == valueTriggering) {
                if (m_activeConnection.get() == nullptr) {
                    Iec104Utility::log_info("%s No active connexion, skip GI request.", beforeLog.c_str());
                    return false;
                }
                if(!m_activeConnection->getGiRequested()){
                    return true;
                }
            }
        }
    }
    return false;
}

// Each of the following function handle a specific type of ASDU. They cast the
// contained IO into a specific object that is strictly linked to the type
// for example a MeasuredValueScaled is type M_ME_NB_1
void IEC104Client::handle_M_ME_NB_1(vector<Datapoint*>& datapoints, string& label,
                             unsigned int ca,
                             CS101_ASDU asdu, InformationObject io,
                             uint64_t ioa)
{
    auto io_casted = (MeasuredValueScaled)io;
    int64_t value = MeasuredValueScaled_getValue((MeasuredValueScaled)io_casted);
    QualityDescriptor qd = MeasuredValueScaled_getQuality(io_casted);

    datapoints.push_back(m_createDataObject(asdu, ioa, label, value, &qd));
}

void IEC104Client::handle_M_SP_NA_1(vector<Datapoint*>& datapoints, string& label,
                             unsigned int ca,
                             CS101_ASDU asdu, InformationObject io,
                             uint64_t ioa)
{
    auto io_casted = (SinglePointInformation)io;
    int64_t value = SinglePointInformation_getValue((SinglePointInformation)io_casted);
    QualityDescriptor qd = SinglePointInformation_getQuality((SinglePointInformation)io_casted);

    datapoints.push_back(m_createDataObject(asdu, ioa, label, value, &qd));
}

void IEC104Client::handle_M_SP_TB_1(vector<Datapoint*>& datapoints, string& label,
                             unsigned int ca,
                             CS101_ASDU asdu, InformationObject io,
                             uint64_t ioa)
{
    auto io_casted = (SinglePointWithCP56Time2a)io;
    int64_t value = SinglePointInformation_getValue((SinglePointInformation)io_casted);
    QualityDescriptor qd = SinglePointInformation_getQuality((SinglePointInformation)io_casted);

    CP56Time2a ts = SinglePointWithCP56Time2a_getTimestamp(io_casted);

    datapoints.push_back(m_createDataObject(asdu, ioa, label, value, &qd, ts));
}

void IEC104Client::handle_M_DP_NA_1(vector<Datapoint*>& datapoints, string& label,
                             unsigned int ca,
                             CS101_ASDU asdu, InformationObject io,
                             uint64_t ioa)
{
    auto io_casted = (DoublePointInformation)io;
    int64_t value = DoublePointInformation_getValue((DoublePointInformation)io_casted);
    QualityDescriptor qd = DoublePointInformation_getQuality((DoublePointInformation)io_casted);

    datapoints.push_back(m_createDataObject(asdu, ioa, label, value, &qd));
}

void IEC104Client::handle_M_DP_TB_1(vector<Datapoint*>& datapoints, string& label,
                             unsigned int ca,
                             CS101_ASDU asdu, InformationObject io,
                             uint64_t ioa)
{
    auto io_casted = (DoublePointWithCP56Time2a)io;
    int64_t value = DoublePointInformation_getValue((DoublePointInformation)io_casted);
    QualityDescriptor qd = DoublePointInformation_getQuality((DoublePointInformation)io_casted);

    CP56Time2a ts = DoublePointWithCP56Time2a_getTimestamp(io_casted);
    bool is_invalid = CP56Time2a_isInvalid(ts);

    datapoints.push_back(m_createDataObject(asdu, ioa, label, value, &qd, ts));
}

void IEC104Client::handle_M_ST_NA_1(vector<Datapoint*>& datapoints, string& label,
                             unsigned int ca,
                             CS101_ASDU asdu, InformationObject io,
                             uint64_t ioa)
{
    auto io_casted = (StepPositionInformation)io;
    int64_t posValue = StepPositionInformation_getValue((StepPositionInformation)io_casted);
    bool transient = StepPositionInformation_isTransient((StepPositionInformation)io_casted);
    std::string value = "[" + std::to_string(posValue) + "," + (transient ? "true" : "false") + "]";
    QualityDescriptor qd = StepPositionInformation_getQuality((StepPositionInformation)io_casted);

    datapoints.push_back(m_createDataObject(asdu, ioa, label, value, &qd));
}

void IEC104Client::handle_M_ST_TB_1(vector<Datapoint*>& datapoints, string& label,
                             unsigned int ca,
                             CS101_ASDU asdu, InformationObject io,
                             uint64_t ioa)
{
    auto io_casted = (StepPositionWithCP56Time2a)io;
    int64_t posValue = StepPositionInformation_getValue((StepPositionInformation)io_casted);
    bool transient = StepPositionInformation_isTransient((StepPositionInformation)io_casted);
    std::string value = "[" + std::to_string(posValue) + "," + (transient ? "true" : "false") + "]";
    QualityDescriptor qd = StepPositionInformation_getQuality((StepPositionInformation)io_casted);

    CP56Time2a ts = StepPositionWithCP56Time2a_getTimestamp(io_casted);
    bool is_invalid = CP56Time2a_isInvalid(ts);

    datapoints.push_back(m_createDataObject(asdu, ioa, label, value, &qd, ts));
}

void IEC104Client::handle_M_ME_NA_1(vector<Datapoint*>& datapoints, string& label,
                             unsigned int ca,
                             CS101_ASDU asdu, InformationObject io,
                             uint64_t ioa)
{
    auto io_casted = (MeasuredValueNormalized)io;
    float value = MeasuredValueNormalized_getValue((MeasuredValueNormalized)io_casted);
    QualityDescriptor qd = MeasuredValueNormalized_getQuality((MeasuredValueNormalized)io_casted);

    datapoints.push_back(m_createDataObject(asdu, ioa, label, value, &qd));
}

void IEC104Client::handle_M_ME_TD_1(vector<Datapoint*>& datapoints, string& label,
                             unsigned int ca,
                             CS101_ASDU asdu, InformationObject io,
                             uint64_t ioa)
{
    auto io_casted = (MeasuredValueNormalizedWithCP56Time2a)io;
    float value = MeasuredValueNormalized_getValue((MeasuredValueNormalized)io_casted);
    QualityDescriptor qd = MeasuredValueNormalized_getQuality((MeasuredValueNormalized)io_casted);

    CP56Time2a ts = MeasuredValueNormalizedWithCP56Time2a_getTimestamp(io_casted);
    bool is_invalid = CP56Time2a_isInvalid(ts);

    datapoints.push_back(m_createDataObject(asdu, ioa, label, value, &qd, ts));
}

void IEC104Client::handle_M_ME_TE_1(vector<Datapoint*>& datapoints, string& label,
                             unsigned int ca,
                             CS101_ASDU asdu, InformationObject io,
                             uint64_t ioa)
{
    auto io_casted = (MeasuredValueScaledWithCP56Time2a)io;
    int64_t value = MeasuredValueScaled_getValue((MeasuredValueScaled)io_casted);
    QualityDescriptor qd = MeasuredValueScaled_getQuality((MeasuredValueScaled)io_casted);

    CP56Time2a ts = MeasuredValueScaledWithCP56Time2a_getTimestamp(io_casted);
    bool is_invalid = CP56Time2a_isInvalid(ts);

    datapoints.push_back(m_createDataObject(asdu, ioa, label, value, &qd, ts));
}

void IEC104Client::handle_M_ME_NC_1(vector<Datapoint*>& datapoints, string& label,
                             unsigned int ca,
                             CS101_ASDU asdu, InformationObject io,
                             uint64_t ioa)
{
    auto io_casted = (MeasuredValueShort)io;
    float value = MeasuredValueShort_getValue((MeasuredValueShort)io_casted);
    QualityDescriptor qd = MeasuredValueShort_getQuality((MeasuredValueShort)io_casted);

    datapoints.push_back(m_createDataObject(asdu, ioa, label, value, &qd));
}

void IEC104Client::handle_M_ME_TF_1(vector<Datapoint*>& datapoints, string& label,
                             unsigned int ca,
                             CS101_ASDU asdu, InformationObject io,
                             uint64_t ioa)
{
    auto io_casted = (MeasuredValueShortWithCP56Time2a)io;
    float value = MeasuredValueShort_getValue((MeasuredValueShort)io_casted);
    QualityDescriptor qd = MeasuredValueShort_getQuality((MeasuredValueShort)io_casted);

    CP56Time2a ts = MeasuredValueShortWithCP56Time2a_getTimestamp(io_casted);
    bool is_invalid = CP56Time2a_isInvalid(ts);

    datapoints.push_back(m_createDataObject(asdu, ioa, label, value, &qd, ts));
}

void IEC104Client::handle_C_SC_NA_1(vector<Datapoint*>& datapoints, string& label,
                             unsigned int ca,
                             CS101_ASDU asdu, InformationObject io,
                             uint64_t ioa,
                             std::shared_ptr<OutstandingCommand> outstandingCommand)
{
    std::string beforeLog = Iec104Utility::PluginName + " - IEC104Client::handle_C_SC_NA_1 -";
    auto io_casted = (SingleCommandWithCP56Time2a)io;
    int64_t state = SingleCommand_getState((SingleCommand)io_casted);

    QualifierOfCommand qu = SingleCommand_getQU((SingleCommand)io_casted);

    auto typeId = CS101_ASDU_getTypeID(asdu);
    auto cot = CS101_ASDU_getCOT(asdu);
    if (outstandingCommand) {
        if (cot == CS101_COT_ACTIVATION_CON) {
            Iec104Utility::log_debug("%s Received ACT-CON for %s (%d) COT: %s (%d)", beforeLog.c_str(),
                                    IEC104ClientConfig::getStringFromTypeID(typeId).c_str(), typeId,
                                    CS101_CauseOfTransmission_toString(cot), cot);
            outstandingCommand->actConReceived = true;
            outstandingCommand->timeout = getMonotonicTimeInMs();
        }
        else if (cot == CS101_COT_ACTIVATION_TERMINATION) {
            Iec104Utility::log_debug("%s Received ACT-TERM for %s (%d) COT: %s (%d)", beforeLog.c_str(),
                                    IEC104ClientConfig::getStringFromTypeID(typeId).c_str(), typeId,
                                    CS101_CauseOfTransmission_toString(cot), cot);
            removeOutstandingCommand(outstandingCommand);
        }
    }

    if (typeId == C_SC_TA_1)
    {
        CP56Time2a ts = SingleCommandWithCP56Time2a_getTimestamp(io_casted);
        bool is_invalid = CP56Time2a_isInvalid(ts);

        datapoints.push_back(m_createDataObject(asdu, ioa, label, state, nullptr, ts));
    }
    else {
        datapoints.push_back(m_createDataObject(asdu, ioa, label, state, nullptr));
    }
}

void IEC104Client::handle_C_DC_NA_1(vector<Datapoint*>& datapoints, string& label,
                             unsigned int ca,
                             CS101_ASDU asdu, InformationObject io,
                             uint64_t ioa,
                             std::shared_ptr<OutstandingCommand> outstandingCommand)
{
    std::string beforeLog = Iec104Utility::PluginName + " - IEC104Client::handle_C_DC_NA_1 -";
    auto io_casted = (DoubleCommandWithCP56Time2a)io;
    int64_t state = DoubleCommand_getState((DoubleCommand)io_casted);

    QualifierOfCommand qu = DoubleCommand_getQU((DoubleCommand)io_casted);

    auto typeId = CS101_ASDU_getTypeID(asdu);
    auto cot = CS101_ASDU_getCOT(asdu);
    if (outstandingCommand) {
        if (cot == CS101_COT_ACTIVATION_CON) {
            Iec104Utility::log_debug("%s Received ACT-CON for %s (%d) COT: %s (%d)", beforeLog.c_str(),
                                    IEC104ClientConfig::getStringFromTypeID(typeId).c_str(), typeId,
                                    CS101_CauseOfTransmission_toString(cot), cot);
            outstandingCommand->actConReceived = true;
            outstandingCommand->timeout = getMonotonicTimeInMs();
        }
        else if (cot == CS101_COT_ACTIVATION_TERMINATION) {
            Iec104Utility::log_debug("%s Received ACT-TERM for %s (%d) COT: %s (%d)", beforeLog.c_str(),
                                    IEC104ClientConfig::getStringFromTypeID(typeId).c_str(), typeId,
                                    CS101_CauseOfTransmission_toString(cot), cot);
            removeOutstandingCommand(outstandingCommand);
        }
    }

    if (typeId == C_DC_TA_1)
    {
        CP56Time2a ts = DoubleCommandWithCP56Time2a_getTimestamp(io_casted);

        datapoints.push_back(m_createDataObject(asdu, ioa, label, state, nullptr, ts));
    }
    else
        datapoints.push_back(m_createDataObject(asdu, ioa, label, state, nullptr));
}

void IEC104Client::handle_C_RC_NA_1(vector<Datapoint*>& datapoints, string& label,
                             unsigned int ca,
                             CS101_ASDU asdu, InformationObject io,
                             uint64_t ioa,
                             std::shared_ptr<OutstandingCommand> outstandingCommand)
{
    std::string beforeLog = Iec104Utility::PluginName + " - IEC104Client::handle_C_RC_NA_1 -";
    auto io_casted = (StepCommandWithCP56Time2a)io;
    int64_t state = StepCommand_getState((StepCommand)io_casted);

    QualifierOfCommand qu = StepCommand_getQU((StepCommand)io_casted);

    auto typeId = CS101_ASDU_getTypeID(asdu);
    auto cot = CS101_ASDU_getCOT(asdu);
    if (outstandingCommand) {
        if (cot == CS101_COT_ACTIVATION_CON) {
            Iec104Utility::log_debug("%s Received ACT-CON for %s (%d) COT: %s (%d)", beforeLog.c_str(),
                        IEC104ClientConfig::getStringFromTypeID(typeId).c_str(), typeId,
                        CS101_CauseOfTransmission_toString(cot), cot);
            outstandingCommand->actConReceived = true;
            outstandingCommand->timeout = getMonotonicTimeInMs();
        }
        else if (cot == CS101_COT_ACTIVATION_TERMINATION) {
            Iec104Utility::log_debug("%s Received ACT-TERM for %s (%d) COT: %s (%d)", beforeLog.c_str(),
                                    IEC104ClientConfig::getStringFromTypeID(typeId).c_str(), typeId,
                                    CS101_CauseOfTransmission_toString(cot), cot);
            removeOutstandingCommand(outstandingCommand);
        }
    }

    if (typeId == C_RC_TA_1)
    {
        CP56Time2a ts = StepCommandWithCP56Time2a_getTimestamp(io_casted);

        datapoints.push_back(m_createDataObject(asdu, ioa, label, state, nullptr, ts));
    }
    else
        datapoints.push_back(m_createDataObject(asdu, ioa, label, state, nullptr));
}

void IEC104Client::handle_C_SE_NA_1(vector<Datapoint*>& datapoints, string& label,
                             unsigned int ca,
                             CS101_ASDU asdu, InformationObject io,
                             uint64_t ioa,
                             std::shared_ptr<OutstandingCommand> outstandingCommand)
{
    std::string beforeLog = Iec104Utility::PluginName + " - IEC104Client::handle_C_SE_NA_1 -";
    auto io_casted = (SetpointCommandNormalizedWithCP56Time2a)io;

    float value = SetpointCommandNormalized_getValue((SetpointCommandNormalized)io_casted);

    QualifierOfCommand ql = SetpointCommandNormalized_getQL((SetpointCommandNormalized)io_casted);

    auto typeId = CS101_ASDU_getTypeID(asdu);
    auto cot = CS101_ASDU_getCOT(asdu);
    if (outstandingCommand) {
        if (cot == CS101_COT_ACTIVATION_CON) {
            Iec104Utility::log_debug("%s Received ACT-CON for %s (%d) COT: %s (%d)", beforeLog.c_str(),
                        IEC104ClientConfig::getStringFromTypeID(typeId).c_str(), typeId,
                        CS101_CauseOfTransmission_toString(cot), cot);
            removeOutstandingCommand(outstandingCommand);
        }
    }

    if (typeId == C_SE_TA_1)
    {
        CP56Time2a ts = SetpointCommandNormalizedWithCP56Time2a_getTimestamp(io_casted);

        datapoints.push_back(m_createDataObject(asdu, ioa, label, value, nullptr, ts));
    }
    else
        datapoints.push_back(m_createDataObject(asdu, ioa, label, value, nullptr));
}

void IEC104Client::handle_C_SE_NB_1(vector<Datapoint*>& datapoints, string& label,
                             unsigned int ca,
                             CS101_ASDU asdu, InformationObject io,
                             uint64_t ioa,
                             std::shared_ptr<OutstandingCommand> outstandingCommand)
{
    std::string beforeLog = Iec104Utility::PluginName + " - IEC104Client::handle_C_SE_NB_1 -";
    auto io_casted = (SetpointCommandScaledWithCP56Time2a)io;

    int64_t value = SetpointCommandScaled_getValue((SetpointCommandScaled)io_casted);

    QualifierOfCommand ql = SetpointCommandScaled_getQL((SetpointCommandScaled)io_casted);

    auto typeId = CS101_ASDU_getTypeID(asdu);
    auto cot = CS101_ASDU_getCOT(asdu);
    if (outstandingCommand) {
        if (cot == CS101_COT_ACTIVATION_CON) {
            Iec104Utility::log_debug("%s Received ACT-CON for %s (%d) COT: %s (%d)", beforeLog.c_str(),
                        IEC104ClientConfig::getStringFromTypeID(typeId).c_str(), typeId,
                        CS101_CauseOfTransmission_toString(cot), cot);
            removeOutstandingCommand(outstandingCommand);
        }
    }

    if (typeId == C_SE_TB_1)
    {
        CP56Time2a ts = SetpointCommandScaledWithCP56Time2a_getTimestamp(io_casted);

        datapoints.push_back(m_createDataObject(asdu, ioa, label, value, nullptr, ts));
    }
    else
        datapoints.push_back(m_createDataObject(asdu, ioa, label, value, nullptr));
}

void IEC104Client::handle_C_SE_NC_1(vector<Datapoint*>& datapoints, string& label,
                             unsigned int ca,
                             CS101_ASDU asdu, InformationObject io,
                             uint64_t ioa,
                             std::shared_ptr<OutstandingCommand> outstandingCommand)
{
    std::string beforeLog = Iec104Utility::PluginName + " - IEC104Client::handle_C_SE_NC_1 -";
    auto io_casted = (SetpointCommandShortWithCP56Time2a)io;

    float value = SetpointCommandShort_getValue((SetpointCommandShort)io_casted);

    QualifierOfCommand ql = SetpointCommandShort_getQL((SetpointCommandShort)io_casted);

    auto typeId = CS101_ASDU_getTypeID(asdu);
    auto cot = CS101_ASDU_getCOT(asdu);
    if (outstandingCommand) {
        if (cot == CS101_COT_ACTIVATION_CON) {
            Iec104Utility::log_debug("%s Received ACT-CON for %s (%d) COT: %s (%d)", beforeLog.c_str(),
                        IEC104ClientConfig::getStringFromTypeID(typeId).c_str(), typeId,
                        CS101_CauseOfTransmission_toString(cot), cot);
            removeOutstandingCommand(outstandingCommand);
        }
    }

    if (typeId == C_SE_TC_1)
    {
        CP56Time2a ts = SetpointCommandShortWithCP56Time2a_getTimestamp(io_casted);

        datapoints.push_back(m_createDataObject(asdu, ioa, label, value, nullptr, ts));
    }
    else
        datapoints.push_back(m_createDataObject(asdu, ioa, label, value, nullptr));
}

bool
IEC104Client::prepareConnections()
{
    auto& redGroups = m_config->RedundancyGroups();

    auto configuredRedGroups = static_cast<int>(redGroups.size());
    for (int i=0 ; i<configuredRedGroups ; i++) {
        auto& redGroup = redGroups[i];
        auto& connections = redGroup->Connections();

        for (int j=0 ; j<connections.size() ; j++) {
            auto connection = connections[j];
            auto newConnection = std::make_shared<IEC104ClientConnection>(m_iec104->getClient(), redGroup, connection, m_config, (j == 0 ? "A" : "B"));
            if (newConnection != nullptr) {
                m_connections.push_back(newConnection);
            }
        }
        // Send initial path connection status audit
        auto configuredConnections = static_cast<int>(connections.size());
        if (configuredConnections == 0) {
            Iec104Utility::audit_info("SRVFL", m_iec104->getServiceName() + "-" + std::to_string(i) + "-A-unused");
        }
        if (configuredConnections <= 1) {
            Iec104Utility::audit_info("SRVFL", m_iec104->getServiceName() + "-" + std::to_string(i) + "-B-unused");
        }
    }
    // Send initial path connection status audit
    int maxRedGroups = m_config->GetMaxRedGroups();
    for(int i=configuredRedGroups ; i<maxRedGroups ; i++) {
        Iec104Utility::audit_info("SRVFL", m_iec104->getServiceName() + "-" + std::to_string(i) + "-A-unused");
        Iec104Utility::audit_info("SRVFL", m_iec104->getServiceName() + "-" + std::to_string(i) + "-B-unused");
    }

    // Send initial connection status audit
    Iec104Utility::audit_fail("SRVFL", m_iec104->getServiceName() + "-disconnected");

    return true;
}

void
IEC104Client::start()
{
    std::string beforeLog = Iec104Utility::PluginName + " - IEC104Client::start -";
    Iec104Utility::log_info("%s IEC104 client starting (started: %s)...", beforeLog.c_str(), m_started?"true":"false");
    if (m_started == false) {

        prepareConnections();

        m_started = true;
        m_monitoringThread = std::make_shared<std::thread>(&IEC104Client::_monitoringThread, this);
    }
    Iec104Utility::log_info("%s IEC104 client started!", beforeLog.c_str());
}

void
IEC104Client::stop()
{
    std::string beforeLog = Iec104Utility::PluginName + " - IEC104Client::stop -";
    Iec104Utility::log_info("%s IEC104 client stopping...", beforeLog.c_str());
    if (m_started == true)
    {
        m_started = false;
        Iec104Utility::log_debug("%s Waiting for monitoring thread to join", beforeLog.c_str());
        if (m_monitoringThread != nullptr) {
            m_monitoringThread->join();
            m_monitoringThread = nullptr;
        }
    }
    Iec104Utility::log_info("%s IEC104 client stopped!", beforeLog.c_str());
}

void
IEC104Client::_monitoringThread()
{
    std::string beforeLog = Iec104Utility::PluginName + " - IEC104Client::_monitoringThread -";
    uint64_t qualityUpdateTimeout = 500; /* 500 ms */
    uint64_t qualityUpdateTimer = 0;
    bool qualityUpdated = false;
    bool firstConnected = false;

    if (m_started)
    {
        Iec104Utility::log_info("%s Starting all client connections", beforeLog.c_str());
        for (auto clientConnection : m_connections)
        {
            clientConnection->Start();
        }
    }

    updateConnectionStatus(ConnectionStatus::NOT_CONNECTED);

    updateQualityForAllDataObjects(IEC60870_QUALITY_INVALID);

    uint64_t backupConnectionStartTime = Hal_getTimeInMs() + BACKUP_CONNECTION_TIMEOUT;

    while (m_started)
    {
        {
            std::lock_guard<std::mutex> lock(m_activeConnectionMtx);

            if (m_activeConnection == nullptr)
            {
                bool foundOpenConnections = false;

                /* activate the first open connection */
                for (auto clientConnection : m_connections)
                {
                    if (clientConnection->Connected()) {

                        backupConnectionStartTime = Hal_getTimeInMs() + BACKUP_CONNECTION_TIMEOUT;

                        foundOpenConnections = true;

                        clientConnection->Activate();

                        m_activeConnection = clientConnection;

                        updateConnectionStatus(ConnectionStatus::STARTED);

                        Iec104Utility::log_info("%s Activated connection", beforeLog.c_str());
                        break;
                    }
                }

                if (foundOpenConnections) {
                    firstConnected = true;
                    qualityUpdateTimer = 0;
                    qualityUpdated = false;
                }
                else {

                    if (firstConnected) {

                        if (qualityUpdated == false) {
                            if (qualityUpdateTimer != 0) {
                                if (getMonotonicTimeInMs() > qualityUpdateTimer) {
                                    Iec104Utility::log_info("%s Sending all data objects with non topical quality after connection lost for %dms",
                                                            beforeLog.c_str(), qualityUpdateTimer);
                                    updateQualityForAllDataObjects(IEC60870_QUALITY_NON_TOPICAL);
                                    qualityUpdated = true;
                                }
                            }
                            else {
                                qualityUpdateTimer = getMonotonicTimeInMs() + qualityUpdateTimeout;
                            }
                        }

                    }

                    updateConnectionStatus(ConnectionStatus::NOT_CONNECTED);

                    if (Hal_getTimeInMs() > backupConnectionStartTime)
                    {
                        Iec104Utility::log_info("%s Activating backup connections", beforeLog.c_str());
                        /* Connect all disconnected connections */
                        for (auto clientConnection : m_connections)
                        {
                            if (clientConnection->Disconnected()) {
                                clientConnection->Connect();
                            }
                        }

                        backupConnectionStartTime = Hal_getTimeInMs() + BACKUP_CONNECTION_TIMEOUT;
                    }
                }
            }
            else {
                backupConnectionStartTime = Hal_getTimeInMs() + BACKUP_CONNECTION_TIMEOUT;

                if (m_activeConnection->Connected() == false)
                {
                    Iec104Utility::log_info("%s Active connection lost", beforeLog.c_str());
                    m_activeConnection = nullptr;
                }
                else {
                    /* Check for connection that should be disconnected */

                    for (auto clientConnection : m_connections)
                    {
                        if (clientConnection != m_activeConnection) {
                            if (clientConnection->Connected() && !clientConnection->Autostart()) {
                                Iec104Utility::log_info("%s Disconnecting unnecessary connection", beforeLog.c_str());
                                clientConnection->Disonnect();
                            }
                        }
                    }
                }
            }
        }

        checkOutstandingCommandTimeouts();

        Thread_sleep(100);
    }

    Iec104Utility::log_info("%s Terminating all client connections", beforeLog.c_str());

    std::lock_guard<std::mutex> lock(m_activeConnectionMtx);
    m_activeConnection = nullptr;
    // This ensures that all shared_ptr to IEC104ClientConnection present in outstanding commands are cleared
    // before we exit the monitoring thread which prevents crashes in some unit tests where connection object
    // would be destroyed after the IEC104Client object was destroyed.
    if(!m_outstandingCommands.empty()) {
        std::lock_guard<std::mutex> lock2(m_outstandingCommandsMtx);
        m_outstandingCommands.clear();
    }
    m_connections.clear();
    updateConnectionStatus(ConnectionStatus::NOT_CONNECTED);
}

bool
IEC104Client::sendInterrogationCommand(int ca)
{
    // send interrogation request over active connection
    bool success = false;

    std::lock_guard<std::mutex> lock(m_activeConnectionMtx);

    if (m_activeConnection != nullptr)
    {
        success = m_activeConnection->sendInterrogationCommand(ca);
    }

    return success;
}

std::shared_ptr<IEC104Client::OutstandingCommand> IEC104Client::addOutstandingCommandAndCheckLimit(int ca, int ioa, bool withTime, int typeIdWithTimestamp, int typeIdNoTimestamp)
{
    std::string beforeLog = Iec104Utility::PluginName + " - IEC104Client::addOutstandingCommandAndCheckLimit -";
    std::shared_ptr<OutstandingCommand> command;

    // check if number of allowed parallel commands is not exceeded.

    std::lock_guard<std::mutex> lock(m_activeConnectionMtx);

    int cmdParrallel = m_config->CmdParallel();
    int typeId = withTime ? typeIdWithTimestamp : typeIdNoTimestamp;
    if (cmdParrallel > 0) {

        if (m_outstandingCommands.size() < cmdParrallel) {
            command = std::make_shared<OutstandingCommand>(typeId, ca, ioa, m_activeConnection);
        }
        else {
            Iec104Utility::log_warn("%s Maximum number of parallel command exceeded (%d) -> ignore command with typeId=%s, CA=%d, IOA=%d",
                                    beforeLog.c_str(), cmdParrallel, IEC104ClientConfig::getStringFromTypeID(typeId).c_str(), ca, ioa);
            return nullptr;
        }
    }
    else {
        command = std::make_shared<OutstandingCommand>(typeId, ca, ioa, m_activeConnection);
    }

    if (command) {
        m_outstandingCommands.push_back(command);
    }

    return command;
}

bool
IEC104Client::sendSingleCommand(int ca, int ioa, bool value, bool withTime, bool select, long time)
{
    std::string beforeLog = Iec104Utility::PluginName + " - IEC104Client::sendSingleCommand -";
    // send single command over active connection
    bool success = false;

    // check if the data point is in the exchange configuration
    std::string cmdName = withTime?"C_SC_TA_1":"C_SC_NA_1";
    if (m_config->checkExchangeDataLayer(C_SC_NA_1, ca, ioa) == nullptr) {
        Iec104Utility::log_error("%s Command %s (CA: %d, IOA: %d) not found in exchange configuration",
                                beforeLog.c_str(), cmdName.c_str(), ca, ioa);
        return false;
    }

    std::shared_ptr<OutstandingCommand> command = addOutstandingCommandAndCheckLimit(ca, ioa, withTime, C_SC_TA_1, C_SC_NA_1);

    if (command == nullptr)
        return false; //LCOV_EXCL_LINE

    std::lock_guard<std::mutex> lock(m_activeConnectionMtx);

    if (m_activeConnection != nullptr)
    {
        success = m_activeConnection->sendSingleCommand(ca, ioa, value, withTime, select, time);
    }
    else {
        Iec104Utility::log_warn("%s No active connection, cannot send command %s (CA: %d, IOA: %d)",
                                beforeLog.c_str(), cmdName.c_str(), ca, ioa);
    }

    if (!success) removeOutstandingCommand(command);

    return success;
}

bool
IEC104Client::sendDoubleCommand(int ca, int ioa, int value, bool withTime, bool select, long time)
{
    std::string beforeLog = Iec104Utility::PluginName + " - IEC104Client::sendDoubleCommand -";
    // send double command over active connection
    bool success = false;

    // check if the data point is in the exchange configuration
    std::string cmdName = withTime?"C_DC_TA_1":"C_DC_NA_1";
    if (m_config->checkExchangeDataLayer(C_DC_NA_1, ca, ioa) == nullptr) {
        Iec104Utility::log_error("%s Command %s (CA: %d, IOA: %d) not found in exchange configuration",
                                beforeLog.c_str(), cmdName.c_str(), ca, ioa);
        return false;
    }

    std::shared_ptr<OutstandingCommand> command = addOutstandingCommandAndCheckLimit(ca, ioa, withTime, C_DC_TA_1, C_DC_NA_1);

    if (command == nullptr)
        return false;//LCOV_EXCL_LINE

    std::lock_guard<std::mutex> lock(m_activeConnectionMtx);

    if (m_activeConnection != nullptr)
    {
        success = m_activeConnection->sendDoubleCommand(ca, ioa, value, withTime, select, time);
    }
    else {
        Iec104Utility::log_warn("%s No active connection, cannot send command %s (CA: %d, IOA: %d)",
                                beforeLog.c_str(), cmdName.c_str(), ca, ioa);
    }

    if (!success) removeOutstandingCommand(command);

    return success;
}

bool
IEC104Client::sendStepCommand(int ca, int ioa, int value, bool withTime, bool select, long time)
{
    std::string beforeLog = Iec104Utility::PluginName + " - IEC104Client::sendStepCommand -";
    // send step command over active connection
    bool success = false;

    // check if the data point is in the exchange configuration
    std::string cmdName = withTime?"C_RC_TA_1":"C_RC_NA_1";
    if (m_config->checkExchangeDataLayer(C_RC_NA_1, ca, ioa) == nullptr) {
        Iec104Utility::log_error("%s Command %s (CA: %d, IOA: %d) not found in exchange configuration",
                                beforeLog.c_str(), cmdName.c_str(), ca, ioa);
        return false;
    }

    std::shared_ptr<OutstandingCommand> command = addOutstandingCommandAndCheckLimit(ca, ioa, withTime, C_RC_TA_1, C_RC_NA_1);

    if (command == nullptr)
        return false;//LCOV_EXCL_LINE

    std::lock_guard<std::mutex> lock(m_activeConnectionMtx);

    if (m_activeConnection != nullptr)
    {
        success = m_activeConnection->sendStepCommand(ca, ioa, value, withTime, select, time);
    }
    else {
        Iec104Utility::log_warn("%s No active connection, cannot send command %s (CA: %d, IOA: %d)",
                                beforeLog.c_str(), cmdName.c_str(), ca, ioa);
    }

    if (!success) removeOutstandingCommand(command);

    return success;
}

bool
IEC104Client::sendSetpointNormalized(int ca, int ioa, float value, bool withTime, long time)
{
    std::string beforeLog = Iec104Utility::PluginName + " - IEC104Client::sendSetpointNormalized -";
    // send setpoint command normalized over active connection
    bool success = false;

    // check if the data point is in the exchange configuration
    std::string cmdName = withTime?"C_SE_TA_1":"C_SE_NA_1";
    if (m_config->checkExchangeDataLayer(C_SE_NA_1, ca, ioa) == nullptr) {
        Iec104Utility::log_error("%s Command %s (CA: %d, IOA: %d) not found in exchange configuration",
                                beforeLog.c_str(), cmdName.c_str(), ca, ioa);
        return false;
    }

    std::shared_ptr<OutstandingCommand> command = addOutstandingCommandAndCheckLimit(ca, ioa, withTime, C_SE_TA_1, C_SE_NA_1);

    if (command == nullptr)
        return false;//LCOV_EXCL_LINE

    std::lock_guard<std::mutex> lock(m_activeConnectionMtx);

    if (m_activeConnection != nullptr)
    {
        success = m_activeConnection->sendSetpointNormalized(ca, ioa, value, withTime, time);
    }
    else {
        Iec104Utility::log_warn("%s No active connection, cannot send command %s (CA: %d, IOA: %d)",
                                beforeLog.c_str(), cmdName.c_str(), ca, ioa);
    }

    if (!success) removeOutstandingCommand(command);

    return success;
}

bool
IEC104Client::sendSetpointScaled(int ca, int ioa, int value, bool withTime, long time)
{
    std::string beforeLog = Iec104Utility::PluginName + " - IEC104Client::sendSetpointScaled -";
    // send setpoint command scaled over active connection
    bool success = false;

    // check if the data point is in the exchange configuration
    std::string cmdName = withTime?"C_SE_TB_1":"C_SE_NB_1";
    if (m_config->checkExchangeDataLayer(C_SE_NB_1, ca, ioa) == nullptr) {
        Iec104Utility::log_error("%s Command %s (CA: %d, IOA: %d) not found in exchange configuration",
                                beforeLog.c_str(), cmdName.c_str(), ca, ioa);
        return false;
    }

    std::shared_ptr<OutstandingCommand> command = addOutstandingCommandAndCheckLimit(ca, ioa, withTime, C_SE_TB_1, C_SE_NB_1);

    if (command == nullptr)
        return false;//LCOV_EXCL_LINE

    std::lock_guard<std::mutex> lock(m_activeConnectionMtx);

    if (m_activeConnection != nullptr)
    {
        success = m_activeConnection->sendSetpointScaled(ca, ioa, value, withTime, time);
    }
    else {
        Iec104Utility::log_warn("%s No active connection, cannot send command %s (CA: %d, IOA: %d)",
                                beforeLog.c_str(), cmdName.c_str(), ca, ioa);
    }

    if (!success) removeOutstandingCommand(command);

    return success;
}

bool
IEC104Client::sendSetpointShort(int ca, int ioa, float value, bool withTime, long time)
{
    std::string beforeLog = Iec104Utility::PluginName + " - IEC104Client::sendSetpointShort -";
    // send setpoint command short over active connection
    bool success = false;

    // check if the data point is in the exchange configuration
    std::string cmdName = withTime?"C_SE_TC_1":"C_SE_NC_1";
    if (m_config->checkExchangeDataLayer(C_SE_NC_1, ca, ioa) == nullptr) {
        Iec104Utility::log_error("%s Command %s (CA: %d, IOA: %d) not found in exchange configuration",
                                beforeLog.c_str(), cmdName.c_str(), ca, ioa);
        return false;
    }

    std::shared_ptr<OutstandingCommand> command = addOutstandingCommandAndCheckLimit(ca, ioa, withTime, C_SE_TC_1, C_SE_NC_1);

    if (command == nullptr)
        return false;//LCOV_EXCL_LINE

    std::lock_guard<std::mutex> lock(m_activeConnectionMtx);

    if (m_activeConnection != nullptr)
    {
        success = m_activeConnection->sendSetpointShort(ca, ioa, value, withTime, time);
    }
    else {
        Iec104Utility::log_warn("%s No active connection, cannot send command %s (CA: %d, IOA: %d)",
                                beforeLog.c_str(), cmdName.c_str(), ca, ioa);
    }

    if (!success) removeOutstandingCommand(command);

    return success;
}

 bool
 IEC104Client::sendConnectionStatus()
 {
    sendSouthMonitoringEvent(true, true);

    return true;
 }

const std::string&
IEC104Client::getServiceName() const
{
    return m_iec104->getServiceName();
}

bool IEC104Client::scheduleGI()
{
    if (m_activeConnection != nullptr)
    {
        return m_activeConnection->addScheduledGI();
    }
    return false;
}