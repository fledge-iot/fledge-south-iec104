#ifndef INCLUDE_IEC104_H_
#define INCLUDE_IEC104_H_

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

#include <lib60870/cs104_connection.h>
#include <lib60870/cs101_information_objects.h>
#include <lib60870/hal_thread.h>
#include <lib60870/hal_time.h>
#include <lib60870/tls_config.h>
#include <logger.h>
#include <plugin_api.h>
#include <reading.h>

// clang-format off
#include <mutex>
#include <chrono>
#include <string>
#include <thread>
#include <utility>
#include <vector>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
// clang-format on

#include "iec104_client_config.h"
#include "iec104_client_connection.h"

class IEC104Client;

class IEC104
{
public:
    typedef void (*INGEST_CB)(void*, Reading);

    IEC104();
    ~IEC104() = default;

    void setAssetName(const std::string& asset) { m_asset = asset; }
    void setJsonConfig(const std::string& stack_configuration,
                              const std::string& msg_configuration,
                              const std::string& tls_configuration);

    void restart();
    void start();
    void stop();

    // void ingest(Reading& reading);
    void ingest(std::string assetName, std::vector<Datapoint*>& points);
    void registerIngest(void* data, void (*cb)(void*, Reading));
    bool operation(const std::string& operation, int count,
                   PLUGIN_PARAMETER** params);

private:

    bool m_singleCommandOperation(int count, PLUGIN_PARAMETER** params, bool withTime);
    bool m_doubleCommandOperation(int count, PLUGIN_PARAMETER** params, bool withTime);
    bool m_stepCommandOperation(int count, PLUGIN_PARAMETER** params, bool withTime);
    bool m_setpointNormalized(int count, PLUGIN_PARAMETER** params, bool withTime);
    bool m_setpointScaled(int count, PLUGIN_PARAMETER** params, bool withTime);
    bool m_setpointShort(int count, PLUGIN_PARAMETER** params, bool withTime);

    IEC104ClientConfig* m_config;

    std::string m_asset;

protected:
    std::vector<CS104_Connection> m_connections;

private:
    INGEST_CB m_ingest = nullptr;  // Callback function used to send data to south service
    void* m_data;        // Ingest function data
    IEC104Client* m_client = nullptr;
};

class IEC104Client
{
public:

    explicit IEC104Client(IEC104* iec104, IEC104ClientConfig* config);

    ~IEC104Client();

    // ==================================================================== //

    // Sends the datapoints passed as Reading to Fledge
    void sendData(std::vector<Datapoint*> data,
                  const std::vector<std::string> labels);

    bool sendInterrogationCommand(int ca);

    bool sendSingleCommand(int ca, int ioa, bool value, bool withTime, bool select);

    bool sendDoubleCommand(int ca, int ioa, int value, bool withTime, bool select);

    bool sendStepCommand(int ca, int ioa, int value, bool withTime, bool select);

    bool sendSetpointNormalized(int ca, int ioa, float value, bool withTime);

    bool sendSetpointScaled(int ca, int ioa, int value, bool withTime);

    bool sendSetpointShort(int ca, int ioa, float value, bool withTime);

    bool handleASDU(IEC104ClientConnection* connection, CS101_ASDU asdu);

    void start();

    void stop();

    static bool isMessageTypeMatching(int expectedType, int rcvdType);

private:

    IEC104ClientConfig* m_config;

   // std::map<int, int>::iterator m_listOfCA_it;

    //std::map<int, int> m_listOfCA;

    std::vector<IEC104ClientConnection*> m_connections = std::vector<IEC104ClientConnection*>();

    IEC104ClientConnection* m_activeConnection = nullptr;
    std::mutex m_activeConnectionMtx;

    bool m_started = false;

    std::thread* m_monitoringThread = nullptr;
    void _monitoringThread();

    int broadcastCA(); // TODO remove?

    void prepareParameters(CS104_Connection connection, IEC104ClientRedGroup* redgroup, RedGroupCon* redgroupCon);
    bool prepareConnections();
    void performPeriodicTasks();

    static void m_connectionHandler(void* parameter, CS104_Connection connection,
                                 CS104_ConnectionEvent event);

    template <class T>
    static Datapoint* m_createDatapoint(const std::string& dataname,
                                        const T value)
    {
        DatapointValue dp_value = DatapointValue(value);
        return new Datapoint(dataname, dp_value);
    }

    template <class T>
    Datapoint* m_createDataObject(CS101_ASDU asdu, int64_t ioa, const std::string& dataname, const T value,
        QualityDescriptor* qd, CP56Time2a ts = nullptr);

