/*
 * Fledge south service plugin
 *
 * Released under the Apache 2.0 Licence
 *
 * Author: Estelle Chigot, Lucas Barret
 */

#include <reading.h>
#include <logger.h>
#include "include/iec104.h"

using namespace std;


/** Constructor for the iec104 plugin */
IEC104::IEC104(const char *ip, uint16_t port) :
m_connected(false),
m_client(nullptr)
{
    setIp(ip);
    setPort(port);
}

/** Connection event handler */
void IEC104::connectionHandler (void* parameter, CS104_Connection connection, CS104_ConnectionEvent event)
{
    if (event == CS104_CONNECTION_CLOSED)
    {
        Logger::getLogger()->warn("CONNECTION LOST... Reconnecting");
        auto iec104 = (IEC104*)parameter;
        iec104->m_connected = false;
        iec104->restart();
    }
}

/**
 * Handle ASDU message
 * For CS104 the address parameter has to be ignored
 */
bool IEC104::asduReceivedHandler (void* parameter, int address, CS101_ASDU asdu)
{
    vector<Datapoint*> datapoints;
    auto mclient = static_cast<IEC104Client*>(parameter);

    int i;
    switch (CS101_ASDU_getTypeID(asdu)) {
        case M_ME_NB_1:
            for (i = 0; i < CS101_ASDU_getNumberOfElements(asdu); i++) {
                auto io = (MeasuredValueScaled) CS101_ASDU_getElement(asdu, i);
                long int value = MeasuredValueScaled_getValue((MeasuredValueScaled) io);
                QualityDescriptor qd = MeasuredValueScaled_getQuality(io);
                IEC104Client::addData(datapoints, "M_ME_NB_1", value, qd);

                MeasuredValueScaled_destroy(io);
            }
            break;
        case M_SP_NA_1:
            for (i = 0; i < CS101_ASDU_getNumberOfElements(asdu); i++) {
                auto io = (SinglePointInformation) CS101_ASDU_getElement(asdu, i);
                long int value = SinglePointInformation_getValue((SinglePointInformation) io);
                QualityDescriptor qd = SinglePointInformation_getQuality((SinglePointInformation) io);
                IEC104Client::addData(datapoints, "M_SP_NA_1", value, qd);

                SinglePointInformation_destroy(io);
            }
            break;
        case M_SP_TB_1:
            for (i = 0; i < CS101_ASDU_getNumberOfElements(asdu); i++) {
                auto io = (SinglePointWithCP56Time2a) CS101_ASDU_getElement(asdu, i);
                long int value = SinglePointInformation_getValue((SinglePointInformation) io);
                QualityDescriptor qd = SinglePointInformation_getQuality((SinglePointInformation) io);
                CP56Time2a ts = SinglePointWithCP56Time2a_getTimestamp(io);
                bool is_invalid = CP56Time2a_isInvalid(ts);
                IEC104Client::addData(datapoints, "M_SP_TB_1", value, qd, ts, is_invalid);

                SinglePointWithCP56Time2a_destroy(io);
            }
            break;
        case M_DP_NA_1:
            for (i = 0; i < CS101_ASDU_getNumberOfElements(asdu); i++) {
                auto io = (DoublePointInformation) CS101_ASDU_getElement(asdu, i);
                long int value = DoublePointInformation_getValue((DoublePointInformation) io);
                QualityDescriptor qd = DoublePointInformation_getQuality((DoublePointInformation) io);
                IEC104Client::addData(datapoints, "M_DP_NA_1", value, qd);

                DoublePointInformation_destroy(io);
            }
            break;
        case M_DP_TB_1:
            for (i = 0; i < CS101_ASDU_getNumberOfElements(asdu); i++) {
                auto io = (DoublePointWithCP56Time2a) CS101_ASDU_getElement(asdu, i);
                long int value = DoublePointInformation_getValue((DoublePointInformation) io);
                QualityDescriptor qd = DoublePointInformation_getQuality((DoublePointInformation) io);
                CP56Time2a ts = DoublePointWithCP56Time2a_getTimestamp(io);
                bool is_invalid = CP56Time2a_isInvalid(ts);
                IEC104Client::addData(datapoints, "M_DP_TB_1", value, qd, ts, is_invalid);

                DoublePointWithCP56Time2a_destroy(io);
            }
            break;
        case M_ST_NA_1:
            for (i = 0; i < CS101_ASDU_getNumberOfElements(asdu); i++) {
                auto io = (StepPositionInformation) CS101_ASDU_getElement(asdu, i);
                long int value = StepPositionInformation_getValue((StepPositionInformation) io);
                QualityDescriptor qd = StepPositionInformation_getQuality((StepPositionInformation) io);
                IEC104Client::addData(datapoints, "M_ST_NA_1", value, qd);

                StepPositionInformation_destroy(io);
            }
            break;
        case M_ST_TB_1:
            for (i = 0; i < CS101_ASDU_getNumberOfElements(asdu); i++) {
                auto io = (StepPositionWithCP56Time2a) CS101_ASDU_getElement(asdu, i);
                long int value = StepPositionInformation_getValue((StepPositionInformation) io);
                QualityDescriptor qd = StepPositionInformation_getQuality((StepPositionInformation) io);
                CP56Time2a ts = StepPositionWithCP56Time2a_getTimestamp(io);
                bool is_invalid = CP56Time2a_isInvalid(ts);
                IEC104Client::addData(datapoints, "M_ST_TB_1", value, qd, ts, is_invalid);

                StepPositionWithCP56Time2a_destroy(io);
            }
            break;
        case M_ME_NA_1:
            for (i = 0; i < CS101_ASDU_getNumberOfElements(asdu); i++) {
                auto io = (MeasuredValueNormalized) CS101_ASDU_getElement(asdu, i);
                float value = MeasuredValueNormalized_getValue((MeasuredValueNormalized) io);
                QualityDescriptor qd = MeasuredValueNormalized_getQuality((MeasuredValueNormalized) io);
                IEC104Client::addData(datapoints, "M_ME_NA_1", value, qd);

                MeasuredValueNormalized_destroy(io);
            }
            break;
        case M_ME_TD_1:
            for (i = 0; i < CS101_ASDU_getNumberOfElements(asdu); i++) {
                auto io = (MeasuredValueNormalizedWithCP56Time2a) CS101_ASDU_getElement(asdu, i);
                float value = MeasuredValueNormalized_getValue((MeasuredValueNormalized) io);
                QualityDescriptor qd = MeasuredValueNormalized_getQuality((MeasuredValueNormalized) io);
                CP56Time2a ts = MeasuredValueNormalizedWithCP56Time2a_getTimestamp(io);
                bool is_invalid = CP56Time2a_isInvalid(ts);
                IEC104Client::addData(datapoints, "M_ME_TD_1", value, qd, ts, is_invalid);

                MeasuredValueNormalizedWithCP56Time2a_destroy(io);
            }
            break;
        case M_ME_TE_1:
            for (i = 0; i < CS101_ASDU_getNumberOfElements(asdu); i++) {
                auto io = (MeasuredValueScaledWithCP56Time2a) CS101_ASDU_getElement(asdu, i);
                long int value = MeasuredValueScaled_getValue((MeasuredValueScaled) io);
                QualityDescriptor qd = MeasuredValueScaled_getQuality((MeasuredValueScaled) io);
                CP56Time2a ts = MeasuredValueScaledWithCP56Time2a_getTimestamp(io);
                bool is_invalid = CP56Time2a_isInvalid(ts);
                IEC104Client::addData(datapoints, "M_ME_TE_1", value, qd, ts, is_invalid);

                MeasuredValueScaledWithCP56Time2a_destroy(io);
            }
            break;
        case M_ME_NC_1:
            for (i = 0; i < CS101_ASDU_getNumberOfElements(asdu); i++) {
                auto io = (MeasuredValueShort) CS101_ASDU_getElement(asdu, i);
                float value = MeasuredValueShort_getValue((MeasuredValueShort) io);
                QualityDescriptor qd = MeasuredValueShort_getQuality((MeasuredValueShort) io);
                IEC104Client::addData(datapoints, "M_ME_NC_1", value, qd);

                MeasuredValueShort_destroy(io);
            }
            break;
        case M_ME_TF_1:
            for (i = 0; i < CS101_ASDU_getNumberOfElements(asdu); i++) {
                auto io = (MeasuredValueShortWithCP56Time2a) CS101_ASDU_getElement(asdu, i);
                float value = MeasuredValueShort_getValue((MeasuredValueShort) io);
                QualityDescriptor qd = MeasuredValueShort_getQuality((MeasuredValueShort) io);
                CP56Time2a ts = MeasuredValueShortWithCP56Time2a_getTimestamp(io);
                bool is_invalid = CP56Time2a_isInvalid(ts);
                IEC104Client::addData(datapoints, "M_ME_TF_1", value, qd, ts, is_invalid);

                MeasuredValueShortWithCP56Time2a_destroy(io);
            }
            break;
        default:
            Logger::getLogger()->error("Type of message not supported");
            return false;
    }
    mclient->sendData(datapoints);

    return true;
}

