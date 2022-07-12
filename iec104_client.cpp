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

#include <cmath>
#include <ctime>
#include <fstream>
#include <iostream>
#include <utility>

using namespace std;
using namespace nlohmann;

static uint64_t getMonotonicTimeInMs()
{
    uint64_t timeVal = 0;

    struct timespec ts;

    if (clock_gettime(CLOCK_MONOTONIC, &ts) == 0) {
        timeVal = ((uint64_t) ts.tv_sec * 1000LL) + (ts.tv_nsec / 1000000);
    }

    return timeVal;
}

// Map of all handled ASDU types by the plugin
static map<string, int> mapAsduTypeId = {
    {"M_ME_NB_1", M_ME_NB_1},
    {"M_SP_NA_1", M_SP_NA_1},
    {"M_SP_TB_1", M_SP_TB_1},
    {"M_DP_NA_1", M_DP_NA_1},
    {"M_DP_TB_1", M_DP_TB_1},
    {"M_ST_NA_1", M_ST_NA_1},
    {"M_ST_TB_1", M_ST_TB_1},
    {"M_ME_NA_1", M_ME_NA_1},
    {"M_ME_TD_1", M_ME_TD_1},
    {"M_ME_TE_1", M_ME_TE_1},
    {"M_ME_NC_1", M_ME_NC_1},
    {"M_ME_TF_1", M_ME_TF_1},
    {"C_SC_NA_1", C_SC_NA_1},
    {"C_SC_TA_1", C_SC_TA_1},
    {"C_DC_NA_1", C_DC_NA_1},
    {"C_DC_TA_1", C_DC_TA_1},
    {"C_RC_NA_1", C_RC_NA_1},
    {"C_RC_TA_1", C_RC_TA_1},
    {"C_SE_NA_1", C_SE_NA_1},
    {"C_SE_TA_1", C_SE_TA_1},
    {"C_SE_NB_1", C_SE_NB_1},
    {"C_SE_TB_1", C_SE_TB_1},
    {"C_SE_NC_1", C_SE_NC_1},
    {"C_SE_TC_1", C_SE_TC_1}
};

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

int IEC104Client::broadcastCA()
{
    int caSize = m_getConfigValueDefault<int>(
        *m_stack_configuration,
        "/application_layer/ca_asdu_size"_json_pointer, 2);

    if (caSize == 1)
        return 0xff;

    return 0xffff;
}

int IEC104Client::defaultCA()
{
    return m_getConfigValueDefault<int>(
        *m_stack_configuration,
        "/application_layer/default_ca"_json_pointer, -1);
}

int IEC104Client::timeSyncCA()
{
    return m_getConfigValueDefault<int>(
        *m_stack_configuration,
        "/application_layer/time_sync_ca"_json_pointer, -1);
}


bool IEC104Client::isMessageTypeMatching(int expectedType, int rcvdType)
{
    printf("isMessageTypeMatching(%i,%i)\n", expectedType, rcvdType);

    if (expectedType == rcvdType) {
        return true; /* direct match */
    }

    switch (expectedType) {

        case M_SP_NA_1:
            if ((rcvdType == M_SP_TA_1) || (rcvdType == M_SP_TB_1)) {
                return true;
            }

            break;

        case M_SP_TA_1:
            if ((rcvdType == M_SP_NA_1) || (rcvdType == M_SP_TB_1)) {
                return true;
            }

            break;

        case M_SP_TB_1:
            if ((rcvdType == M_SP_NA_1) || (rcvdType == M_SP_TA_1)) {
                return true;
            }

            break;

        case M_DP_NA_1:
            if ((rcvdType == M_DP_TA_1) || (rcvdType == M_DP_TB_1)) {
                return true;
            }

            break;

        case M_DP_TA_1:
            if ((rcvdType == M_DP_NA_1) || (rcvdType == M_DP_TB_1)) {
                return true;
            }

            break;

        case M_DP_TB_1:
            if ((rcvdType == M_DP_NA_1) || (rcvdType == M_DP_TA_1)) {
                return true;
            }

            break;

        case M_ME_NA_1:
            if ((rcvdType == M_ME_TA_1) || (rcvdType == M_ME_TD_1) || (rcvdType == M_ME_ND_1)) {
                return true;
            }

            break;

        case M_ME_TA_1:
            if ((rcvdType == M_ME_NA_1) || (rcvdType == M_ME_TD_1) || (rcvdType == M_ME_ND_1)) {
                return true;
            }

            break;

        case M_ME_TD_1:
            if ((rcvdType == M_ME_TA_1) || (rcvdType == M_ME_NA_1) || (rcvdType == M_ME_ND_1)) {
                return true;
            }

            break;

        case M_ME_ND_1:
            if ((rcvdType == M_ME_TA_1) || (rcvdType == M_ME_TD_1) || (rcvdType == M_ME_NA_1)) {
                return true;
            }

            break;

        case M_ME_NB_1:
            if ((rcvdType == M_ME_TB_1) || (rcvdType == M_ME_TE_1)) {
                return true;
            }

            break;

        case M_ME_TB_1:
            if ((rcvdType == M_ME_NB_1) || (rcvdType == M_ME_TE_1)) {
                return true;
            }

            break;

        case M_ME_TE_1:
            if ((rcvdType == M_ME_TB_1) || (rcvdType == M_ME_NB_1)) {
                return true;
            }

            break;

        case M_ME_NC_1:
            if ((rcvdType == M_ME_TC_1) || (rcvdType == M_ME_TF_1)) {
                return true;
            }

            break;

        case M_ME_TC_1:
            if ((rcvdType == M_ME_NC_1) || (rcvdType == M_ME_TF_1)) {
                return true;
            }

            break;

        case M_ME_TF_1:
            if ((rcvdType == M_ME_TC_1) || (rcvdType == M_ME_NC_1)) {
                return true;
            }

            break;


        case M_ST_NA_1:
            if ((rcvdType == M_ST_TA_1) || (rcvdType == M_ST_TB_1)) {
                return true;
            }

            break;

        case M_ST_TA_1:
            if ((rcvdType == M_ST_NA_1) || (rcvdType == M_ST_TB_1)) {
                return true;
            }

            break;

        case M_ST_TB_1:
            if ((rcvdType == M_ST_TA_1) || (rcvdType == M_ST_NA_1)) {
                return true;
            }

            break;


        default:
            //Type not supported
            break;
    }   

    return false;
}

