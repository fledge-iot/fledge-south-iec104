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
#include <json.hpp>
// clang-format on

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


    void prepareParameters(CS104_Connection connection);  

    // For test purpose
    void sendInterrogationCommmands();
    void sendInterrogationCommmandToCA(unsigned int ca, int gi_repeat_count,
                                       int gi_time);

    static bool getCommWttag() { return m_comm_wttag; };
    static void setCommWttag(bool comm_wttag) { m_comm_wttag = comm_wttag; };

    static std::string getTsiv() { return m_tsiv; };
    static void setTsiv(std::string tsiv) { m_tsiv = tsiv; };

private:

    template <class T>
    static T m_getConfigValue(nlohmann::json configuration,
                              nlohmann::json_pointer<nlohmann::json> path);

    void m_sendInterrogationCommmands();
    void m_sendInterrogationCommmandToCA(unsigned int ca, int gi_repeat_count,
                                         int gi_time);

    static CS104_Connection m_createTlsConnection(const char* ip, int port);

    static int m_watchdog(int delay, int checkRes, bool* flag, std::string id);

    int m_getBroadcastCA();

    bool m_singleCommandOperation(int count, PLUGIN_PARAMETER** params, bool withTime);
    bool m_doubleCommandOperation(int count, PLUGIN_PARAMETER** params, bool withTime);
    bool m_stepCommandOperation(int count, PLUGIN_PARAMETER** params, bool withTime);
    bool m_setpointNormalized(int count, PLUGIN_PARAMETER** params, bool withTime);
    bool m_setpointScaled(int count, PLUGIN_PARAMETER** params, bool withTime);
    bool m_setpointShort(int count, PLUGIN_PARAMETER** params, bool withTime);

    bool m_startup_done;

    nlohmann::json m_stack_configuration;
    nlohmann::json m_msg_configuration;
    nlohmann::json m_tls_configuration;

    std::string m_asset;

    static bool m_comm_wttag;
    static std::string m_tsiv;

protected:
    std::vector<CS104_Connection> m_connections;

private:
    INGEST_CB m_ingest = nullptr;  // Callback function used to send data to south service
    void* m_data;        // Ingest function data
    IEC104Client* m_client = nullptr;
    bool m_isRunning = false;
};

class IEC104Client
{
public:

    explicit IEC104Client(IEC104* iec104,
                    nlohmann::json* stack_configuration,
                    nlohmann::json* msg_configuration);

    ~IEC104Client();

    // ==================================================================== //

    // Sends the datapoints passed as Reading to Fledge
    void sendData(CS101_ASDU asdu, std::vector<Datapoint*> data,
                  const std::vector<std::string> labels);


    bool sendInterrogationCommand(int ca);

    bool sendSingleCommand(int ca, int ioa, bool value, bool withTime, bool select);

    bool sendDoubleCommand(int ca, int ioa, int value, bool withTime, bool select);

    bool sendStepCommand(int ca, int ioa, int value, bool withTime, bool select);

    bool sendSetpointNormalized(int ca, int ioa, float value, bool withTime);

    bool sendSetpointScaled(int ca, int ioa, int value, bool withTime);

    bool sendSetpointShort(int ca, int ioa, float value, bool withTime);

    void start();

    void stop();

    static bool isMessageTypeMatching(int expectedType, int rcvdType);

private:

    typedef enum {
        CON_STATE_IDLE,
        CON_STATE_CONNECTING,
        CON_STATE_CONNECTED_INACTIVE,
        CON_STATE_CONNECTED_ACTIVE,
        CON_STATE_CLOSED,
        CON_STATE_WAIT_FOR_RECONNECT,
        CON_STATE_FATAL_ERROR
    } ConState;


    typedef struct {
        int ca;
        int ioa;
        int typeId;
        std::string label;
    } DataExchangeDefinition;

    std::map<int, std::map<int, DataExchangeDefinition*>> exchangeDefinitions;


    ConState m_connectionState = CON_STATE_IDLE;
    bool m_started = false;
    bool m_startDtSent = false;
    bool m_timeSyncEnabled = false;
    bool m_timeSynchronized = false;
    bool m_timeSyncCommandSent = false;
    bool m_timeSyncPeriod = 0;
    uint64_t m_nextTimeSync;

    CS104_Connection m_connection = nullptr;

    uint64_t m_delayExpirationTime;

    std::thread* m_conThread = nullptr;
    void _conThread();

    int broadcastCA();
    int defaultCA();
    int timeSyncCA();
    void createDataExchangeDefinitions();

    void prepareParameters(CS104_Connection connection);

    bool prepareConnection();
    void performPeriodicTasks();

    std::string m_checkExchangedDataLayer(int ca, int type_id, int ioa);

    static void m_connectionHandler(void* parameter, CS104_Connection connection,
                                 CS104_ConnectionEvent event);

