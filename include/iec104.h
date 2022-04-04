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
    static void setJsonConfig(const std::string& stack_configuration,
                              const std::string& msg_configuration,
                              const std::string& pivot_configuration,
                              const std::string& tls_configuration);

    void restart();
    void start();
    void stop();
    void prepareParameters(CS104_Connection& connection);
    void connect(unsigned int connection_index);

    // void ingest(Reading& reading);
    void ingest(std::string assetName, std::vector<Datapoint*>& points);
    void registerIngest(void* data, void (*cb)(void*, Reading));
    bool operation(const std::string& operation, int count,
                   PLUGIN_PARAMETER** params);
    // For test purpose
    void sendInterrogationCommmands();
    void sendInterrogationCommmandToCA(unsigned int ca, int gi_repeat_count,
                                       int gi_time);

    // For test purpose
    static bool m_asduReceivedHandlerP(void* parameter, int address,
                                       CS101_ASDU asdu);

    static bool getCommWttag() { return m_comm_wttag; };
    static void setCommWttag(bool comm_wttag) { m_comm_wttag = comm_wttag; };

    static std::string getTsiv() { return m_tsiv; };
    static void setTsiv(std::string tsiv) { m_tsiv = tsiv; };

private:
    typedef void (*IEC104_ASDUHandler)(std::vector<Datapoint*>& datapoints,
                                       std::string& label,
                                       IEC104Client* mclient, unsigned int& ca,
                                       CS101_ASDU& asdu, InformationObject& io,
                                       uint64_t& ioa);
    template <class T>
    static T m_getConfigValue(nlohmann::json configuration,
                              nlohmann::json_pointer<nlohmann::json> path);

    void m_sendInterrogationCommmands();
    void m_sendInterrogationCommmandToCA(unsigned int ca, int gi_repeat_count,
                                         int gi_time);
    void m_sendTestCommmands();

    static std::string m_checkExchangedDataLayer(unsigned int ca,
                                                 const std::string& type_id,
                                                 uint64_t ioa);

    static CS104_Connection m_createTlsConnection(const char* ip, int port);

    static int m_watchdog(int delay, int checkRes, bool* flag, std::string id);

    static int m_getBroadcastCA();

    static void m_connectionHandler(void* parameter,
                                    CS104_Connection connection,
                                    CS104_ConnectionEvent event);
    static bool m_asduReceivedHandler(void* parameter, int address,
                                      CS101_ASDU asdu);

    static void handleASDU(std::vector<std::string>&,
                           std::vector<Datapoint*>& datapoints,
                           IEC104Client* mclient, CS101_ASDU& asdu,
                           IEC104_ASDUHandler callback);

    bool m_startup_done;

    static void handleM_ME_NB_1(std::vector<Datapoint*>& datapoints,
                                std::string& label, IEC104Client* mclient,
                                unsigned int& ca, CS101_ASDU& asdu,
                                InformationObject& io, uint64_t& ioa);

    static void handleM_SP_NA_1(std::vector<Datapoint*>& datapoints,
                                std::string& label, IEC104Client* mclient,
                                unsigned int& ca, CS101_ASDU& asdu,
                                InformationObject& io, uint64_t& ioa);

    static void handleM_SP_TB_1(std::vector<Datapoint*>& datapoints,
                                std::string& label, IEC104Client* mclient,
                                unsigned int& ca, CS101_ASDU& asdu,
                                InformationObject& io, uint64_t& ioa);

    static void handleM_DP_NA_1(std::vector<Datapoint*>& datapoints,
                                std::string& label, IEC104Client* mclient,
                                unsigned int& ca, CS101_ASDU& asdu,
                                InformationObject& io, uint64_t& ioa);

    static void handleM_DP_TB_1(std::vector<Datapoint*>& datapoints,
                                std::string& label, IEC104Client* mclient,
                                unsigned int& ca, CS101_ASDU& asdu,
                                InformationObject& io, uint64_t& ioa);

    static void handleM_ST_NA_1(std::vector<Datapoint*>& datapoints,
                                std::string& label, IEC104Client* mclient,
                                unsigned int& ca, CS101_ASDU& asdu,
                                InformationObject& io, uint64_t& ioa);

    static void handleM_ST_TB_1(std::vector<Datapoint*>& datapoints,
                                std::string& label, IEC104Client* mclient,
                                unsigned int& ca, CS101_ASDU& asdu,
                                InformationObject& io, uint64_t& ioa);

    static void handleM_ME_NA_1(std::vector<Datapoint*>& datapoints,
                                std::string& label, IEC104Client* mclient,
                                unsigned int& ca, CS101_ASDU& asdu,
                                InformationObject& io, uint64_t& ioa);

    static void handleM_ME_TD_1(std::vector<Datapoint*>& datapoints,
                                std::string& label, IEC104Client* mclient,
                                unsigned int& ca, CS101_ASDU& asdu,
                                InformationObject& io, uint64_t& ioa);

    static void handleM_ME_TE_1(std::vector<Datapoint*>& datapoints,
                                std::string& label, IEC104Client* mclient,
                                unsigned int& ca, CS101_ASDU& asdu,
                                InformationObject& io, uint64_t& ioa);

    static void handleM_ME_NC_1(std::vector<Datapoint*>& datapoints,
                                std::string& label, IEC104Client* mclient,
                                unsigned int& ca, CS101_ASDU& asdu,
                                InformationObject& io, uint64_t& ioa);

    static void handleM_ME_TF_1(std::vector<Datapoint*>& datapoints,
                                std::string& label, IEC104Client* mclient,
                                unsigned int& ca, CS101_ASDU& asdu,
                                InformationObject& io, uint64_t& ioa);

    static nlohmann::json m_stack_configuration;
    static nlohmann::json m_msg_configuration;
    static nlohmann::json m_pivot_configuration;
    static nlohmann::json m_tls_configuration;

    std::string m_asset;

    static bool m_comm_wttag;
    static std::string m_tsiv;

