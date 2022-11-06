#ifndef _IEC104_H
#define _IEC104_H

/*
 * Fledge IEC 104 south plugin.
 *
 * Copyright (c) 2020, RTE (https://www.rte-france.com)
 * 
 * Released under the Apache 2.0 Licence
 *
 * Author: Estelle Chigot, Lucas Barret, Chauchadis Rémi, Colin Constans, Akli Rahmoun
 */

#include <iostream>
#include <cstring>
#include <vector>
#include <reading.h>
#include <logger.h>
#include <cstdlib>
#include <cstdio>
#include <lib60870/tls_config.h>
#include <lib60870/hal_time.h>
#include <lib60870/hal_thread.h>
#include <lib60870/cs104_connection.h>
#include <utility>
#include <plugin_api.h>
#include <json.hpp> // https://github.com/nlohmann/json
#include <thread>
#include <chrono>
#include <mutex>


class IEC104Client;

class IEC104
{
public:
    typedef void (*INGEST_CB)(void *, Reading);


    IEC104();
    ~IEC104() = default;

    void		setAssetName(const std::string& asset) { m_asset = asset; }
    static void        setJsonConfig(const std::string& stack_configuration, const std::string& msg_configuration,
                              const std::string& pivot_configuration, const std::string& tls_configuration);

    void		restart();
    void        start();
    void		stop();
    void		connect(unsigned int connection_index);

    void		ingest(Reading& reading);
    void		registerIngest(void *data, void (*cb)(void *, Reading));
    bool        operation(const std::string& operation, int count, PLUGIN_PARAMETER **params);


private:
    template <class T>
    static T m_getConfigValue(nlohmann::json configuration, nlohmann::json_pointer<nlohmann::json> path);

    void m_sendInterrogationCommmands();
    void m_sendInterrogationCommmandToCA(unsigned int ca, int gi_repeat_count, int gi_time);
	void m_sendTestCommmands();
	
	static std::string m_checkExchangedDataLayer(unsigned int ca, const std::string& type_id, unsigned int ioa);

    static CS104_Connection m_createTlsConnection(const char* ip, int port);

	static int m_watchdog(int delay, int checkRes, bool *flag, std::string id);

    static int m_getBroadcastCA() ;

    static void m_connectionHandler (void* parameter, CS104_Connection connection, CS104_ConnectionEvent event);
    static bool m_asduReceivedHandler (void* parameter, int address, CS101_ASDU asdu);

    bool m_startup_done;

    static nlohmann::json m_stack_configuration;
    static nlohmann::json m_msg_configuration;
    static nlohmann::json m_pivot_configuration;
    static nlohmann::json m_tls_configuration;

    std::string	m_asset;

    static bool	        m_comm_wttag;
	static std::string	m_tsiv;
    std::vector<CS104_Connection>    m_connections;

    INGEST_CB			m_ingest;     // Callback function used to send data to south service
    void*               m_data;       // Ingest function data
    IEC104Client*       m_client;
};

class IEC104Client
{
public :
    explicit IEC104Client(IEC104 *iec104, nlohmann::json* pivot_configuration) :
        m_iec104(iec104),
        m_pivot_configuration(pivot_configuration)
        {};

    // ==================================================================== //
    // Note : The overloaded method addData is used to prevent the user from
    // giving value type that can't be handled. The real work is forwarded
    // to the private method m_addData

    void addData(std::vector<std::pair<std::string, Datapoint*>>& datapoints, long ioa,
                        const std::string& dataname, const long int value,
                        QualityDescriptor qd, CP56Time2a ts = nullptr)
    { m_addData(datapoints, ioa, dataname, value, qd, ts); }

    void addData(std::vector<std::pair<std::string, Datapoint*>>& datapoints, long ioa,
                        const std::string& dataname, const float value,
                        QualityDescriptor qd, CP56Time2a ts = nullptr)
    { m_addData(datapoints, ioa, dataname, value, qd, ts); }
    // ==================================================================== //

    // Sends the datapoints passed as Reading to Fledge
    void sendData(CS101_ASDU asdu, std::vector<std::pair<std::string, Datapoint*>> data);

private:
    template <class T>
    void m_addData(std::vector<std::pair<std::string, Datapoint*>> &datapoints, long ioa,
                          const std::string& dataname, const T value,
                          QualityDescriptor qd, CP56Time2a ts);

    template <class T>
    static Datapoint* m_createDatapoint(const std::string& dataname, const T value)
    {
        DatapointValue dp_value = DatapointValue(value);
        return new Datapoint(dataname,dp_value);
    }

    // Format 2019-01-01 10:00:00.123456+08:00
    static std::string CP56Time2aToString(const CP56Time2a ts)
    {
        if (ts == nullptr)
            return "";

        return std::to_string(CP56Time2a_getYear(ts) + 2000) + "-" +
               std::to_string(CP56Time2a_getMonth(ts)) + "-" +
               std::to_string(CP56Time2a_getDayOfMonth (ts)) + " " +
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

#endif