    static bool m_asduReceivedHandler(void* parameter, int address,
        CS101_ASDU asdu);

    template <class T>
    static T m_getConfigValue(nlohmann::json configuration,
                              nlohmann::json_pointer<nlohmann::json> path);

    template <class T>
    static T m_getConfigValueDefault(nlohmann::json configuration, 
                                nlohmann::json_pointer<nlohmann::json> path, T defaultValue);

    template <class T>
    void m_addData(CS101_ASDU asdu, std::vector<Datapoint*>& datapoints, int64_t ioa,
                   const std::string& dataname, const T value,
                   QualityDescriptor qd, CP56Time2a ts = nullptr);

    template <class T>
    static Datapoint* m_createDatapoint(const std::string& dataname,
                                        const T value)
    {
        DatapointValue dp_value = DatapointValue(value);
        return new Datapoint(dataname, dp_value);
    }

    typedef void (*IEC104_ASDUHandler)(std::vector<Datapoint*>& datapoints,
                                    std::string& label,
                                    IEC104Client* mclient, unsigned int ca,
                                    CS101_ASDU asdu, InformationObject io,
                                    uint64_t ioa);

    static void handleASDU(std::vector<std::string>&,
                           std::vector<Datapoint*>& datapoints,
                           IEC104Client* mclient, CS101_ASDU asdu,
                           IEC104_ASDUHandler callback);

    static void handleM_ME_NB_1(std::vector<Datapoint*>& datapoints,
                                std::string& label, IEC104Client* mclient,
                                unsigned int ca, CS101_ASDU asdu,
                                InformationObject io, uint64_t ioa);

    static void handleM_SP_NA_1(std::vector<Datapoint*>& datapoints,
                                std::string& label, IEC104Client* mclient,
                                unsigned int ca, CS101_ASDU asdu,
                                InformationObject io, uint64_t ioa);

    static void handleM_SP_TB_1(std::vector<Datapoint*>& datapoints,
                                std::string& label, IEC104Client* mclient,
                                unsigned int ca, CS101_ASDU asdu,
                                InformationObject io, uint64_t ioa);

    static void handleM_DP_NA_1(std::vector<Datapoint*>& datapoints,
                                std::string& label, IEC104Client* mclient,
                                unsigned int ca, CS101_ASDU asdu,
                                InformationObject io, uint64_t ioa);

    static void handleM_DP_TB_1(std::vector<Datapoint*>& datapoints,
                                std::string& label, IEC104Client* mclient,
                                unsigned int ca, CS101_ASDU asdu,
                                InformationObject io, uint64_t ioa);

    static void handleM_ST_NA_1(std::vector<Datapoint*>& datapoints,
                                std::string& label, IEC104Client* mclient,
                                unsigned int ca, CS101_ASDU asdu,
                                InformationObject io, uint64_t ioa);

    static void handleM_ST_TB_1(std::vector<Datapoint*>& datapoints,
                                std::string& label, IEC104Client* mclient,
                                unsigned int ca, CS101_ASDU asdu,
                                InformationObject io, uint64_t ioa);

    static void handleM_ME_NA_1(std::vector<Datapoint*>& datapoints,
                                std::string& label, IEC104Client* mclient,
                                unsigned int ca, CS101_ASDU asdu,
                                InformationObject io, uint64_t ioa);

    static void handleM_ME_TD_1(std::vector<Datapoint*>& datapoints,
                                std::string& label, IEC104Client* mclient,
                                unsigned int ca, CS101_ASDU asdu,
                                InformationObject io, uint64_t ioa);

    static void handleM_ME_TE_1(std::vector<Datapoint*>& datapoints,
                                std::string& label, IEC104Client* mclient,
                                unsigned int ca, CS101_ASDU asdu,
                                InformationObject io, uint64_t ioa);

    static void handleM_ME_NC_1(std::vector<Datapoint*>& datapoints,
                                std::string& label, IEC104Client* mclient,
                                unsigned int ca, CS101_ASDU asdu,
                                InformationObject io, uint64_t ioa);

    static void handleM_ME_TF_1(std::vector<Datapoint*>& datapoints,
                                std::string& label, IEC104Client* mclient,
                                unsigned int ca, CS101_ASDU asdu,
                                InformationObject io, uint64_t ioa);

    // commands and setpoint commands
    static void handleC_SC_TA_1(std::vector<Datapoint*>& datapoints,
                                std::string& label, IEC104Client* mclient,
                                unsigned int ca, CS101_ASDU asdu,
                                InformationObject io, uint64_t ioa);

    static void handleC_DC_TA_1(std::vector<Datapoint*>& datapoints,
                                std::string& label, IEC104Client* mclient,
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
    nlohmann::json* m_stack_configuration;
    nlohmann::json* m_msg_configuration;

    bool m_comm_wttag = false;
};

#endif  // INCLUDE_IEC104_H_