protected:
    std::vector<CS104_Connection> m_connections;

private:
    INGEST_CB m_ingest;  // Callback function used to send data to south service
    void* m_data;        // Ingest function data
    IEC104Client* m_client;
};

class IEC104Client
{
public:
    explicit IEC104Client(IEC104* iec104, nlohmann::json* pivot_configuration)
        : m_iec104(iec104), m_pivot_configuration(pivot_configuration)
    {
    }

    // ==================================================================== //
    // Note : The overloaded method addData is used to prevent the user from
    // giving value type that can't be handled. The real work is forwarded
    // to the private method m_addData

    void addData(std::vector<Datapoint*>& datapoints, int64_t ioa,
                 const std::string& dataname, const int64_t value,
                 QualityDescriptor qd, CP56Time2a ts = nullptr)
    {
        m_addData(datapoints, ioa, dataname, value, qd, ts);
    }

    void addData(std::vector<Datapoint*>& datapoints, int64_t ioa,
                 const std::string& dataname, const float value,
                 QualityDescriptor qd, CP56Time2a ts = nullptr)
    {
        m_addData(datapoints, ioa, dataname, value, qd, ts);
    }
    // ==================================================================== //

    // Sends the datapoints passed as Reading to Fledge
    void sendData(CS101_ASDU asdu, std::vector<Datapoint*> data,
                  const std::vector<std::string> labels);

private:
    template <class T>
    void m_addData(std::vector<Datapoint*>& datapoints, int64_t ioa,
                   const std::string& dataname, const T value,
                   QualityDescriptor qd, CP56Time2a ts);

    template <class T>
    static Datapoint* m_createDatapoint(const std::string& dataname,
                                        const T value)
    {
        DatapointValue dp_value = DatapointValue(value);
        return new Datapoint(dataname, dp_value);
    }

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
    nlohmann::json* m_pivot_configuration;
};

#endif  // INCLUDE_IEC104_H_