    typedef void (*IEC104_ASDUHandler)(std::vector<Datapoint*>& datapoints,
                                    std::string& label,
                                    IEC104Client* mclient, unsigned int ca,
                                    CS101_ASDU asdu, InformationObject io,
                                    uint64_t ioa);

    void handle_M_ME_NB_1(std::vector<Datapoint*>& datapoints,
                                std::string& label,
                                unsigned int ca, CS101_ASDU asdu,
                                InformationObject io, uint64_t ioa);

    void handle_M_SP_NA_1(std::vector<Datapoint*>& datapoints,
                                std::string& label,
                                unsigned int ca, CS101_ASDU asdu,
                                InformationObject io, uint64_t ioa);

    void handle_M_SP_TB_1(std::vector<Datapoint*>& datapoints,
                                std::string& label,
                                unsigned int ca, CS101_ASDU asdu,
                                InformationObject io, uint64_t ioa);

    void handle_M_DP_NA_1(std::vector<Datapoint*>& datapoints,
                                std::string& label,
                                unsigned int ca, CS101_ASDU asdu,
                                InformationObject io, uint64_t ioa);

    void handle_M_DP_TB_1(std::vector<Datapoint*>& datapoints,
                                std::string& label,
                                unsigned int ca, CS101_ASDU asdu,
                                InformationObject io, uint64_t ioa);

    void handle_M_ST_NA_1(std::vector<Datapoint*>& datapoints,
                                std::string& label,
                                unsigned int ca, CS101_ASDU asdu,
                                InformationObject io, uint64_t ioa);

    void handle_M_ST_TB_1(std::vector<Datapoint*>& datapoints,
                                std::string& label,
                                unsigned int ca, CS101_ASDU asdu,
                                InformationObject io, uint64_t ioa);

    void handle_M_ME_NA_1(std::vector<Datapoint*>& datapoints,
                                std::string& label,
                                unsigned int ca, CS101_ASDU asdu,
                                InformationObject io, uint64_t ioa);

    void handle_M_ME_TD_1(std::vector<Datapoint*>& datapoints,
                                std::string& label,
                                unsigned int ca, CS101_ASDU asdu,
                                InformationObject io, uint64_t ioa);

    void handle_M_ME_TE_1(std::vector<Datapoint*>& datapoints,
                                std::string& label,
                                unsigned int ca, CS101_ASDU asdu,
                                InformationObject io, uint64_t ioa);

    void handle_M_ME_NC_1(std::vector<Datapoint*>& datapoints,
                                std::string& label,
                                unsigned int ca, CS101_ASDU asdu,
                                InformationObject io, uint64_t ioa);

    void handle_M_ME_TF_1(std::vector<Datapoint*>& datapoints,
                                std::string& label,
                                unsigned int ca, CS101_ASDU asdu,
                                InformationObject io, uint64_t ioa);

    // commands and setpoint commands (for ACKs)
    void handle_C_SC_TA_1(std::vector<Datapoint*>& datapoints,
                                std::string& label,
                                unsigned int ca, CS101_ASDU asdu,
                                InformationObject io, uint64_t ioa);

    void handle_C_DC_TA_1(std::vector<Datapoint*>& datapoints,
                                std::string& label,
                                unsigned int ca, CS101_ASDU asdu,
                                InformationObject io, uint64_t ioa);

    // Format 2019-01-01 10:00:00.123456+08:00
    static std::string CP56Time2aToString(const CP56Time2a ts)
    {
        if (ts == nullptr) return "";

        return std::to_string(CP56Time2a_getYear(ts) + 2000) + "-" +
               std::to_string(CP56Time2a_getMonth(ts)) + "-" +
               std::to_string(CP56Time2a_getDayOfMonth(ts)) + " " +
               std::to_string(CP56Time2a_getHour(ts)) + ":" +
               std::to_string(CP56Time2a_getMinute(ts)) + ":" +
               std::to_string(CP56Time2a_getSecond(ts)) + "." +
               millisecondsToString(CP56Time2a_getMillisecond(ts));
    }

    static std::string millisecondsToString(int ms)
    {
        if (ms < 10)
            return "00" + std::to_string(ms);
        else if (ms < 100)
            return "0" + std::to_string(ms);
        else
            return std::to_string(ms);
    }

    IEC104* m_iec104;

    bool m_comm_wttag = false;
};

#endif  // INCLUDE_IEC104_H_
