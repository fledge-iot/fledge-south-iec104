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
#include <fstream>
#include <iostream>
#include <utility>

using namespace std;
using namespace nlohmann;

void IEC104Client::addData(std::vector<Datapoint*>& datapoints, int64_t ioa,
                const std::string& dataname, const int64_t value,
                QualityDescriptor qd, CP56Time2a ts)
{
    m_addData(datapoints, ioa, dataname, value, qd, ts);
}

void IEC104Client::addData(std::vector<Datapoint*>& datapoints, int64_t ioa,
                const std::string& dataname, const float value,
                QualityDescriptor qd, CP56Time2a ts)
{
    m_addData(datapoints, ioa, dataname, value, qd, ts);
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

IEC104Client::IEC104Client(IEC104* iec104, nlohmann::json* pivot_configuration, nlohmann::json* stack_configuration)
        : m_iec104(iec104), m_pivot_configuration(pivot_configuration), m_stack_configuration(stack_configuration)
{
}

bool IEC104Client::m_asduReceivedHandler(void* parameter, int address,
                                   CS101_ASDU asdu)
{
    IEC104Client* self = static_cast<IEC104Client*>(parameter);

    printf("asdu received\n");

    return true;
}

void IEC104Client::m_connectionHandler(void* parameter, CS104_Connection connection,
                                 CS104_ConnectionEvent event)
{
    IEC104Client* self = static_cast<IEC104Client*>(parameter);

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
            Logger::getLogger()->info("Connection created");
        }

        if (new_connection) {
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

    if (new_connection) {
        m_connection = new_connection;
        return true;
    }
    else {
        m_connection = nullptr;
        return false;
    }
}

void IEC104Client::_conThread()
{
    while (m_started) {
        printf("tick\n");

        switch (m_connectionState) {
            case CON_STATE_IDLE:
                {
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

bool IEC104Client::sendSingleCommand(int ca, int ioa, bool value, bool withTime)
{
    // send single command over active connection
    bool success = false;

    if ((m_connection != nullptr) && (m_connectionState == CON_STATE_CONNECTED_ACTIVE)) 
    {
        InformationObject sc = nullptr;

        if (withTime) {
            struct sCP56Time2a ts;
            
            CP56Time2a_createFromMsTimestamp(&ts, Hal_getTimeInMs());

            sc = (InformationObject)SingleCommandWithCP56Time2a_create(NULL, ioa, value, false, 0, &ts);
        }
        else {
            sc = (InformationObject)SingleCommand_create(NULL, ioa, value, false, 0);
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

bool IEC104Client::sendDoubleCommand(int ca, int ioa, int value, bool withTime)
{
    //TODO send double command over active connection
    return true;
}

bool IEC104Client::sendStepCommand(int ca, int ioa, int value, bool withTime)
{
    //TODO send double command over active connection
    return true;
}