template <class T>
T  IEC104Client::m_getConfigValueDefault(json configuration, json_pointer<json> path, T defaultValue)
{
    T typed_value = defaultValue;

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

template <class T>
T IEC104Client::m_getConfigValue(json configuration, json_pointer<json> path)
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

template <class T>
void IEC104Client::m_addData(CS101_ASDU asdu, vector<Datapoint*>& datapoints, int64_t ioa,
                             const std::string& dataname, const T value,
                             QualityDescriptor qd, CP56Time2a ts)
{
    auto* measure_features = new vector<Datapoint*>;

    measure_features->push_back(m_createDatapoint("do_type", mapAsduTypeIdStr[CS101_ASDU_getTypeID(asdu)]));

    measure_features->push_back(m_createDatapoint("do_ca", (int64_t)CS101_ASDU_getCA(asdu)));

    measure_features->push_back(m_createDatapoint("do_oa", (int64_t)CS101_ASDU_getOA(asdu)));

    measure_features->push_back(m_createDatapoint("do_cot", (int64_t)CS101_ASDU_getCOT(asdu)));

    measure_features->push_back(m_createDatapoint("do_test", (int64_t)CS101_ASDU_isTest(asdu)));

    measure_features->push_back(m_createDatapoint("do_negative", (int64_t)CS101_ASDU_isNegative(asdu)));

    measure_features->push_back(m_createDatapoint("do_ioa", (int64_t)ioa));

    measure_features->push_back(m_createDatapoint("do_value", value));

    measure_features->push_back(m_createDatapoint("do_quality_iv", (qd & IEC60870_QUALITY_INVALID) ? 1L : 0L));

    measure_features->push_back(m_createDatapoint("do_quality_bl", (qd & IEC60870_QUALITY_BLOCKED) ? 1L : 0L));

    measure_features->push_back(m_createDatapoint("do_quality_ov", (qd & IEC60870_QUALITY_OVERFLOW) ? 1L : 0L));

    measure_features->push_back(m_createDatapoint("do_quality_sb", (qd & IEC60870_QUALITY_SUBSTITUTED) ? 1L : 0L));

    measure_features->push_back(m_createDatapoint("do_quality_nt", (qd & IEC60870_QUALITY_NON_TOPICAL) ? 1L : 0L));

    if (ts) {
         measure_features->push_back(m_createDatapoint("do_ts", (int64_t)CP56Time2a_toMsTimestamp(ts)));

         measure_features->push_back(m_createDatapoint("do_ts_iv", (CP56Time2a_isInvalid(ts)) ? 1L : 0L));

         measure_features->push_back(m_createDatapoint("do_ts_su", (CP56Time2a_isSummerTime(ts)) ? 1L : 0L));

         measure_features->push_back(m_createDatapoint("do_ts_sub", (CP56Time2a_isSubstituted(ts)) ? 1L : 0L));
    }

    DatapointValue dpv(measure_features, true);

    datapoints.push_back(new Datapoint("data_object", dpv));
}

void IEC104Client::sendData(CS101_ASDU asdu, vector<Datapoint*> datapoints,
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

IEC104Client::IEC104Client(IEC104* iec104, nlohmann::json* stack_configuration,
                nlohmann::json* msg_configuration)
        : m_iec104(iec104),
            m_stack_configuration(stack_configuration),
            m_msg_configuration(msg_configuration)
{
    createDataExchangeDefinitions();
}

IEC104Client::~IEC104Client()
{
    for (auto const &element : exchangeDefinitions) {
        for (auto const &elem2 : element.second) {
            delete elem2.second;
        }
    }
}

void IEC104Client::createDataExchangeDefinitions()
{
    for (auto& element : (*m_msg_configuration)["asdu_list"])
    {
        int ca = m_getConfigValue<unsigned int>(element, "/ca"_json_pointer);
        int ioa = m_getConfigValue<unsigned int>(element, "/ioa"_json_pointer);
        std::string typeIdStr = m_getConfigValue<string>(element,
                                                   "/type_id"_json_pointer);

        std::string label = m_getConfigValue<string>(element,
                                                   "/label"_json_pointer);

        DataExchangeDefinition* def = new DataExchangeDefinition;

        def->ca = ca;
        def->ioa = ioa;
        def->label = label;
        def->typeId = 0;
        def->typeId = mapAsduTypeId[typeIdStr];

        exchangeDefinitions[ca][ioa] = def;
    }
}

std::string IEC104Client::m_checkExchangedDataLayer(int ca,
                                              int type_id,
                                              int ioa)
{
    auto& def = exchangeDefinitions[ca][ioa];

    if (def) {
        // check if message type is matching the exchange definition
        if (isMessageTypeMatching(def->typeId, type_id)) {
            return def->label;
        }
    }

    return "";
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
void IEC104Client::handleASDU(vector<string>& labels, vector<Datapoint*>& datapoints,
                        IEC104Client* mclient, CS101_ASDU asdu,
                        IEC104_ASDUHandler callback)
{
    IEC60870_5_TypeID asduID = CS101_ASDU_getTypeID(asdu);

    int ca = CS101_ASDU_getCA(asdu);
    string label;

    for (int i = 0; i < CS101_ASDU_getNumberOfElements(asdu); i++)
    {
        InformationObject io = CS101_ASDU_getElement(asdu, i);
        int ioa = InformationObject_getObjectAddress(io);

        if (!(label = mclient->m_checkExchangedDataLayer(ca, asduID, ioa)).empty())
        { 
            callback(datapoints, label, mclient, ca, asdu, io, ioa);
            labels.push_back(label);
        }
    }
}

// Each of the following function handle a specific type of ASDU. They cast the
// contained IO into a specific object that is strictly linked to the type
// for example a MeasuredValueScaled is type M_ME_NB_1
void IEC104Client::handleM_ME_NB_1(vector<Datapoint*>& datapoints, string& label,
                             IEC104Client* mclient, unsigned int ca,
                             CS101_ASDU asdu, InformationObject io,
                             uint64_t ioa)
{
    auto io_casted = (MeasuredValueScaled)io;
    int64_t value =
        MeasuredValueScaled_getValue((MeasuredValueScaled)io_casted);
    QualityDescriptor qd = MeasuredValueScaled_getQuality(io_casted);
    mclient->m_addData(asdu, datapoints, ioa, label, value, qd);

    MeasuredValueScaled_destroy(io_casted);
}

void IEC104Client::handleM_SP_NA_1(vector<Datapoint*>& datapoints, string& label,
                             IEC104Client* mclient, unsigned int ca,
                             CS101_ASDU asdu, InformationObject io,
                             uint64_t ioa)
{
    auto io_casted = (SinglePointInformation)io;
    int64_t value =
        SinglePointInformation_getValue((SinglePointInformation)io_casted);
    QualityDescriptor qd =
        SinglePointInformation_getQuality((SinglePointInformation)io_casted);
    mclient->m_addData(asdu, datapoints, ioa, label, value, qd);

    SinglePointInformation_destroy(io_casted);
}

void IEC104Client::handleM_SP_TB_1(vector<Datapoint*>& datapoints, string& label,
                             IEC104Client* mclient, unsigned int ca,
                             CS101_ASDU asdu, InformationObject io,
                             uint64_t ioa)
{
    auto io_casted = (SinglePointWithCP56Time2a)io;
    int64_t value =
        SinglePointInformation_getValue((SinglePointInformation)io_casted);
    QualityDescriptor qd =
        SinglePointInformation_getQuality((SinglePointInformation)io_casted);


    CP56Time2a ts = SinglePointWithCP56Time2a_getTimestamp(io_casted);
    bool is_invalid = CP56Time2a_isInvalid(ts);
    //if (m_tsiv == "PROCESS" || !is_invalid)
        mclient->m_addData(asdu, datapoints, ioa, label, value, qd, ts);

    SinglePointWithCP56Time2a_destroy(io_casted);
}

void IEC104Client::handleM_DP_NA_1(vector<Datapoint*>& datapoints, string& label,
                             IEC104Client* mclient, unsigned int ca,
                             CS101_ASDU asdu, InformationObject io,
                             uint64_t ioa)
{
    auto io_casted = (DoublePointInformation)io;
    int64_t value =
        DoublePointInformation_getValue((DoublePointInformation)io_casted);
    QualityDescriptor qd =
        DoublePointInformation_getQuality((DoublePointInformation)io_casted);
    mclient->m_addData(asdu, datapoints, ioa, label, value, qd);

    DoublePointInformation_destroy(io_casted);
}

void IEC104Client::handleM_DP_TB_1(vector<Datapoint*>& datapoints, string& label,
                             IEC104Client* mclient, unsigned int ca,
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
    //if (m_tsiv == "PROCESS" || !is_invalid)
        mclient->m_addData(asdu, datapoints, ioa, label, value, qd, ts);

    DoublePointWithCP56Time2a_destroy(io_casted);
}

void IEC104Client::handleM_ST_NA_1(vector<Datapoint*>& datapoints, string& label,
                             IEC104Client* mclient, unsigned int ca,
                             CS101_ASDU asdu, InformationObject io,
                             uint64_t ioa)
{
    auto io_casted = (StepPositionInformation)io;
    int64_t value =
        StepPositionInformation_getValue((StepPositionInformation)io_casted);
    QualityDescriptor qd =
        StepPositionInformation_getQuality((StepPositionInformation)io_casted);
    mclient->m_addData(asdu, datapoints, ioa, label, value, qd);

    StepPositionInformation_destroy(io_casted);
}

void IEC104Client::handleM_ST_TB_1(vector<Datapoint*>& datapoints, string& label,
                             IEC104Client* mclient, unsigned int ca,
                             CS101_ASDU asdu, InformationObject io,
                             uint64_t ioa)
{
    auto io_casted = (StepPositionWithCP56Time2a)io;
    int64_t value =
        StepPositionInformation_getValue((StepPositionInformation)io_casted);
    QualityDescriptor qd =
        StepPositionInformation_getQuality((StepPositionInformation)io_casted);
    if (mclient->m_comm_wttag)
    {
        CP56Time2a ts = StepPositionWithCP56Time2a_getTimestamp(io_casted);
        bool is_invalid = CP56Time2a_isInvalid(ts);
        //if (m_tsiv == "PROCESS" || !is_invalid)
            mclient->m_addData(asdu, datapoints, ioa, label, value, qd, ts);
    }
    else
        mclient->m_addData(asdu, datapoints, ioa, label, value, qd);

    StepPositionWithCP56Time2a_destroy(io_casted);
}

void IEC104Client::handleM_ME_NA_1(vector<Datapoint*>& datapoints, string& label,
                             IEC104Client* mclient, unsigned int ca,
                             CS101_ASDU asdu, InformationObject io,
                             uint64_t ioa)
{
    auto io_casted = (MeasuredValueNormalized)io;
    float value =
        MeasuredValueNormalized_getValue((MeasuredValueNormalized)io_casted);
    QualityDescriptor qd =
        MeasuredValueNormalized_getQuality((MeasuredValueNormalized)io_casted);
    mclient->m_addData(asdu, datapoints, ioa, label, value, qd);

    MeasuredValueNormalized_destroy(io_casted);
}

void IEC104Client::handleM_ME_TD_1(vector<Datapoint*>& datapoints, string& label,
                             IEC104Client* mclient, unsigned int ca,
                             CS101_ASDU asdu, InformationObject io,
                             uint64_t ioa)
{
    auto io_casted = (MeasuredValueNormalizedWithCP56Time2a)io;
    float value =
        MeasuredValueNormalized_getValue((MeasuredValueNormalized)io_casted);
    QualityDescriptor qd =
        MeasuredValueNormalized_getQuality((MeasuredValueNormalized)io_casted);
    if (mclient->m_comm_wttag)
    {
        CP56Time2a ts =
            MeasuredValueNormalizedWithCP56Time2a_getTimestamp(io_casted);
        bool is_invalid = CP56Time2a_isInvalid(ts);
        //if (m_tsiv == "PROCESS" || !is_invalid)
            mclient->m_addData(asdu, datapoints, ioa, label, value, qd, ts);
    }
    else
        mclient->m_addData(asdu, datapoints, ioa, label, value, qd);

    MeasuredValueNormalizedWithCP56Time2a_destroy(io_casted);
}

void IEC104Client::handleM_ME_TE_1(vector<Datapoint*>& datapoints, string& label,
                             IEC104Client* mclient, unsigned int ca,
                             CS101_ASDU asdu, InformationObject io,
                             uint64_t ioa)
{
    auto io_casted = (MeasuredValueScaledWithCP56Time2a)io;
    int64_t value =
        MeasuredValueScaled_getValue((MeasuredValueScaled)io_casted);
    QualityDescriptor qd =
        MeasuredValueScaled_getQuality((MeasuredValueScaled)io_casted);
    if (mclient->m_comm_wttag)
    {
        CP56Time2a ts =
            MeasuredValueScaledWithCP56Time2a_getTimestamp(io_casted);
        bool is_invalid = CP56Time2a_isInvalid(ts);
        //if (m_tsiv == "PROCESS" || !is_invalid)
            mclient->m_addData(asdu, datapoints, ioa, label, value, qd, ts);
    }
    else
        mclient->m_addData(asdu, datapoints, ioa, label, value, qd);

    MeasuredValueScaledWithCP56Time2a_destroy(io_casted);
}

void IEC104Client::handleM_ME_NC_1(vector<Datapoint*>& datapoints, string& label,
                             IEC104Client* mclient, unsigned int ca,
                             CS101_ASDU asdu, InformationObject io,
                             uint64_t ioa)
{
    auto io_casted = (MeasuredValueShort)io;
    float value = MeasuredValueShort_getValue((MeasuredValueShort)io_casted);
    QualityDescriptor qd =
        MeasuredValueShort_getQuality((MeasuredValueShort)io_casted);
    mclient->m_addData(asdu, datapoints, ioa, label, value, qd);

    MeasuredValueShort_destroy(io_casted);
}

void IEC104Client::handleM_ME_TF_1(vector<Datapoint*>& datapoints, string& label,
                             IEC104Client* mclient, unsigned int ca,
                             CS101_ASDU asdu, InformationObject io,
                             uint64_t ioa)
{
    auto io_casted = (MeasuredValueShortWithCP56Time2a)io;
    float value = MeasuredValueShort_getValue((MeasuredValueShort)io_casted);
    QualityDescriptor qd =
        MeasuredValueShort_getQuality((MeasuredValueShort)io_casted);
    if (mclient->m_comm_wttag)
    {
        CP56Time2a ts =
            MeasuredValueShortWithCP56Time2a_getTimestamp(io_casted);
        bool is_invalid = CP56Time2a_isInvalid(ts);
        //if (m_tsiv == "PROCESS" || !is_invalid)
            mclient->m_addData(asdu, datapoints, ioa, label, value, qd, ts);
    }
    else
        mclient->m_addData(asdu, datapoints, ioa, label, value, qd);

    MeasuredValueShortWithCP56Time2a_destroy(io_casted);
}

void IEC104Client::handleC_SC_TA_1(vector<Datapoint*>& datapoints, string& label,
                             IEC104Client* mclient, unsigned int ca,
                             CS101_ASDU asdu, InformationObject io,
                             uint64_t ioa)
{
    auto io_casted = (SingleCommandWithCP56Time2a)io;
    int64_t state = SingleCommand_getState((SingleCommand)io_casted);

    QualifierOfCommand qu = SingleCommand_getQU((SingleCommand)io_casted);

    if (mclient->m_comm_wttag)
    {
        CP56Time2a ts = SingleCommandWithCP56Time2a_getTimestamp(io_casted);
        bool is_invalid = CP56Time2a_isInvalid(ts);
        //if (m_tsiv == "PROCESS" || !is_invalid)
            mclient->m_addData(asdu, datapoints, ioa, label, state, qu, ts);
    }
    else
        mclient->m_addData(asdu, datapoints, ioa, label, state, qu);

    SingleCommandWithCP56Time2a_destroy(io_casted);
}

void IEC104Client::handleC_DC_TA_1(vector<Datapoint*>& datapoints, string& label,
                             IEC104Client* mclient, unsigned int ca,
                             CS101_ASDU asdu, InformationObject io,
                             uint64_t ioa)
{
    auto io_casted = (DoubleCommandWithCP56Time2a)io;
    int64_t state = DoubleCommand_getState((DoubleCommand)io_casted);

    QualifierOfCommand qu = DoubleCommand_getQU((DoubleCommand)io_casted);

    if (mclient->m_comm_wttag)
    {
        CP56Time2a ts = DoubleCommandWithCP56Time2a_getTimestamp(io_casted);
        bool is_invalid = CP56Time2a_isInvalid(ts);
        //if (m_tsiv == "PROCESS" || !is_invalid)
            mclient->m_addData(asdu, datapoints, ioa, label, state, qu, ts);
    }
    else
        mclient->m_addData(asdu, datapoints, ioa, label, state, qu);

    DoubleCommandWithCP56Time2a_destroy(io_casted);
}

bool IEC104Client::m_asduReceivedHandler(void* parameter, int address,
                                   CS101_ASDU asdu)
{
    IEC104Client* self = static_cast<IEC104Client*>(parameter);

    vector<Datapoint*> datapoints;
    vector<string> labels;

    switch (CS101_ASDU_getTypeID(asdu))
    {
        case M_ME_NB_1:
            handleASDU(labels, datapoints, self, asdu, handleM_ME_NB_1);
            break;
        case M_SP_NA_1:
            handleASDU(labels, datapoints, self, asdu, handleM_SP_NA_1);
            break;
        case M_SP_TB_1:
            handleASDU(labels, datapoints, self, asdu, handleM_SP_TB_1);
            break;
        case M_DP_NA_1:
            handleASDU(labels, datapoints, self, asdu, handleM_DP_NA_1);
            break;
        case M_DP_TB_1:
            handleASDU(labels, datapoints, self, asdu, handleM_DP_TB_1);
            break;
        case M_ST_NA_1:
            handleASDU(labels, datapoints, self, asdu, handleM_ST_NA_1);
            break;
        case M_ST_TB_1:
            handleASDU(labels, datapoints, self, asdu, handleM_ST_TB_1);
            break;
        case M_ME_NA_1:
            handleASDU(labels, datapoints, self, asdu, handleM_ME_NA_1);
            break;
        case M_ME_TD_1:
            handleASDU(labels, datapoints, self, asdu, handleM_ME_TD_1);
            break;
        case M_ME_TE_1:
            handleASDU(labels, datapoints, self, asdu, handleM_ME_TE_1);
            break;
        case M_ME_NC_1:
            handleASDU(labels, datapoints, self, asdu, handleM_ME_NC_1);
            break;
        case M_ME_TF_1:
            handleASDU(labels, datapoints, self, asdu, handleM_ME_TF_1);
            break;
        case M_EI_NA_1:
            Logger::getLogger()->info("Received end of initialization");
            break;

        case C_CS_NA_1:
            Logger::getLogger()->info("Received time sync response");
            printf("Received time sync response\n");

            if (self->m_timeSyncCommandSent == true) {

                if (CS101_ASDU_getCOT(asdu) == CS101_COT_ACTIVATION_CON) {
                    if (CS101_ASDU_isNegative(asdu) == false) {
                        self->m_timeSyncCommandSent = false;
                        self->m_timeSynchronized = true;

                        printf("time synchronized\n");

                        if (self->m_timeSyncPeriod > 0) {
                            self->m_nextTimeSync = getMonotonicTimeInMs() + (self->m_timeSyncPeriod * 1000);
                        }
                    }
                    else {
                        printf("time synchonizatation failed\n");
                    }
                }
                else if (CS101_ASDU_getCOT(asdu) == CS101_COT_UNKNOWN_TYPE_ID) {

                    Logger::getLogger()->warn("Time synchronization not supported by remote");

                    printf("Time synchronization not supported by remote\n");

                    self->m_timeSyncCommandSent = false;
                    self->m_timeSynchronized = true;
                }

            }
            else {
                if (CS101_ASDU_getCOT(asdu) == CS101_COT_ACTIVATION_CON) {
                    Logger::getLogger()->warn("Unexpected time sync response");
                    printf("Unexpected time sync response!\n");
                }
                else if (CS101_ASDU_getCOT(asdu) == CS101_COT_SPONTANEOUS) {
                    Logger::getLogger()->warn("Received remote clock time");
                    printf("Received remote clock time\n");
                }
                else {
                    Logger::getLogger()->warn("Unexpected time sync message");
                    printf("Unexpected time sync message\n");
                }
            }

            break;

        case C_IC_NA_1:
            Logger::getLogger()->info("General interrogation command");
            break;
        case C_TS_TA_1:
            Logger::getLogger()->info("Test command with time tag CP56Time2a");
            break;
        case C_SC_TA_1:
            handleASDU(labels, datapoints, self, asdu, handleC_SC_TA_1);
            break;
        case C_DC_TA_1:
            handleASDU(labels, datapoints, self, asdu, handleC_DC_TA_1);
            break;
        default:
            Logger::getLogger()->error("Type of message not supported");
            return false;
    }

    if (!datapoints.empty())
        self->sendData(asdu, datapoints, labels);

    return true;
}

void IEC104Client::m_connectionHandler(void* parameter, CS104_Connection connection,
                                 CS104_ConnectionEvent event)
{
    IEC104Client* self = static_cast<IEC104Client*>(parameter);

    Logger::getLogger()->info("Connection state changed: " + std::to_string(event));

    printf("\nConnection state changed: %i\n", event);

    if (event == CS104_CONNECTION_CLOSED)
    {
        self->m_connectionState = CON_STATE_CLOSED;
    }
    else if (event == CS104_CONNECTION_OPENED) 
    {
        self->m_connectionState = CON_STATE_CONNECTED_INACTIVE;
    }
    else if (event == CS104_CONNECTION_STARTDT_CON_RECEIVED)
    {
        self->m_nextTimeSync = getMonotonicTimeInMs();
        self->m_timeSynchronized = false;
        self->m_timeSyncCommandSent = false;

        self->m_connectionState = CON_STATE_CONNECTED_ACTIVE;
    }
    else if (event == CS104_CONNECTION_STOPDT_CON_RECEIVED)
    {
        self->m_connectionState = CON_STATE_CONNECTED_INACTIVE;
    }
}

bool IEC104Client::prepareConnection()
{
    CS104_Connection new_connection = nullptr;

    for (auto& path_element : (*m_stack_configuration)["transport_layer"]["connection"]["path"])
    {
        string ip =
            m_getConfigValue<string>(path_element, "/srv_ip"_json_pointer);

        int port = m_getConfigValue<int>(path_element, "/port"_json_pointer);

        if (m_getConfigValue<bool>(
                (*m_stack_configuration),
                "/transport_layer/connection/tls"_json_pointer))
        {
            Logger::getLogger()->error("TLS not supported yet");
        }
        else
        {
            new_connection = CS104_Connection_create(ip.c_str(), port);
            Logger::getLogger()->info("Connection tp %s:%i created", ip.c_str(), port);
        }

        if (new_connection) {

            m_iec104->prepareParameters(new_connection);

            CS104_Connection_setConnectionHandler(
                new_connection, m_connectionHandler, static_cast<void*>(this));
            CS104_Connection_setASDUReceivedHandler(new_connection,
                                                    m_asduReceivedHandler,
                                                    static_cast<void*>(this));

            CS104_Connection_connectAsync(new_connection);
        }

        // // If conn_all == false, only use the first path
        // if (!m_getConfigValue<bool>(m_stack_configuration,
        //                             "/transport_layer/conn_all"_json_pointer))
        // {
        //     break;
        // }

        // only use first part -> TODO implement redundancy handling
        break;
    }

    auto& appLayerConfig = (*m_stack_configuration)["application_layer"];

    m_timeSyncEnabled = m_getConfigValueDefault<bool>(appLayerConfig, "/time_sync"_json_pointer, false);
    m_timeSyncPeriod = m_getConfigValueDefault<int>(appLayerConfig, "/time_sync_period"_json_pointer, 0);

    printf("timessync_period: %i\n", m_timeSyncPeriod);

    if (new_connection) {
        m_connection = new_connection;
        return true;
    }
    else {
        m_connection = nullptr;
        return false;
    }
}

void IEC104Client::performPeriodicTasks()
{
    /* do time synchroniation when enabled */
    if (m_timeSyncEnabled) {

        bool sendTimeSyncCommand = false;

        /* send first time sync after connection was activated */
        if ((m_timeSynchronized == false) && (m_timeSyncCommandSent == false)) {
            sendTimeSyncCommand = true;
        }
        
        /* send periodic time sync command when configured */
        if ((m_timeSynchronized == true) && (m_timeSyncCommandSent == false)) {
            if (m_timeSyncPeriod > 0) {
                if (getMonotonicTimeInMs() >= m_nextTimeSync) {
                    sendTimeSyncCommand = true;
                }
            }
        }

        if (sendTimeSyncCommand) {
            struct sCP56Time2a ts;
            
            CP56Time2a_createFromMsTimestamp(&ts, Hal_getTimeInMs());

            int ca = timeSyncCA();

            if (ca == -1)
                ca = defaultCA();

            if (ca == -1)
                ca = broadcastCA();

            if (CS104_Connection_sendClockSyncCommand(m_connection, ca, &ts)) {
                Logger::getLogger()->info("Sent clock sync command ...");
                printf("Sent clock sync command ...\n");

                m_timeSyncCommandSent = true;
            }
            else {
                printf("Failed to send clock sync command!\n");
            }
        }
    }

    
    
}

void IEC104Client::_conThread()
{
    while (m_started) 
    {
        switch (m_connectionState) {
            case CON_STATE_IDLE:
                {
                    m_startDtSent = false;

                        if (m_connection != nullptr) {
                            CS104_Connection_destroy(m_connection);

                            m_connection = nullptr;
                        }

                    if (prepareConnection()) {
                        m_connectionState = CON_STATE_CONNECTING;

                        Logger::getLogger()->info("Connecting");
                    }
                    else {
                        m_connectionState = CON_STATE_FATAL_ERROR;
                        Logger::getLogger()->error("Fatal configuration error");
                    }
                    
                }
                break;

            case CON_STATE_CONNECTING:
                /* wait for connected event or timeout */

                

                break;

            case CON_STATE_CONNECTED_INACTIVE:

                if (m_startDtSent == false) {
                    CS104_Connection_sendStartDT(m_connection);
                    m_startDtSent = true;
                }

                break;

            case CON_STATE_CONNECTED_ACTIVE:

                performPeriodicTasks();
                //TODO periodic tasks

                break;

            case CON_STATE_CLOSED:

                // start delay timer for reconnect
                
                m_delayExpirationTime = getMonotonicTimeInMs() + 10000;
                m_connectionState = CON_STATE_WAIT_FOR_RECONNECT;

                break;

            case CON_STATE_WAIT_FOR_RECONNECT:

                // when timeout expired switch to idle state

                if (getMonotonicTimeInMs() >= m_delayExpirationTime) {
                    m_connectionState = CON_STATE_IDLE;
                }

                break;

            case CON_STATE_FATAL_ERROR:
                /* stay in this state until stop is called */
                break;

        }

        Thread_sleep(100);
    }
}

void IEC104Client::start()
{
    if (m_started == false) {
        m_started = true;

        m_conThread = new std::thread(&IEC104Client::_conThread, this);
    } 
}

void IEC104Client::stop()
{
    if (m_started == true) {
        m_started = false;

        if (m_conThread != nullptr) {
            m_conThread->join();
            delete m_conThread;
            m_conThread = nullptr;
        }
    }
}

bool IEC104Client::sendInterrogationCommand(int ca)
{
    // send interrogation request over active connection
    bool success = false;

    if ((m_connection != nullptr) && (m_connectionState == CON_STATE_CONNECTED_ACTIVE)) 
    {
        if (CS104_Connection_sendInterrogationCommand(m_connection, CS101_COT_ACTIVATION, ca, IEC60870_QOI_STATION)) {
            Logger::getLogger()->info("Interrogation command sent");
            success = true;
        }
        else {
            Logger::getLogger()->warn("Failed to send interrogation command");
        }
    }

    return success;
}

bool IEC104Client::sendSingleCommand(int ca, int ioa, bool value, bool withTime, bool select)
{
    // send single command over active connection
    bool success = false;

    int typeID = C_SC_NA_1;

    if (withTime)
        typeID = C_SC_TA_1;

    // check if the data point is in the exchange configuration
    if (m_checkExchangedDataLayer(ca, typeID, ioa) == "") {
        Logger::getLogger()->error("Failed to send command - no such data point");
        printf("Failed to send command - no such data point");

        return false;
    }

    if ((m_connection != nullptr) && (m_connectionState == CON_STATE_CONNECTED_ACTIVE)) 
    {
        InformationObject sc = nullptr;

        if (withTime) {
            struct sCP56Time2a ts;
            
            CP56Time2a_createFromMsTimestamp(&ts, Hal_getTimeInMs());

            sc = (InformationObject)SingleCommandWithCP56Time2a_create(NULL, ioa, value, select, 0, &ts);
        }
        else {
            sc = (InformationObject)SingleCommand_create(NULL, ioa, value, select, 0);
        }

        if (sc) {
            if (CS104_Connection_sendProcessCommandEx(m_connection, CS101_COT_ACTIVATION, ca, sc)) {
                Logger::getLogger()->info("single command sent");
                success = true;
            }
        }  
    }

    if (!success) Logger::getLogger()->warn("Failed to send single command");

    return success;
}

bool IEC104Client::sendDoubleCommand(int ca, int ioa, int value, bool withTime, bool select)
{
    // send double command over active connection
    bool success = false;

    int typeID = C_DC_NA_1;

    if (withTime)
        typeID = C_DC_TA_1;

    // check if the data point is in the exchange configuration
    if (m_checkExchangedDataLayer(ca, typeID, ioa) == "") {
        Logger::getLogger()->error("Failed to send command - no such data point");
        printf("Failed to send command - no such data point");

        return false;
    }

    if ((m_connection != nullptr) && (m_connectionState == CON_STATE_CONNECTED_ACTIVE)) 
    {
        InformationObject sc = nullptr;

        if (withTime) {
            struct sCP56Time2a ts;
            
            CP56Time2a_createFromMsTimestamp(&ts, Hal_getTimeInMs());

            sc = (InformationObject)DoubleCommandWithCP56Time2a_create(NULL, ioa, value, select, 0, &ts);
        }
        else {
            sc = (InformationObject)DoubleCommand_create(NULL, ioa, value, select, 0);
        }

        if (sc) {
            if (CS104_Connection_sendProcessCommandEx(m_connection, CS101_COT_ACTIVATION, ca, sc)) {
                Logger::getLogger()->info("double command sent");
                success = true;
            }
        }  
    }

    if (!success) Logger::getLogger()->warn("Failed to send double command");

    return success;
}

bool IEC104Client::sendStepCommand(int ca, int ioa, int value, bool withTime, bool select)
{
    // send step command over active connection
    bool success = false;

    int typeID = C_RC_NA_1;

    if (withTime)
        typeID = C_RC_TA_1;

    // check if the data point is in the exchange configuration
    if (m_checkExchangedDataLayer(ca, typeID, ioa) == "") {
        Logger::getLogger()->error("Failed to send command - no such data point");
        printf("Failed to send command - no such data point");

        return false;
    }

    if ((m_connection != nullptr) && (m_connectionState == CON_STATE_CONNECTED_ACTIVE)) 
    {
        InformationObject sc = nullptr;

        if (withTime) {
            struct sCP56Time2a ts;
            
            CP56Time2a_createFromMsTimestamp(&ts, Hal_getTimeInMs());

            sc = (InformationObject)StepCommandWithCP56Time2a_create(NULL, ioa, (StepCommandValue)value, select, 0, &ts);
        }
        else {
            sc = (InformationObject)StepCommand_create(NULL, ioa, (StepCommandValue)value, select, 0);
        }

        if (sc) {
            if (CS104_Connection_sendProcessCommandEx(m_connection, CS101_COT_ACTIVATION, ca, sc)) {
                Logger::getLogger()->info("step command sent");
                success = true;
            }
        }  
    }

    if (!success) Logger::getLogger()->warn("Failed to send step command");

    return success;
}

bool IEC104Client::sendSetpointNormalized(int ca, int ioa, float value, bool withTime)
{
    // send setpoint command normalized over active connection
    bool success = false;

    int typeID = C_SE_NA_1;

    if (withTime)
        typeID = C_SE_TA_1;

    // check if the data point is in the exchange configuration
    if (m_checkExchangedDataLayer(ca, typeID, ioa) == "") {
        Logger::getLogger()->error("Failed to send command - no such data point");
        printf("Failed to send command - no such data point");

        return false;
    }

    if ((m_connection != nullptr) && (m_connectionState == CON_STATE_CONNECTED_ACTIVE)) 
    {
        InformationObject sc = nullptr;

        if (withTime) {
            struct sCP56Time2a ts;
            
            CP56Time2a_createFromMsTimestamp(&ts, Hal_getTimeInMs());

            sc = (InformationObject)SetpointCommandNormalizedWithCP56Time2a_create(NULL, ioa, value, false, 0, &ts);
        }
        else {
            sc = (InformationObject)SetpointCommandNormalized_create(NULL, ioa, value, false, 0);
        }

        if (sc) {
            if (CS104_Connection_sendProcessCommandEx(m_connection, CS101_COT_ACTIVATION, ca, sc)) {
                Logger::getLogger()->info("setpoint(normalized) sent");
                success = true;
            }
        }  
    }

    if (!success) Logger::getLogger()->warn("Failed to send setpoint(normalized)");

    return success;
}

bool IEC104Client::sendSetpointScaled(int ca, int ioa, int value, bool withTime)
{
    // send setpoint command scaled over active connection
    bool success = false;

    int typeID = C_SE_NB_1;

    if (withTime)
        typeID = C_SE_TB_1;

    // check if the data point is in the exchange configuration
    if (m_checkExchangedDataLayer(ca, typeID, ioa) == "") {
        Logger::getLogger()->error("Failed to send command - no such data point");
        printf("Failed to send command - no such data point");

        return false;
    }

    if ((m_connection != nullptr) && (m_connectionState == CON_STATE_CONNECTED_ACTIVE)) 
    {
        InformationObject sc = nullptr;

        if (withTime) {
            struct sCP56Time2a ts;
            
            CP56Time2a_createFromMsTimestamp(&ts, Hal_getTimeInMs());

            sc = (InformationObject)SetpointCommandScaledWithCP56Time2a_create(NULL, ioa, value, false, 0, &ts);
        }
        else {
            sc = (InformationObject)SetpointCommandScaled_create(NULL, ioa, value, false, 0);
        }

        if (sc) {
            if (CS104_Connection_sendProcessCommandEx(m_connection, CS101_COT_ACTIVATION, ca, sc)) {
                Logger::getLogger()->info("setpoint(scaled) sent");
                success = true;
            }
        }  
    }

    if (!success) Logger::getLogger()->warn("Failed to send setpoint(scaled)");

    return success;
}

bool IEC104Client::sendSetpointShort(int ca, int ioa, float value, bool withTime)
{
    // send setpoint command short over active connection
    bool success = false;

    int typeID = C_SE_NC_1;

    if (withTime)
        typeID = C_SE_TC_1;

    // check if the data point is in the exchange configuration
    if (m_checkExchangedDataLayer(ca, typeID, ioa) == "") {
        Logger::getLogger()->error("Failed to send command - no such data point");
        printf("Failed to send command - no such data point");

        return false;
    }

    if ((m_connection != nullptr) && (m_connectionState == CON_STATE_CONNECTED_ACTIVE)) 
    {
        InformationObject sc = nullptr;

        if (withTime) {
            struct sCP56Time2a ts;
            
            CP56Time2a_createFromMsTimestamp(&ts, Hal_getTimeInMs());

            sc = (InformationObject)SetpointCommandShortWithCP56Time2a_create(NULL, ioa, value, false, 0, &ts);
        }
        else {
            sc = (InformationObject)SetpointCommandShort_create(NULL, ioa, value, false, 0);
        }

        if (sc) {
            if (CS104_Connection_sendProcessCommandEx(m_connection, CS101_COT_ACTIVATION, ca, sc)) {
                Logger::getLogger()->info("setpoint(short) sent");
                success = true;
            }
        }  
    }

    if (!success) Logger::getLogger()->warn("Failed to send setpoint(short)");

    return success;
}