/** Restarts the plugin */
void IEC104::restart()
{
    stop();
    start();
}

void IEC104::connect()
{
    while (!CS104_Connection_connect(m_connection)) {}
    Logger::getLogger()->info("Connection started");

    m_connected = true;

    CS104_Connection_sendStartDT(m_connection);
}

/** Starts the plugin */
void IEC104::start()
{
    Logger::getLogger()->info("Starting iec104");
    m_client = new IEC104Client(this);
    try {
        const char* conn_ip = m_ip.c_str();
        m_connection = CS104_Connection_create(conn_ip,m_port);
        Logger::getLogger()->info("Connection created");
    } catch (exception &e) {
        Logger::getLogger()->fatal("Exception while creating connection", e.what());
        throw e;
    }

    CS104_Connection_setConnectionHandler(m_connection, connectionHandler, static_cast<void*>(this));
    CS104_Connection_setASDUReceivedHandler(m_connection, asduReceivedHandler, static_cast<void*>(m_client));

    connect();
}

/** Disconnect from the iec104 server */
void IEC104::stop()
{
    delete m_client;
    m_client = nullptr;

    // if the connection has already ended, we don't send another close request
    if (m_connected)
    {
        CS104_Connection_destroy(m_connection);
        m_connected = false;
    }
}

