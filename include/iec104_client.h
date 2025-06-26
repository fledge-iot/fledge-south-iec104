#ifndef INCLUDE_IEC104_CLIENT_H_
#define INCLUDE_IEC104_CLIENT_H_

/*
 * Fledge IEC 104 south plugin.
 *
 * Copyright (c) 2020, RTE (https://www.rte-france.com)
 *
 * Released under the Apache 2.0 Licence
 *
 * Author: Michael Zillgith
 *
 */

#include <mutex>
#include <thread>
#include <vector>
#include <string>
#include <memory>

#include <lib60870/cs104_connection.h>

class IEC104;
class IEC104ClientRedGroup;
class IEC104ClientConnection;
class IEC104ClientConfig;
class DataExchangeDefinition;
class RedGroupCon;
class Datapoint;

class IEC104Client
{
public:

    explicit IEC104Client(IEC104* iec104, std::shared_ptr<IEC104ClientConfig> config);

    ~IEC104Client();

    // ==================================================================== //

    // Sends the datapoints passed as Reading to Fledge
    void sendData(std::vector<Datapoint*> data,
                  const std::vector<std::string> labels);

    bool sendInterrogationCommand(int ca);

    bool sendSingleCommand(int ca, int ioa, bool value, bool withTime, bool select, long time);

    bool sendDoubleCommand(int ca, int ioa, int value, bool withTime, bool select, long time);

    bool sendStepCommand(int ca, int ioa, int value, bool withTime, bool select, long time);

    bool sendSetpointNormalized(int ca, int ioa, float value, bool withTime, long time);

    bool sendSetpointScaled(int ca, int ioa, int value, bool withTime, long time);

    bool sendSetpointShort(int ca, int ioa, float value, bool withTime, long time);

    bool sendConnectionStatus();

    bool scheduleGI();

    bool handleASDU(const IEC104ClientConnection* connection, CS101_ASDU asdu);

    void start();

    void stop();

    enum class GiStatus
    {
        IDLE,
        STARTED,
        IN_PROGRESS,
        FAILED,
        FINISHED
    };

    void updateGiStatus(GiStatus newState);

    GiStatus getGiStatus();

    static bool isMessageTypeMatching(int expectedType, int rcvdType);

    void updateQualityForAllDataObjectsInStationGroup(QualityDescriptor qd);

    void createListOfDatapointsInStationGroup();

    void updateQualityForDataObjectsNotReceivedInGIResponse(QualityDescriptor qd);

    const std::string& getServiceName() const;

private:

    std::vector<std::shared_ptr<DataExchangeDefinition>> m_listOfStationGroupDatapoints;

    std::shared_ptr<IEC104ClientConfig> m_config;

    class OutstandingCommand {
    public:

        explicit OutstandingCommand(int typeId, int ca, int ioa, std::shared_ptr<IEC104ClientConnection> con);

        int typeId = 0;
        int ca = 0;
        int ioa = 0;
        std::shared_ptr<IEC104ClientConnection> clientCon;
        bool actConReceived = false;
        uint64_t timeout = 0;
    };

    std::vector<std::shared_ptr<OutstandingCommand>> m_outstandingCommands; // list of outstanding commands
    std::mutex m_outstandingCommandsMtx; // protect access to list of outstanding commands

    std::shared_ptr<OutstandingCommand> checkForOutstandingCommand(int typeId, int ca, int ioa, const IEC104ClientConnection* connection);

    void checkOutstandingCommandTimeouts();

    void removeOutstandingCommand(std::shared_ptr<OutstandingCommand> command);

    std::shared_ptr<OutstandingCommand> addOutstandingCommandAndCheckLimit(int ca, int ioa, bool withTime, int typeIdWithTimestamp, int typeIdNoTimestamp);

    enum class ConnectionStatus
    {
        STARTED,
        NOT_CONNECTED
    };

    ConnectionStatus m_connStatus = ConnectionStatus::NOT_CONNECTED;

    void updateConnectionStatus(ConnectionStatus newState);

    GiStatus m_giStatus = GiStatus::IDLE;

    void sendSouthMonitoringEvent(bool connxStatus, bool giStatus);

    std::vector<std::shared_ptr<IEC104ClientConnection>> m_connections;

    std::shared_ptr<IEC104ClientConnection> m_activeConnection;
    std::mutex m_activeConnectionMtx;

    bool m_started = false;

    std::shared_ptr<std::thread> m_monitoringThread;
    void _monitoringThread();

    void prepareParameters(CS104_Connection connection, std::shared_ptr<IEC104ClientRedGroup> redgroup, std::shared_ptr<RedGroupCon> redgroupCon);
    bool prepareConnections();
    void performPeriodicTasks();

    template <class T>
    static Datapoint* m_createDatapoint(const std::string& dataname, const T value);

    Datapoint* m_createQualityUpdateForDataObject(std::shared_ptr<DataExchangeDefinition> dataDefinition, const QualityDescriptor* qd, CP56Time2a ts);

    void updateQualityForAllDataObjects(QualityDescriptor qd);

    void removeFromListOfDatapoints(std::shared_ptr<DataExchangeDefinition> toRemove);

    template <class T>
    Datapoint* m_createDataObject(CS101_ASDU asdu, int64_t ioa, const std::string& dataname, const T value,
        QualityDescriptor* qd, CP56Time2a ts = nullptr);

    typedef void (*IEC104_ASDUHandler)(std::vector<Datapoint*>& datapoints,
                                    std::string& label,
                                    std::shared_ptr<IEC104Client> mclient, unsigned int ca,
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
    void handle_C_SC_NA_1(std::vector<Datapoint*>& datapoints,
                                std::string& label,
                                unsigned int ca, CS101_ASDU asdu,
                                InformationObject io, uint64_t ioa,
                                std::shared_ptr<OutstandingCommand> outstandingCommand);

    void handle_C_DC_NA_1(std::vector<Datapoint*>& datapoints,
                                std::string& label,
                                unsigned int ca, CS101_ASDU asdu,
                                InformationObject io, uint64_t ioa,
                                std::shared_ptr<OutstandingCommand> outstandingCommand);

    void handle_C_RC_NA_1(std::vector<Datapoint*>& datapoints,
                                std::string& label,
                                unsigned int ca, CS101_ASDU asdu,
                                InformationObject io, uint64_t ioa,
                                std::shared_ptr<OutstandingCommand> outstandingCommand);

    void handle_C_SE_NA_1(std::vector<Datapoint*>& datapoints,
                                std::string& label,
                                unsigned int ca, CS101_ASDU asdu,
                                InformationObject io, uint64_t ioa,
                                std::shared_ptr<OutstandingCommand> outstandingCommand);

    void handle_C_SE_NB_1(std::vector<Datapoint*>& datapoints,
                                std::string& label,
                                unsigned int ca, CS101_ASDU asdu,
                                InformationObject io, uint64_t ioa,
                                std::shared_ptr<OutstandingCommand> outstandingCommand);

    void handle_C_SE_NC_1(std::vector<Datapoint*>& datapoints,
                                std::string& label,
                                unsigned int ca, CS101_ASDU asdu,
                                InformationObject io, uint64_t ioa,
                                std::shared_ptr<OutstandingCommand> outstandingCommand);

    bool isAsduTriggerGi(std::vector<Datapoint*>& datapoints,
                            unsigned int ca,
                            CS101_ASDU asdu,
                            uint64_t ioa,
                            IEC60870_5_TypeID typeId);

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

    IEC104* m_iec104 = nullptr;
};

#endif  // INCLUDE_IEC104_CLIENT_H_
