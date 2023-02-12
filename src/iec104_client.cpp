/*
 * Fledge IEC 104 south plugin.
 *
 * Copyright (c) 2022, RTE (https://www.rte-france.com)
 *
 * Released under the Apache 2.0 Licence
 *
 * Author: Michael Zillgith
 */

#include <iec104.h>
#include <logger.h>
#include <reading.h>
#include <lib60870/cs104_connection.h>

#include <cmath>
#include <ctime>
#include <fstream>
#include <iostream>
#include <utility>
#include <algorithm>

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

static map<int, string> mapAsduTypeIdStr = {
    {M_ME_NB_1, "M_ME_NB_1"},
    {M_SP_NA_1, "M_SP_NA_1"},
    {M_SP_TB_1, "M_SP_TB_1"},
    {M_DP_NA_1, "M_DP_NA_1"},
    {M_DP_TB_1, "M_DP_TB_1"},
    {M_ST_NA_1, "M_ST_NA_1"},
    {M_ST_TB_1, "M_ST_TB_1"},
    {M_ME_NA_1, "M_ME_NA_1"},
    {M_ME_TD_1, "M_ME_TD_1"},
    {M_ME_TE_1, "M_ME_TE_1"},
    {M_ME_NC_1, "M_ME_NC_1"},
    {M_ME_TF_1, "M_ME_TF_1"},
    {C_SC_TA_1, "C_SC_TA_1"},
    {C_SC_NA_1, "C_SC_NA_1"},
    {C_DC_TA_1, "C_DC_TA_1"},
    {C_DC_NA_1, "C_DC_NA_1"},
    {C_RC_TA_1, "C_RC_TA_1"},
    {C_RC_NA_1, "C_RC_NA_1"},
    {C_SE_TA_1, "C_SE_TA_1"},
    {C_SE_NA_1, "C_SE_NA_1"},
    {C_SE_TB_1, "C_SE_TB_1"},
    {C_SE_NB_1, "C_SE_NB_1"},
    {C_SE_TC_1, "C_SE_TC_1"},
    {C_SE_NC_1, "C_SE_NC_1"}
};

int
IEC104Client::broadcastCA()
{
    if (m_config->CaSize() == 1)
        return 0xff;

    return 0xffff;
}