/**
 * Called when a data changed event is received. This calls back to the south service
 * and adds the points to the readings queue to send.
 *
 * @param points    The points in the reading we must create
 */
void IEC104::ingest(Reading& reading)
{
    (*m_ingest)(m_data, reading);
}

/**
 * Save the callback function and its data
 * @param data   The Ingest function data
 * @param cb     The callback function to call
 */
void IEC104::registerIngest(void *data, INGEST_CB cb)
{
    m_ingest = cb;
    m_data = data;
}

void IEC104Client::sendData(vector<Datapoint*> datapoints)
{
    string asset = datapoints[0]->getName();    // Every point has the same name

    Reading reading(asset, datapoints);

    m_iec104->ingest(reading);
}

template <class T>
void IEC104Client::m_addData(vector<Datapoint*>& datapoints,
                             const std::string& dataname, const T value,
                             QualityDescriptor qd, CP56Time2a ts, bool is_ts_invalid)
{
    auto* measure_features = new vector<Datapoint*>(
    {
        m_createDatapoint("Value", value),
        m_createDatapoint("Quality_Descriptor", (long) qd),
        m_createDatapoint("Timestamp", CP56Time2aToString(ts)),
        m_createDatapoint("Is_Timestamp_Valid", (long) (!is_ts_invalid))
    });

    DatapointValue dpv(measure_features, true);

    datapoints.push_back(new Datapoint(dataname, dpv));
}