Datapoint* IEC104Client::m_createQualityUpdateForDataObject(DataExchangeDefinition* dataDefinition, QualityDescriptor* qd, CP56Time2a ts)
{
    auto* attributes = new vector<Datapoint*>;

    attributes->push_back(m_createDatapoint("do_type", mapAsduTypeIdStr[dataDefinition->typeId]));

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

static bool isDataPointInMonitoringDirection(DataExchangeDefinition* dp)
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

IEC104Client::OutstandingCommand* IEC104Client::checkForOutstandingCommand(int typeId, int ca, int ioa, IEC104ClientConnection* connection)
{
    m_outstandingCommandsMtx.lock();

    for (OutstandingCommand* command : m_outstandingCommands) {
        if ((command->typeId == typeId) && (command->ca == ca) && (command->ioa == ioa) && (command->clientCon == connection)) {
            m_outstandingCommandsMtx.unlock();
            return command;
        }
    }

    m_outstandingCommandsMtx.unlock();

    return nullptr;
}

void IEC104Client::checkOutstandingCommandTimeouts()
{
    uint64_t currentTime = getMonotonicTimeInMs();

    std::vector<OutstandingCommand*> listOfTimedoutCommands;

    m_outstandingCommandsMtx.lock();

    for (OutstandingCommand* command : m_outstandingCommands)
    {    
        if (command->actConReceived) {
            if (command->timeout + m_config->CmdTermTimeout() < currentTime) {
                printf("ACT-TERM timeout for outstanding command - type: %i ca: %i ioa: %i ack: %i\n", command->typeId, command->ca, command->ioa, command->actConReceived);
                
                Logger::getLogger()->warn("ACT-TERM timeout for outstanding command - type: %i ca: %i ioa: %i", command->typeId, command->ca, command->ioa);

                listOfTimedoutCommands.push_back(command);
            }
        }
        else {
            if (command->timeout + m_config->CmdAckTimeout() < currentTime) {
                printf("ACT-CON timeout for outstanding command - type: %i ca: %i ioa: %i ack: %i\n", command->typeId, command->ca, command->ioa, command->actConReceived);

                Logger::getLogger()->warn("ACT-CON timeout for outstanding command - type: %i ca: %i ioa: %i", command->typeId, command->ca, command->ioa);

                listOfTimedoutCommands.push_back(command);
            }
        }
    }

    // remove all timed out commands from the list of outstanding commands
    for (OutstandingCommand* commandToRemove : listOfTimedoutCommands) {
        // remove object command from m_outstandingCommands
        m_outstandingCommands.erase(std::remove(m_outstandingCommands.begin(), m_outstandingCommands.end(), commandToRemove), m_outstandingCommands.end());

        delete commandToRemove;
    }

    m_outstandingCommandsMtx.unlock();
}

void IEC104Client::removeOutstandingCommand(OutstandingCommand* command)
{
    m_outstandingCommandsMtx.lock();

    // remove object command from m_outstandingCommands
    m_outstandingCommands.erase(std::remove(m_outstandingCommands.begin(), m_outstandingCommands.end(), command), m_outstandingCommands.end());

    m_outstandingCommandsMtx.unlock();

    delete command;
}

void IEC104Client::updateQualityForAllDataObjects(QualityDescriptor qd)
{
    vector<Datapoint*> datapoints;
    vector<string> labels;

    for (auto const& exchangeDefintions : m_config->ExchangeDefinition()) {
        for (auto const& dpPair : exchangeDefintions.second) {
            DataExchangeDefinition* dp = dpPair.second;

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

template <class T>
Datapoint* IEC104Client::m_createDataObject(CS101_ASDU asdu, int64_t ioa, const std::string& dataname, const T value,
    QualityDescriptor* qd, CP56Time2a ts)
{
    auto* attributes = new vector<Datapoint*>;

    attributes->push_back(m_createDatapoint("do_type", mapAsduTypeIdStr[CS101_ASDU_getTypeID(asdu)]));

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

IEC104Client::OutstandingCommand::OutstandingCommand(int typeId, int ca, int ioa, IEC104ClientConnection* con):
    typeId(typeId), ca(ca), ioa(ioa), clientCon(con)
{
    timeout = getMonotonicTimeInMs();
}

void
IEC104Client::sendSouthMonitoringEvent(bool connxStatus, bool giStatus)
{
    if (m_config->GetConnxStatusSignal().empty())
        return;

    if ((connxStatus == false) && (giStatus == false))
        return;

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

    Datapoint* southEvent = new Datapoint("iec104_south_event", dpv);

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
}

void
IEC104Client::updateGiStatus(GiStatus newState)
{
    if (m_giStatus == newState)
        return;

    m_giStatus = newState;

    sendSouthMonitoringEvent(false, true);
}

IEC104Client::GiStatus
IEC104Client::getGiStatus()
{
    return m_giStatus;
}

IEC104Client::IEC104Client(IEC104* iec104, IEC104ClientConfig* config)
        : m_iec104(iec104),
          m_config(config)
{
}

IEC104Client::~IEC104Client()
{
    stop();

    for (auto outstandingCommand : m_outstandingCommands) {
        delete outstandingCommand;
    }
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
IEC104Client::handleASDU(IEC104ClientConnection* connection, CS101_ASDU asdu)
{
    bool handledAsdu = true;

    vector<Datapoint*> datapoints;
    vector<string> labels;

    IEC60870_5_TypeID typeId = CS101_ASDU_getTypeID(asdu);
    int ca = CS101_ASDU_getCA(asdu);

    if (isInterrogationResponse(asdu)) {
        if (getGiStatus() == GiStatus::STARTED)
            updateGiStatus(GiStatus::IN_PROGRESS);
    }

    for (int i = 0; i < CS101_ASDU_getNumberOfElements(asdu); i++)
    {
        InformationObject io = CS101_ASDU_getElement(asdu, i);

        if (io) 
        {
            int ioa = InformationObject_getObjectAddress(io);

            std::string* label = m_config->checkExchangeDataLayer(typeId, ca, ioa);

            OutstandingCommand* outstandingCommand = nullptr;

            if (isSupportedCommand(typeId)) {
                outstandingCommand = checkForOutstandingCommand(typeId, ca, ioa, connection);
            }

            bool typeSupported = true;

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
                }
                else {
                    Logger::getLogger()->warn("ASDU type %s not supported for %s (CA: %i IOA: %i)", mapAsduTypeIdStr[typeId].c_str(), label->c_str(), ca, ioa);
                }
            }
            else {
                if (handledAsdu) {
                    Logger::getLogger()->debug("No data point found in exchange configuration for %s with CA: %i IOA: %i", mapAsduTypeIdStr[typeId].c_str(), ca, ioa);
                }
            }

            InformationObject_destroy(io);
        }
        else 
        {
            Logger::getLogger()->error("ASDU with invalid or unknown information object");
        }
    }

    if (labels.empty() == false)
    {
        sendData(datapoints, labels);
    }

    return handledAsdu;
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
    int64_t value =
        SinglePointInformation_getValue((SinglePointInformation)io_casted);
    QualityDescriptor qd =
        SinglePointInformation_getQuality((SinglePointInformation)io_casted);

    CP56Time2a ts = SinglePointWithCP56Time2a_getTimestamp(io_casted);

    datapoints.push_back(m_createDataObject(asdu, ioa, label, value, &qd, ts));
}

void IEC104Client::handle_M_DP_NA_1(vector<Datapoint*>& datapoints, string& label,
                             unsigned int ca,
                             CS101_ASDU asdu, InformationObject io,
                             uint64_t ioa)
{
    auto io_casted = (DoublePointInformation)io;
    int64_t value =
        DoublePointInformation_getValue((DoublePointInformation)io_casted);
    QualityDescriptor qd =
        DoublePointInformation_getQuality((DoublePointInformation)io_casted);
    
    datapoints.push_back(m_createDataObject(asdu, ioa, label, value, &qd));
}

void IEC104Client::handle_M_DP_TB_1(vector<Datapoint*>& datapoints, string& label,
                             unsigned int ca,
                             CS101_ASDU asdu, InformationObject io,
                             uint64_t ioa)
{
    auto io_casted = (DoublePointWithCP56Time2a)io;
    int64_t value =
        DoublePointInformation_getValue((DoublePointInformation)io_casted);
    QualityDescriptor qd =
        DoublePointInformation_getQuality((DoublePointInformation)io_casted);

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
    int64_t value =
        StepPositionInformation_getValue((StepPositionInformation)io_casted);
    QualityDescriptor qd =
        StepPositionInformation_getQuality((StepPositionInformation)io_casted);

    datapoints.push_back(m_createDataObject(asdu, ioa, label, value, &qd));
}

void IEC104Client::handle_M_ST_TB_1(vector<Datapoint*>& datapoints, string& label,
                             unsigned int ca,
                             CS101_ASDU asdu, InformationObject io,
                             uint64_t ioa)
{
    auto io_casted = (StepPositionWithCP56Time2a)io;
    int64_t value =
        StepPositionInformation_getValue((StepPositionInformation)io_casted);
    QualityDescriptor qd =
        StepPositionInformation_getQuality((StepPositionInformation)io_casted);

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
    float value =
        MeasuredValueNormalized_getValue((MeasuredValueNormalized)io_casted);
    QualityDescriptor qd =
        MeasuredValueNormalized_getQuality((MeasuredValueNormalized)io_casted);

    datapoints.push_back(m_createDataObject(asdu, ioa, label, value, &qd));
}

void IEC104Client::handle_M_ME_TD_1(vector<Datapoint*>& datapoints, string& label,
                             unsigned int ca,
                             CS101_ASDU asdu, InformationObject io,
                             uint64_t ioa)
{
    auto io_casted = (MeasuredValueNormalizedWithCP56Time2a)io;
    float value =
        MeasuredValueNormalized_getValue((MeasuredValueNormalized)io_casted);
    QualityDescriptor qd =
        MeasuredValueNormalized_getQuality((MeasuredValueNormalized)io_casted);
    
    CP56Time2a ts =
        MeasuredValueNormalizedWithCP56Time2a_getTimestamp(io_casted);
    bool is_invalid = CP56Time2a_isInvalid(ts);

    datapoints.push_back(m_createDataObject(asdu, ioa, label, value, &qd, ts));
}

void IEC104Client::handle_M_ME_TE_1(vector<Datapoint*>& datapoints, string& label,
                             unsigned int ca,
                             CS101_ASDU asdu, InformationObject io,
                             uint64_t ioa)
{
    auto io_casted = (MeasuredValueScaledWithCP56Time2a)io;
    int64_t value =
        MeasuredValueScaled_getValue((MeasuredValueScaled)io_casted);
    QualityDescriptor qd =
        MeasuredValueScaled_getQuality((MeasuredValueScaled)io_casted);

    CP56Time2a ts =
        MeasuredValueScaledWithCP56Time2a_getTimestamp(io_casted);
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
    QualityDescriptor qd =
        MeasuredValueShort_getQuality((MeasuredValueShort)io_casted);

    datapoints.push_back(m_createDataObject(asdu, ioa, label, value, &qd));
}

void IEC104Client::handle_M_ME_TF_1(vector<Datapoint*>& datapoints, string& label,
                             unsigned int ca,
                             CS101_ASDU asdu, InformationObject io,
                             uint64_t ioa)
{
    auto io_casted = (MeasuredValueShortWithCP56Time2a)io;
    float value = MeasuredValueShort_getValue((MeasuredValueShort)io_casted);
    QualityDescriptor qd =
        MeasuredValueShort_getQuality((MeasuredValueShort)io_casted);

    CP56Time2a ts =
        MeasuredValueShortWithCP56Time2a_getTimestamp(io_casted);
    bool is_invalid = CP56Time2a_isInvalid(ts);

    datapoints.push_back(m_createDataObject(asdu, ioa, label, value, &qd, ts));
}

void IEC104Client::handle_C_SC_NA_1(vector<Datapoint*>& datapoints, string& label,
                             unsigned int ca,
                             CS101_ASDU asdu, InformationObject io,
                             uint64_t ioa,
                             OutstandingCommand* outstandingCommand)
{
    auto io_casted = (SingleCommandWithCP56Time2a)io;
    int64_t state = SingleCommand_getState((SingleCommand)io_casted);

    QualifierOfCommand qu = SingleCommand_getQU((SingleCommand)io_casted);

    printf("C_SC_NA_1 - COT: %s\n", CS101_CauseOfTransmission_toString(CS101_ASDU_getCOT(asdu)));

    if (outstandingCommand) {
        if (CS101_ASDU_getCOT(asdu) == CS101_COT_ACTIVATION_CON) {
            printf("C_SC_NA_1: received ACT-CON\n");
            outstandingCommand->actConReceived = true;
            outstandingCommand->timeout = getMonotonicTimeInMs();
        }
        else if (CS101_ASDU_getCOT(asdu) == CS101_COT_ACTIVATION_TERMINATION) {
            printf("C_SC_NA_1: received ACT-TERM\n");
            removeOutstandingCommand(outstandingCommand);
        }
    }

    if (CS101_ASDU_getTypeID(asdu) == C_SC_TA_1)
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
                             OutstandingCommand* outstandingCommand)
{
    auto io_casted = (DoubleCommandWithCP56Time2a)io;
    int64_t state = DoubleCommand_getState((DoubleCommand)io_casted);

    QualifierOfCommand qu = DoubleCommand_getQU((DoubleCommand)io_casted);

    if (outstandingCommand) {
        if (CS101_ASDU_getCOT(asdu) == CS101_COT_ACTIVATION_CON) {
            outstandingCommand->actConReceived = true;
            outstandingCommand->timeout = getMonotonicTimeInMs();
        }
        else if (CS101_ASDU_getCOT(asdu) == CS101_COT_ACTIVATION_TERMINATION) {
            removeOutstandingCommand(outstandingCommand);
        }
    }

    if (CS101_ASDU_getTypeID(asdu) == C_DC_TA_1)
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
                             OutstandingCommand* outstandingCommand)
{
    auto io_casted = (StepCommandWithCP56Time2a)io;
    int64_t state = StepCommand_getState((StepCommand)io_casted);

    QualifierOfCommand qu = StepCommand_getQU((StepCommand)io_casted);

    if (outstandingCommand) {
        if (CS101_ASDU_getCOT(asdu) == CS101_COT_ACTIVATION_CON) {
            outstandingCommand->actConReceived = true;
            outstandingCommand->timeout = getMonotonicTimeInMs();
        }
        else if (CS101_ASDU_getCOT(asdu) == CS101_COT_ACTIVATION_TERMINATION) {
            removeOutstandingCommand(outstandingCommand);
        }
    }

    if (CS101_ASDU_getTypeID(asdu) == C_DC_TA_1)
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
                             OutstandingCommand* outstandingCommand)
{
    auto io_casted = (SetpointCommandNormalizedWithCP56Time2a)io;

    float value = SetpointCommandNormalized_getValue((SetpointCommandNormalized)io_casted);

    QualifierOfCommand ql = SetpointCommandNormalized_getQL((SetpointCommandNormalized)io_casted);

    if (outstandingCommand) {
        if (CS101_ASDU_getCOT(asdu) == CS101_COT_ACTIVATION_CON) {
            removeOutstandingCommand(outstandingCommand);
        }
    }

    if (CS101_ASDU_getTypeID(asdu) == C_SE_TA_1)
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
                             OutstandingCommand* outstandingCommand)
{
    auto io_casted = (SetpointCommandScaledWithCP56Time2a)io;

    int64_t value = SetpointCommandScaled_getValue((SetpointCommandScaled)io_casted);

    QualifierOfCommand ql = SetpointCommandScaled_getQL((SetpointCommandScaled)io_casted);

    if (outstandingCommand) {
        if (CS101_ASDU_getCOT(asdu) == CS101_COT_ACTIVATION_CON) {
            removeOutstandingCommand(outstandingCommand);
        }
    }

    if (CS101_ASDU_getTypeID(asdu) == C_SE_TB_1)
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
                             OutstandingCommand* outstandingCommand)
{
    auto io_casted = (SetpointCommandShortWithCP56Time2a)io;

    float value = SetpointCommandShort_getValue((SetpointCommandShort)io_casted);

    QualifierOfCommand ql = SetpointCommandShort_getQL((SetpointCommandShort)io_casted);

    if (outstandingCommand) {
        if (CS101_ASDU_getCOT(asdu) == CS101_COT_ACTIVATION_CON) {
            removeOutstandingCommand(outstandingCommand);
        }
    }

    if (CS101_ASDU_getTypeID(asdu) == C_SE_TC_1)
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
    std::vector<IEC104ClientRedGroup*>& redGroups = m_config->RedundancyGroups();

    for (auto& redGroup : redGroups) 
    {
        auto& connections = redGroup->Connections();

        for (auto connection : connections) 
        {
            IEC104ClientConnection* newConnection = new IEC104ClientConnection(this, redGroup, connection, m_config); 
            
            if (newConnection != nullptr) 
            {
                m_connections.push_back(newConnection);
            }
        }
    }

    return true;
}

void
IEC104Client::start()
{
    if (m_started == false) {

        prepareConnections();

        m_started = true;
        m_monitoringThread = new std::thread(&IEC104Client::_monitoringThread, this);
    } 
}

void 
IEC104Client::stop()
{
    if (m_started == true)
    {
        m_started = false;

        if (m_monitoringThread != nullptr) {
            m_monitoringThread->join();
            delete m_monitoringThread;
            m_monitoringThread = nullptr;
        }
    }
}

void
IEC104Client::_monitoringThread()
{
    uint64_t qualityUpdateTimeout = 500; /* 500 ms */
    uint64_t qualityUpdateTimer = 0;
    bool qualityUpdated = false;
    bool firstConnected = false;

    if (m_started) 
    {
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
        m_activeConnectionMtx.lock();

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
                    
                    break;
                }
            }

            if (foundOpenConnections) {
                firstConnected = true;
                qualityUpdateTimer = 0;
                qualityUpdated = false;
            }

            if (foundOpenConnections == false) {

                if (firstConnected) {

                    if (qualityUpdated == false) {
                        if (qualityUpdateTimer != 0) {
                            if (getMonotonicTimeInMs() > qualityUpdateTimer) {
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
                m_activeConnection = nullptr;
            }
            else {
                /* Check for connection that should be disconnected */

                for (auto clientConnection : m_connections)
                {
                    if (clientConnection != m_activeConnection) {
                        if (clientConnection->Connected() && !clientConnection->Autostart()) {
                            clientConnection->Disonnect();
                        }
                    }
                }
            }
        }

        m_activeConnectionMtx.unlock();

        checkOutstandingCommandTimeouts();

        Thread_sleep(100);
    }

    for (auto clientConnection : m_connections)
    {
        clientConnection->Stop();

        delete clientConnection;
    }

    m_connections.clear();
}

bool
IEC104Client::sendInterrogationCommand(int ca)
{
    // send interrogation request over active connection
    bool success = false;

    m_activeConnectionMtx.lock();

    if (m_activeConnection != nullptr)
    {
        success = m_activeConnection->sendInterrogationCommand(ca);
    }

    m_activeConnectionMtx.unlock();

    return success;
}

IEC104Client::OutstandingCommand* IEC104Client::addOutstandingCommandAndCheckLimit(int ca, int ioa, bool withTime, int typeIdWithTimestamp, int typeIdNoTimestamp)
{
    OutstandingCommand* command = nullptr;

    // check if number of allowed parallel commands is not exceeded.

    m_outstandingCommandsMtx.lock();

    if (m_config->CmdParallel() > 0) {

        if (m_outstandingCommands.size() < m_config->CmdParallel()) {
            command = new OutstandingCommand(withTime ? C_SC_TA_1 : C_SC_NA_1, ca, ioa, m_activeConnection);
        }
        else {
            m_outstandingCommandsMtx.unlock();

            printf("Maximum number of parallel command exceeded -> ignore command\n");

            Logger::getLogger()->warn("Maximum number of parallel command exceeded -> ignore command");

            return nullptr;
        }
    }
    else {
        command = new OutstandingCommand(withTime ? C_SC_TA_1 : C_SC_NA_1, ca, ioa, m_activeConnection);
    }

    if (command)
        m_outstandingCommands.push_back(command);

    m_outstandingCommandsMtx.unlock();

    return command;
}

bool
IEC104Client::sendSingleCommand(int ca, int ioa, bool value, bool withTime, bool select)
{
    // send single command over active connection
    bool success = false;

    // check if the data point is in the exchange configuration
    if (m_config->checkExchangeDataLayer(C_SC_NA_1, ca, ioa) == nullptr) {
        Logger::getLogger()->error("Failed to send C_SC_NA_1 command - no such data point");

        return false;
    }

    OutstandingCommand* command = addOutstandingCommandAndCheckLimit(ca, ioa, withTime, C_SC_TA_1, C_SC_NA_1);

    if (command == nullptr)
        return false;

    m_activeConnectionMtx.lock();

    if (m_activeConnection != nullptr)
    {
        success = m_activeConnection->sendSingleCommand(ca, ioa, value, withTime, select);
    }
    
    m_activeConnectionMtx.unlock();

    if (!success) removeOutstandingCommand(command);

    return success;
}

bool
IEC104Client::sendDoubleCommand(int ca, int ioa, int value, bool withTime, bool select)
{
    // send double command over active connection
    bool success = false;

    // check if the data point is in the exchange configuration
    if (m_config->checkExchangeDataLayer(C_DC_NA_1, ca, ioa) == nullptr) {
        Logger::getLogger()->error("Failed to send C_DC_NA_1 command - no such data point");

        return false;
    }

    OutstandingCommand* command = addOutstandingCommandAndCheckLimit(ca, ioa, withTime, C_DC_TA_1, C_DC_NA_1);

    if (command == nullptr)
        return false;

    m_activeConnectionMtx.lock();

    if (m_activeConnection != nullptr)
    {
        success = m_activeConnection->sendDoubleCommand(ca, ioa, value, withTime, select);
    }
    
    m_activeConnectionMtx.unlock();

    if (!success) removeOutstandingCommand(command);

    return success;
}

bool
IEC104Client::sendStepCommand(int ca, int ioa, int value, bool withTime, bool select)
{
    // send step command over active connection
    bool success = false;

    // check if the data point is in the exchange configuration
    if (m_config->checkExchangeDataLayer(C_RC_NA_1, ca, ioa) == nullptr) {
        Logger::getLogger()->error("Failed to send C_RC_NA_1 command - no such data point");

        return false;
    }

    OutstandingCommand* command = addOutstandingCommandAndCheckLimit(ca, ioa, withTime, C_RC_TA_1, C_RC_NA_1);

    if (command == nullptr)
        return false;

    m_activeConnectionMtx.lock();

    if (m_activeConnection != nullptr)
    {
        success = m_activeConnection->sendStepCommand(ca, ioa, value, withTime, select);
    }
    
    m_activeConnectionMtx.unlock();

    if (!success) removeOutstandingCommand(command);

    return success;
}

bool
IEC104Client::sendSetpointNormalized(int ca, int ioa, float value, bool withTime)
{
    // send setpoint command normalized over active connection
    bool success = false;

    // check if the data point is in the exchange configuration
    if (m_config->checkExchangeDataLayer(C_SE_NA_1, ca, ioa) == nullptr) {
        Logger::getLogger()->error("Failed to send C_SE_NA_1 command - no such data point");

        return false;
    }

    OutstandingCommand* command = addOutstandingCommandAndCheckLimit(ca, ioa, withTime, C_SE_TA_1, C_SE_NA_1);

    if (command == nullptr)
        return false;

    m_activeConnectionMtx.lock();

    if (m_activeConnection != nullptr)
    {
        success = m_activeConnection->sendSetpointNormalized(ca, ioa, value, withTime);
    }
    
    m_activeConnectionMtx.unlock();

    if (!success) removeOutstandingCommand(command);

    return success;
}

bool
IEC104Client::sendSetpointScaled(int ca, int ioa, int value, bool withTime)
{
    // send setpoint command scaled over active connection
    bool success = false;

    // check if the data point is in the exchange configuration
    if (m_config->checkExchangeDataLayer(C_SE_NB_1, ca, ioa) == nullptr) {
        Logger::getLogger()->error("Failed to send C_SE_NB_1 command - no such data point");

        return false;
    }

    OutstandingCommand* command = addOutstandingCommandAndCheckLimit(ca, ioa, withTime, C_SE_TB_1, C_SE_NB_1);

    if (command == nullptr)
        return false;

    m_activeConnectionMtx.lock();

    if (m_activeConnection != nullptr)
    {
        success = m_activeConnection->sendSetpointScaled(ca, ioa, value, withTime);
    }
    
    m_activeConnectionMtx.unlock();

    if (!success) removeOutstandingCommand(command);

    return success;
}

bool
IEC104Client::sendSetpointShort(int ca, int ioa, float value, bool withTime)
{
    // send setpoint command short over active connection
    bool success = false;

    // check if the data point is in the exchange configuration
    if (m_config->checkExchangeDataLayer(C_SE_NC_1, ca, ioa) == nullptr) {
        Logger::getLogger()->error("Failed to send C_SE_NC_1 command - no such data point");

        return false;
    }

    OutstandingCommand* command = addOutstandingCommandAndCheckLimit(ca, ioa, withTime, C_SE_TC_1, C_SE_NC_1);

    if (command == nullptr)
        return false;

    m_activeConnectionMtx.lock();

    if (m_activeConnection != nullptr)
    {
        success = m_activeConnection->sendSetpointShort(ca, ioa, value, withTime);
    }
    
    m_activeConnectionMtx.unlock();

    if (!success) removeOutstandingCommand(command);

    return success;
}

 bool
 IEC104Client::sendConnectionStatus()
 {
    sendSouthMonitoringEvent(true, true);

    return true;
 }
