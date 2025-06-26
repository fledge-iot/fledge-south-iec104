#include <ctime>
#include <functional>

#include <utils.h>
#include <reading.h>

#include <lib60870/hal_time.h>
#include <lib60870/hal_thread.h>

#include "iec104_client.h"
#include "iec104_client_config.h"
#include "iec104_client_connection.h"
#include "iec104_client_redgroup.h"
#include "iec104_utility.h"


//DUPLICATE! see iec104_client.c
static uint64_t getMonotonicTimeInMs()
{
    uint64_t timeVal = 0;

    struct timespec ts;

    if (clock_gettime(CLOCK_MONOTONIC, &ts) == 0) {
        timeVal = ((uint64_t) ts.tv_sec * 1000LL) + (ts.tv_nsec / 1000000);
    }

    return timeVal;
}

IEC104ClientConnection::IEC104ClientConnection(
    std::shared_ptr<IEC104Client> client, std::shared_ptr<IEC104ClientRedGroup> redGroup, std::shared_ptr<RedGroupCon> connection,
    std::shared_ptr<IEC104ClientConfig> config, const std::string& pathLetter):
    m_config(config), m_redGroup(redGroup), m_redGroupConnection(connection), m_client(client), m_path_letter(pathLetter)
{
    // Send initial path connection status audit
    m_sendConnectionStatusAudit("disconnected");
}

IEC104ClientConnection::~IEC104ClientConnection()
{
    Stop();
}

void
IEC104ClientConnection::Activate()
{
    std::string beforeLog = Iec104Utility::PluginName + " - IEC104ClientConnection::Activate - ["
                        + m_redGroup->Name() + ", " + std::to_string(m_redGroupConnection->ConnId()) + ", "
                        + m_redGroupConnection->ServerIP() + ":" + std::to_string(m_redGroupConnection->TcpPort()) + "] -";

    std::lock_guard<std::mutex> lock(m_conLock);
    if (m_connectionState == CON_STATE_CONNECTED_INACTIVE) {
        if (m_connection) {
            Iec104Utility::log_info("%s Sending START-DT", beforeLog.c_str());
            CS104_Connection_sendStartDT(m_connection);
        }
        else {
            Iec104Utility::log_warn("%s CS104 connection unavailable, cannot send START-DT", beforeLog.c_str());
        }

        m_startDtSent = true;

        m_connectionState = CON_STATE_CONNECTED_ACTIVE;
        Iec104Utility::log_debug("%s New internal connection state: %d", beforeLog.c_str(), m_connectionState);
    }
}

int
IEC104ClientConnection::broadcastCA() const
{
    if (m_config->CaSize() == 1)
        return 0xff;

    return 0xffff;
}

void
IEC104ClientConnection::m_connectionHandler(void* parameter, CS104_Connection connection,
                                 CS104_ConnectionEvent event)
{
    IEC104ClientConnection* self = static_cast<IEC104ClientConnection*>(parameter);

    std::string beforeLog = Iec104Utility::PluginName + " - IEC104ClientConnection::m_connectionHandler - ["
                        + self->m_redGroup->Name() + ", "
                        + std::to_string(self->m_redGroupConnection->ConnId()) + ", "
                        + self->m_redGroupConnection->ServerIP() + ":"
                        + std::to_string(self->m_redGroupConnection->TcpPort()) + "] -";

    Iec104Utility::log_debug("%s Connection state changed: %d", beforeLog.c_str(), static_cast<int>(event));

    std::lock_guard<std::mutex> lock(self->m_conLock);
    ConState oldConnectionState = self->m_connectionState;
    if (event == CS104_CONNECTION_CLOSED)
    {
        self->m_connectionState = CON_STATE_CLOSED;
        self->m_connected = false;
        self->m_connecting = false;
    }
    else if (event == CS104_CONNECTION_OPENED)
    {
        self->m_connectionState = CON_STATE_CONNECTED_INACTIVE;
        self->m_connected = true;
        self->m_connecting = false;
    }
    else if (event == CS104_CONNECTION_STARTDT_CON_RECEIVED)
    {
        self->m_nextTimeSync = getMonotonicTimeInMs();
        self->m_timeSynchronized = false;
        self->m_firstTimeSyncOperationCompleted = false;
        self->m_firstGISent = false;

        self->m_connectionState = CON_STATE_CONNECTED_ACTIVE;
    }
    else if (event == CS104_CONNECTION_STOPDT_CON_RECEIVED)
    {
        self->m_connectionState = CON_STATE_CONNECTED_INACTIVE;
    }

    if (self->m_connectionState != oldConnectionState) {
        Iec104Utility::log_debug("%s New internal connection state: %d", beforeLog.c_str(), self->m_connectionState);
    }
}

bool
IEC104ClientConnection::sendInterrogationCommand(int ca)
{
    std::string beforeLog = Iec104Utility::PluginName + " - IEC104ClientConnection::sendInterrogationCommand - ["
                        + m_redGroup->Name() + ", " + std::to_string(m_redGroupConnection->ConnId()) + ", "
                        + m_redGroupConnection->ServerIP() + ":" + std::to_string(m_redGroupConnection->TcpPort()) + "] -";
    bool success = false;

    std::lock_guard<std::mutex> lock(m_conLock);

    if ((m_connection != nullptr) && (m_connectionState == CON_STATE_CONNECTED_ACTIVE))
    {
        if (CS104_Connection_sendInterrogationCommand(m_connection, CS101_COT_ACTIVATION, ca, IEC60870_QOI_STATION)) {
            Iec104Utility::log_debug("%s Interrogation command sent (CA=%i)", beforeLog.c_str(), ca);
            success = true;
        }
        else {
            Iec104Utility::log_warn("%s Failed to send interrogation command (CA=%i)", beforeLog.c_str(), ca);
        }
    }
    else {
        Iec104Utility::log_warn("%s Connection unavailable (%s) or not in connected state (%i), cannot send interrogation command (CA=%i)",
                                beforeLog.c_str(), (m_connection == nullptr)?"true":"false", static_cast<int>(m_connectionState), ca);
    }

    return success;
}

bool
IEC104ClientConnection::sendSingleCommand(int ca, int ioa, bool value, bool withTime, bool select, long msTimestamp)
{
    std::string beforeLog = Iec104Utility::PluginName + " - IEC104ClientConnection::sendSingleCommand - ["
                        + m_redGroup->Name() + ", " + std::to_string(m_redGroupConnection->ConnId()) + ", "
                        + m_redGroupConnection->ServerIP() + ":" + std::to_string(m_redGroupConnection->TcpPort()) + "] -";
    bool success = false;

    std::lock_guard<std::mutex> lock(m_conLock);

    if ((m_connection != nullptr) && (m_connectionState == CON_STATE_CONNECTED_ACTIVE))
    {
        InformationObject cmdObj = nullptr;

        if (withTime) {
            struct sCP56Time2a ts;

            CP56Time2a_createFromMsTimestamp(&ts, msTimestamp);

            cmdObj = (InformationObject)SingleCommandWithCP56Time2a_create(NULL, ioa, value, select, 0, &ts);
        }
        else {
            cmdObj = (InformationObject)SingleCommand_create(NULL, ioa, value, select, 0);
        }

        if (cmdObj) {
            if (CS104_Connection_sendProcessCommandEx(m_connection, CS101_COT_ACTIVATION, ca, cmdObj)) {
                Iec104Utility::log_debug("%s single command sent (CA=%i, IOA=%i, value=%s, select=%s, withTime=%s, msTimestamp=%ld)",
                                        beforeLog.c_str(), ca, ioa, value?"true":"false", select?"true":"false", withTime?"true":"false",
                                        msTimestamp);
                success = true;
            }

            InformationObject_destroy(cmdObj);
        }
    }
    else {
        Iec104Utility::log_warn("%s Connection unavailable (%s) or not in connected state (%i), cannot send single command",
                                beforeLog.c_str(), (m_connection == nullptr)?"true":"false", static_cast<int>(m_connectionState));
    }

    if (!success) Iec104Utility::log_warn("%s Failed to send single command (CA=%i, IOA=%i, value=%s, select=%s, withTime=%s, msTimestamp=%ld)",
                                        beforeLog.c_str(), ca, ioa, value?"true":"false", select?"true":"false", withTime?"true":"false",
                                        msTimestamp);

    return success;
}

bool
IEC104ClientConnection::sendDoubleCommand(int ca, int ioa, int value, bool withTime, bool select, long msTimestamp)
{
    std::string beforeLog = Iec104Utility::PluginName + " - IEC104ClientConnection::sendDoubleCommand - ["
                        + m_redGroup->Name() + ", " + std::to_string(m_redGroupConnection->ConnId()) + ", "
                        + m_redGroupConnection->ServerIP() + ":" + std::to_string(m_redGroupConnection->TcpPort()) + "] -";
    bool success = false;

    std::lock_guard<std::mutex> lock(m_conLock);

    if ((m_connection != nullptr) && (m_connectionState == CON_STATE_CONNECTED_ACTIVE))
    {
        InformationObject cmdObj = nullptr;

        if (withTime) {
            struct sCP56Time2a ts;

            CP56Time2a_createFromMsTimestamp(&ts, msTimestamp);

            cmdObj = (InformationObject)DoubleCommandWithCP56Time2a_create(NULL, ioa, value, select, 0, &ts);
        }
        else {
            cmdObj = (InformationObject)DoubleCommand_create(NULL, ioa, value, select, 0);
        }

        if (cmdObj) {
            if (CS104_Connection_sendProcessCommandEx(m_connection, CS101_COT_ACTIVATION, ca, cmdObj)) {
                Iec104Utility::log_debug("%s double command sent (CA=%i, IOA=%i, value=%d, select=%s, withTime=%s, msTimestamp=%ld)",
                                        beforeLog.c_str(), ca, ioa, value, select?"true":"false", withTime?"true":"false", msTimestamp);
                success = true;
            }

            InformationObject_destroy(cmdObj);
        }
    }
    else {
        Iec104Utility::log_warn("%s Connection unavailable (%s) or not in connected state (%i), cannot send double command",
                                beforeLog.c_str(), (m_connection == nullptr)?"true":"false", static_cast<int>(m_connectionState));
    }

    if (!success) Iec104Utility::log_warn("%s Failed to send double command (CA=%i, IOA=%i, value=%d, select=%s, withTime=%s, msTimestamp=%ld)",
                                        beforeLog.c_str(), ca, ioa, value, select?"true":"false", withTime?"true":"false", msTimestamp);

    return success;
}

bool
IEC104ClientConnection::sendStepCommand(int ca, int ioa, int value, bool withTime, bool select, long msTimestamp)
{
    std::string beforeLog = Iec104Utility::PluginName + " - IEC104ClientConnection::sendStepCommand - ["
                        + m_redGroup->Name() + ", " + std::to_string(m_redGroupConnection->ConnId()) + ", "
                        + m_redGroupConnection->ServerIP() + ":" + std::to_string(m_redGroupConnection->TcpPort()) + "] -";
    bool success = false;

    std::lock_guard<std::mutex> lock(m_conLock);

    if ((m_connection != nullptr) && (m_connectionState == CON_STATE_CONNECTED_ACTIVE))
    {
        InformationObject cmdObj = nullptr;

        if (withTime) {
            struct sCP56Time2a ts;

            CP56Time2a_createFromMsTimestamp(&ts, msTimestamp);

            cmdObj = (InformationObject)StepCommandWithCP56Time2a_create(NULL, ioa, (StepCommandValue)value, select, 0, &ts);
        }
        else {
            cmdObj = (InformationObject)StepCommand_create(NULL, ioa, (StepCommandValue)value, select, 0);
        }

        if (cmdObj) {
            if (CS104_Connection_sendProcessCommandEx(m_connection, CS101_COT_ACTIVATION, ca, cmdObj)) {
                Iec104Utility::log_debug("%s step command sent (CA=%i, IOA=%i, value=%d, select=%s, withTime=%s, msTimestamp=%ld)",
                                        beforeLog.c_str(), ca, ioa, value, select?"true":"false", withTime?"true":"false", msTimestamp);
                success = true;
            }

            InformationObject_destroy(cmdObj);
        }
    }
    else {
        Iec104Utility::log_warn("%s Connection unavailable (%s) or not in connected state (%i), cannot send step command",
                                beforeLog.c_str(), (m_connection == nullptr)?"true":"false", static_cast<int>(m_connectionState));
    }

    if (!success) Iec104Utility::log_warn("%s Failed to send step command (CA=%i, IOA=%i, value=%d, select=%s, withTime=%s, msTimestamp=%ld)",
                                        beforeLog.c_str(), ca, ioa, value, select?"true":"false", withTime?"true":"false", msTimestamp);

    return success;
}

bool
IEC104ClientConnection::sendSetpointNormalized(int ca, int ioa, float value, bool withTime, long msTimestamp)
{
    std::string beforeLog = Iec104Utility::PluginName + " - IEC104ClientConnection::sendSetpointNormalized - ["
                        + m_redGroup->Name() + ", " + std::to_string(m_redGroupConnection->ConnId()) + ", "
                        + m_redGroupConnection->ServerIP() + ":" + std::to_string(m_redGroupConnection->TcpPort()) + "] -";
    bool success = false;

    std::lock_guard<std::mutex> lock(m_conLock);

    if ((m_connection != nullptr) && (m_connectionState == CON_STATE_CONNECTED_ACTIVE))
    {
        InformationObject cmdObj = nullptr;

        if (withTime) {
            struct sCP56Time2a ts;

            CP56Time2a_createFromMsTimestamp(&ts, msTimestamp);

            cmdObj = (InformationObject)SetpointCommandNormalizedWithCP56Time2a_create(NULL, ioa, value, false, 0, &ts);
        }
        else {
            cmdObj = (InformationObject)SetpointCommandNormalized_create(NULL, ioa, value, false, 0);
        }

        if (cmdObj) {
            if (CS104_Connection_sendProcessCommandEx(m_connection, CS101_COT_ACTIVATION, ca, cmdObj)) {
                Iec104Utility::log_debug("%s setpoint(normalized) sent (CA=%i, IOA=%i, value=%f, withTime=%s, msTimestamp=%ld)",
                                        beforeLog.c_str(), ca, ioa, value, withTime?"true":"false", msTimestamp);
                success = true;
            }

            InformationObject_destroy(cmdObj);
        }
    }
    else {
        Iec104Utility::log_warn("%s Connection unavailable (%s) or not in connected state (%i), cannot send step command",
                                beforeLog.c_str(), (m_connection == nullptr)?"true":"false", static_cast<int>(m_connectionState));
    }

    if (!success) Iec104Utility::log_warn("%s Failed to send setpoint(normalized) (CA=%i, IOA=%i, value=%f, withTime=%s, msTimestamp=%ld)",
                                        beforeLog.c_str(), ca, ioa, value, withTime?"true":"false", msTimestamp);

    return success;
}

bool
IEC104ClientConnection::sendSetpointScaled(int ca, int ioa, int value, bool withTime, long msTimestamp)
{
    std::string beforeLog = Iec104Utility::PluginName + " - IEC104ClientConnection::sendSetpointScaled - ["
                        + m_redGroup->Name() + ", " + std::to_string(m_redGroupConnection->ConnId()) + ", "
                        + m_redGroupConnection->ServerIP() + ":" + std::to_string(m_redGroupConnection->TcpPort()) + "] -";
    bool success = false;

    std::lock_guard<std::mutex> lock(m_conLock);

    if ((m_connection != nullptr) && (m_connectionState == CON_STATE_CONNECTED_ACTIVE))
    {
        InformationObject cmdObj = nullptr;

        if (withTime) {
            struct sCP56Time2a ts;

            CP56Time2a_createFromMsTimestamp(&ts, msTimestamp);

            cmdObj = (InformationObject)SetpointCommandScaledWithCP56Time2a_create(NULL, ioa, value, false, 0, &ts);
        }
        else {
            cmdObj = (InformationObject)SetpointCommandScaled_create(NULL, ioa, value, false, 0);
        }

        if (cmdObj) {
            if (CS104_Connection_sendProcessCommandEx(m_connection, CS101_COT_ACTIVATION, ca, cmdObj)) {
                Iec104Utility::log_debug("%s setpoint(scaled) sent (CA=%i, IOA=%i, value=%d, withTime=%s, msTimestamp=%ld)",
                                        beforeLog.c_str(), ca, ioa, value, withTime?"true":"false", msTimestamp);
                success = true;
            }

            InformationObject_destroy(cmdObj);
        }
    }
    else {
        Iec104Utility::log_warn("%s Connection unavailable (%s) or not in connected state (%i), cannot send step command",
                                beforeLog.c_str(), (m_connection == nullptr)?"true":"false", static_cast<int>(m_connectionState));
    }

    if (!success) Iec104Utility::log_warn("%s Failed to send setpoint(scaled) (CA=%i, IOA=%i, value=%d, withTime=%s, msTimestamp=%ld)",
                                        beforeLog.c_str(), ca, ioa, value, withTime?"true":"false", msTimestamp);

    return success;
}

bool
IEC104ClientConnection::sendSetpointShort(int ca, int ioa, float value, bool withTime, long msTimestamp)
{
    std::string beforeLog = Iec104Utility::PluginName + " - IEC104ClientConnection::sendSetpointShort - ["
                        + m_redGroup->Name() + ", " + std::to_string(m_redGroupConnection->ConnId()) + ", "
                        + m_redGroupConnection->ServerIP() + ":" + std::to_string(m_redGroupConnection->TcpPort()) + "] -";
    bool success = false;

    std::lock_guard<std::mutex> lock(m_conLock);

    if ((m_connection != nullptr) && (m_connectionState == CON_STATE_CONNECTED_ACTIVE))
    {
        InformationObject cmdObj = nullptr;

        if (withTime) {
            struct sCP56Time2a ts;

            CP56Time2a_createFromMsTimestamp(&ts, msTimestamp);

            cmdObj = (InformationObject)SetpointCommandShortWithCP56Time2a_create(NULL, ioa, value, false, 0, &ts);
        }
        else {
            cmdObj = (InformationObject)SetpointCommandShort_create(NULL, ioa, value, false, 0);
        }

        if (cmdObj) {
            if (CS104_Connection_sendProcessCommandEx(m_connection, CS101_COT_ACTIVATION, ca, cmdObj)) {
                Iec104Utility::log_debug("%s setpoint(short) sent (CA=%i, IOA=%i, value=%f, withTime=%s, msTimestamp=%ld)",
                                        beforeLog.c_str(), ca, ioa, value, withTime?"true":"false", msTimestamp);
                success = true;
            }

            InformationObject_destroy(cmdObj);
        }
    }
    else {
        Iec104Utility::log_warn("%s Connection unavailable (%s) or not in connected state (%i), cannot send step command",
                                beforeLog.c_str(), (m_connection == nullptr)?"true":"false", static_cast<int>(m_connectionState));
    }

    if (!success) Iec104Utility::log_warn("%s Failed to send setpoint(short) (CA=%i, IOA=%i, value=%f, withTime=%s, msTimestamp=%ld)",
                                        beforeLog.c_str(), ca, ioa, value, withTime?"true":"false", msTimestamp);

    return success;
}

void
IEC104ClientConnection::prepareParameters()
{
    std::string beforeLog = Iec104Utility::PluginName + " - IEC104ClientConnection::prepareParameters - ["
                        + m_redGroup->Name() + ", " + std::to_string(m_redGroupConnection->ConnId()) + ", "
                        + m_redGroupConnection->ServerIP() + ":" + std::to_string(m_redGroupConnection->TcpPort()) + "] -";
    // Transport layer initialization
    sCS104_APCIParameters apci_parameters = {12, 8,  10,
                                             15, 10, 20};  // default values
    apci_parameters.k = m_redGroup->K();
    apci_parameters.w = m_redGroup->W();
    apci_parameters.t0 = m_redGroup->T0();
    apci_parameters.t1 = m_redGroup->T1();
    apci_parameters.t2 = m_redGroup->T2();
    apci_parameters.t3 = m_redGroup->T3();

    CS104_Connection_setAPCIParameters(m_connection, &apci_parameters);

    int asdu_size = m_config->AsduSize();

    // If 0 is set in the configuration file, use the maximum value (249 for IEC 104)
    if (asdu_size == 0) asdu_size = 249;

    // Application layer initialization
    sCS101_AppLayerParameters app_layer_parameters = {
        1, 1, 2, 0, 2, 3, 249};  // default values

    app_layer_parameters.originatorAddress = m_config->OrigAddr();
    app_layer_parameters.sizeOfCA = m_config->CaSize();
    app_layer_parameters.sizeOfIOA = m_config->IOASize();
    app_layer_parameters.maxSizeOfASDU = asdu_size;

    CS104_Connection_setAppLayerParameters(m_connection, &app_layer_parameters);

    Iec104Utility::log_info("%s Connection initialized", beforeLog.c_str());
}

void
IEC104ClientConnection::startNewInterrogationCycle()
{
    std::string beforeLog = Iec104Utility::PluginName + " - IEC104ClientConnection::startNewInterrogationCycle - ["
                        + m_redGroup->Name() + ", " + std::to_string(m_redGroupConnection->ConnId()) + ", "
                        + m_redGroupConnection->ServerIP() + ":" + std::to_string(m_redGroupConnection->TcpPort()) + "] -";
    /* reset end of init flag */
    m_endOfInitReceived = false;

    m_client->createListOfDatapointsInStationGroup();

    if (!m_config->GiForAllCa()) {

        m_client->updateGiStatus(IEC104Client::GiStatus::STARTED);

        int broadcastAddr = broadcastCA();
        if (sendInterrogationCommand(broadcastAddr)) {
            Iec104Utility::log_debug("%s Sent interrogation command to broadcast address %d", beforeLog.c_str(), broadcastAddr);
            m_firstGISent = true;
            m_interrogationInProgress = true;
            m_interrogationRequestState = 1;
            m_interrogationRequestSent = getMonotonicTimeInMs();
            m_nextGIStartTime = m_interrogationRequestSent + (m_config->GiCycle() * 1000);
        }
        else {
            Iec104Utility::log_error("%s Failed to send interrogation command to broadcast address %d", beforeLog.c_str(), broadcastAddr);
            m_firstGISent = true;
        }
    }
    else {
        Iec104Utility::log_debug("%s Prepare interrogation command for all CA", beforeLog.c_str());
        m_listOfCA_it = m_config->ListOfCAs().begin();

        m_firstGISent = true;

        if (m_listOfCA_it != m_config->ListOfCAs().end()) {
            m_interrogationInProgress = true;

            m_client->updateGiStatus(IEC104Client::GiStatus::STARTED);
        }
    }
}

void
IEC104ClientConnection::closeConnection()
{
    std::string beforeLog = Iec104Utility::PluginName + " - IEC104ClientConnection::closeConnection - ["
                        + m_redGroup->Name() + ", " + std::to_string(m_redGroupConnection->ConnId()) + ", "
                        + m_redGroupConnection->ServerIP() + ":" + std::to_string(m_redGroupConnection->TcpPort()) + "] -";
    Iec104Utility::log_info("%s Closing connection (%s)", beforeLog.c_str(), (m_connection != nullptr)?"true":"false");

    if (m_connection) {
        CS104_Connection_close(m_connection);
    }

    Iec104Utility::log_info("%s Connection closed", beforeLog.c_str());
}

void
IEC104ClientConnection::executePeriodicTasks()
{
    std::string beforeLog = Iec104Utility::PluginName + " - IEC104ClientConnection::executePeriodicTasks - ["
                        + m_redGroup->Name() + ", " + std::to_string(m_redGroupConnection->ConnId()) + ", "
                        + m_redGroupConnection->ServerIP() + ":" + std::to_string(m_redGroupConnection->TcpPort()) + "] -";
    /* do time synchroniation when enabled */
    if (m_config->isTimeSyncEnabled()) {

        bool sendTimeSyncCommand = false;

        std::lock_guard<std::mutex> lock(m_conLock);
        /* send first time sync after connection was activated */
        if ((m_timeSynchronized == false) && (m_firstTimeSyncOperationCompleted == false) && (m_timeSyncCommandSent == false)) {
            sendTimeSyncCommand = true;
        }

        /* send periodic time sync command when configured */
        if ((m_timeSynchronized == true) && (m_timeSyncCommandSent == false)) {
            uint64_t currentTime = getMonotonicTimeInMs();

            if (currentTime >= m_nextTimeSync) {
                sendTimeSyncCommand = true;

                m_nextTimeSync = currentTime + (m_config->TimeSyncPeriod() * 1000);
            }
        }

        if (sendTimeSyncCommand) {
            struct sCP56Time2a ts;

            CP56Time2a_createFromMsTimestamp(&ts, Hal_getTimeInMs());

            int ca = m_config->TimeSyncCa();

            if (ca == -1)
                ca = m_config->DefaultCa();

            if (ca == -1)
                ca = broadcastCA();

            if (CS104_Connection_sendClockSyncCommand(m_connection, ca, &ts)) {
                Iec104Utility::log_info("%s Sent clock sync command (CA=%d)...", beforeLog.c_str(), ca);
                m_timeSyncCommandSent = true;
            }
            else {
                Iec104Utility::log_error("%s Failed to send clock sync command (CA=%d)", beforeLog.c_str(), ca);
            }
        }
    }

    if ((m_config->isTimeSyncEnabled() == false) || (m_firstTimeSyncOperationCompleted == true))
    {
        if (m_config->GiEnabled())
        {
            if (m_firstGISent == false)
            {
                Iec104Utility::log_debug("%s Starting first GI cycle", beforeLog.c_str());
                startNewInterrogationCycle();
            }
            else
            {
                uint64_t currentTime = getMonotonicTimeInMs();

                if (m_interrogationInProgress) {

                    if (m_interrogationRequestState != 0) {

                        int giTime = m_config->GiTime();
                        if (m_interrogationRequestState == 1) { /* wait for ACT_CON */

                            if (giTime != 0) {
                                if (currentTime > m_interrogationRequestSent + (giTime * 1000)) {
                                    Iec104Utility::log_error("%s Interrogation request timed out (no ACT_CON in %ds)", beforeLog.c_str(),
                                                            giTime);

                                    m_interrogationRequestState = 0;
                                    m_nextGIStartTime = currentTime + (m_config->GiCycle() * 1000);

                                    m_client->updateGiStatus(IEC104Client::GiStatus::FAILED);

                                    m_client->updateQualityForDataObjectsNotReceivedInGIResponse(IEC60870_QUALITY_INVALID);

                                    closeConnection();
                                }
                            }
                        }
                        else if (m_interrogationRequestState == 2) { /* wait for ACT_TERM */

                            if (giTime != 0) {
                                if (currentTime > m_interrogationRequestSent + (giTime * 1000)) {
                                    Iec104Utility::log_error("%s Interrogation request timed out (no ACT_TERM in %ds)", beforeLog.c_str(),
                                                            giTime);

                                    m_nextGIStartTime = m_config->GiCycle();
                                    m_interrogationRequestState = 0;
                                    m_nextGIStartTime = currentTime + (m_config->GiCycle() * 1000);

                                    m_client->updateGiStatus(IEC104Client::GiStatus::FAILED);

                                    m_client->updateQualityForDataObjectsNotReceivedInGIResponse(IEC60870_QUALITY_INVALID);

                                    closeConnection();
                                }
                            }
                        }
                    }
                    else {

                        if (m_config->GiForAllCa()) {

                            if (m_listOfCA_it != m_config->ListOfCAs().end()) {
                                if (sendInterrogationCommand(*m_listOfCA_it)) {
                                    Iec104Utility::log_debug("%s Sent GI request to CA=%i", beforeLog.c_str(), *m_listOfCA_it);
                                    m_interrogationRequestState = 1;
                                    m_interrogationRequestSent = getMonotonicTimeInMs();

                                    m_client->updateGiStatus(IEC104Client::GiStatus::STARTED); //TODO is STARTED or IN_PROGRESS?
                                }
                                else {
                                    Iec104Utility::log_error("%s Failed to send interrogation command to CA=%i!", beforeLog.c_str(),
                                                            *m_listOfCA_it);

                                    m_client->updateGiStatus(IEC104Client::GiStatus::FAILED);

                                    m_client->updateQualityForDataObjectsNotReceivedInGIResponse(IEC60870_QUALITY_INVALID);

                                    closeConnection();
                                }

                                m_listOfCA_it++;
                            }
                            else {
                                Iec104Utility::log_debug("%s GI sent to all CA succesfully", beforeLog.c_str());
                                m_interrogationInProgress = false;
                                m_nextGIStartTime = currentTime + (m_config->GiCycle() * 1000);
                            }
                        }
                        else {
                            Iec104Utility::log_debug("%s GI sent to single CA, end interrogation", beforeLog.c_str());
                            m_interrogationInProgress = false;
                        }
                    }
                }
                else
                {
                    if ((m_config->GiCycle() > 0) && (currentTime > m_nextGIStartTime)) {
                        Iec104Utility::log_debug("%s Starting new GI cycle", beforeLog.c_str());
                        startNewInterrogationCycle();
                    }
                    else if (m_endOfInitReceived) {
                        Iec104Utility::log_debug("%s Starting GI cycle after end of init", beforeLog.c_str());
                        startNewInterrogationCycle();
                    }
                    if (getGiRequested() && (m_client->getGiStatus() == IEC104Client::GiStatus::FAILED || m_client->getGiStatus() == IEC104Client::GiStatus::FINISHED)) {
                        Iec104Utility::log_debug("%s Starting GI cycle on request.", beforeLog.c_str());
                        setGiRequested(false);
                        startNewInterrogationCycle();
                    }
                    else if (m_giPendingCount > 0 && (m_client->getGiStatus() == IEC104Client::GiStatus::FAILED || m_client->getGiStatus() == IEC104Client::GiStatus::FINISHED)) {
                        Iec104Utility::log_debug("%s Starting Scheduled GI cycle on request.", beforeLog.c_str());
                        m_giPendingCount--;
                        startNewInterrogationCycle();
                    }
                }
            }
        }
    }
}
/**

*/
bool IEC104ClientConnection::addScheduledGI(){
    int old = m_giPendingCount.load();
    if (old < MAX_GI_PENDING) {
        m_giPendingCount++;
        return true;
    }
    return false;
}

bool
IEC104ClientConnection::m_asduReceivedHandler(void* parameter, int address,
                                   CS101_ASDU asdu)
{
    IEC104ClientConnection* self = static_cast<IEC104ClientConnection*>(parameter);

    std::string beforeLog = Iec104Utility::PluginName + " - IEC104ClientConnection::m_asduReceivedHandler - ["
                        + self->m_redGroup->Name() + ", "
                        + std::to_string(self->m_redGroupConnection->ConnId()) + ", "
                        + self->m_redGroupConnection->ServerIP() + ":"
                        + std::to_string(self->m_redGroupConnection->TcpPort()) + "] -";

    CS101_CauseOfTransmission cot = CS101_ASDU_getCOT(asdu);

    if (cot == CS101_COT_INTERROGATED_BY_STATION) {
        if (self->m_interrogationRequestState == 2) {
            self->m_interrogationRequestSent = getMonotonicTimeInMs();
        }
        else {
            Iec104Utility::log_warn("%s Unexpected interrogation response (state=%d, COT=%d)", beforeLog.c_str(),
                                    self->m_interrogationRequestState, cot);
        }
    }

    if (self->m_client->handleASDU(self, asdu) == false)
    {
        /* ASDU not handled */
        int typeId = CS101_ASDU_getTypeID(asdu);
        switch (typeId)
        {
            case M_EI_NA_1:
                Iec104Utility::log_info("%s Received end of initialization", beforeLog.c_str());
                self->m_endOfInitReceived = true;
                break;

            case C_CS_NA_1:
            {
                Iec104Utility::log_info("%s Received time sync response", beforeLog.c_str());

                std::lock_guard<std::mutex> lock(self->m_conLock);
                if (self->m_timeSyncCommandSent == true) {

                    if (cot == CS101_COT_ACTIVATION_CON) {
                        if (CS101_ASDU_isNegative(asdu) == false) {
                            self->m_timeSynchronized = true;

                            self->m_nextTimeSync = getMonotonicTimeInMs() + (self->m_config->TimeSyncPeriod() * 1000);

                            self->m_timeSyncCommandSent = false;
                            Iec104Utility::log_debug("%s Time synchonizatation success", beforeLog.c_str());
                        }
                        else {
                            Iec104Utility::log_error("%s Time synchonizatation failed", beforeLog.c_str());
                        }
                    }
                    else if (cot == CS101_COT_UNKNOWN_TYPE_ID) {

                        Iec104Utility::log_warn("%s Time synchronization not supported by remote", beforeLog.c_str());

                        self->m_timeSynchronized = true;
                    }
                    else {
                        Iec104Utility::log_error("%s Time synchonizatation has invalid COT: %d", beforeLog.c_str(), cot);
                    }
                }
                else {
                    if (cot == CS101_COT_ACTIVATION_CON) {
                        Iec104Utility::log_warn("%s Unexpected time sync response", beforeLog.c_str());
                    }
                    else if (cot == CS101_COT_SPONTANEOUS) {
                        Iec104Utility::log_warn("%s Received remote clock time", beforeLog.c_str());
                    }
                    else {
                        Iec104Utility::log_warn("%s Unexpected time sync message (COT=%d)", beforeLog.c_str(), cot);
                    }
                }

                self->m_firstTimeSyncOperationCompleted = true;

                break;
            }

            case C_IC_NA_1:
                {
                    Iec104Utility::log_debug("%s Received C_IC_NA_1 with COT=%i", beforeLog.c_str(), cot);

                    if (cot == CS101_COT_ACTIVATION_CON) {
                        if (self->m_interrogationRequestState == 1) {
                            self->m_interrogationRequestState = 2;

                            if (CS101_ASDU_isNegative(asdu)) {
                                self->m_client->updateGiStatus(IEC104Client::GiStatus::FAILED);

                                self->m_client->updateQualityForDataObjectsNotReceivedInGIResponse(IEC60870_QUALITY_INVALID);
                                Iec104Utility::log_debug("%s Received negative ACT_CON", beforeLog.c_str());
                            }
                            else {
                                self->m_client->updateGiStatus(IEC104Client::GiStatus::IN_PROGRESS);
                                Iec104Utility::log_debug("%s Received positive ACT_CON", beforeLog.c_str());
                            }
                        }
                        else {
                            Iec104Utility::log_warn("%s Unexpected ACT_CON (state: %i)", beforeLog.c_str(), self->m_interrogationRequestState);
                        }
                    }
                    else if (cot == CS101_COT_ACTIVATION_TERMINATION) {
                        if (self->m_interrogationRequestState == 2) {
                            self->m_interrogationRequestState = 0;

                            auto giStatus = self->m_client->getGiStatus();

                            if ((giStatus == IEC104Client::GiStatus::STARTED) || (giStatus == IEC104Client::GiStatus::IN_PROGRESS)) {
                                self->m_client->updateGiStatus(IEC104Client::GiStatus::FINISHED);

                                self->m_client->updateQualityForDataObjectsNotReceivedInGIResponse(IEC60870_QUALITY_INVALID);

                                Iec104Utility::log_debug("%s Received ACT_TERM", beforeLog.c_str());
                            }
                            else {
                                Iec104Utility::log_warn("%s Unexpected ACT_TERM (giStatus: %i)", beforeLog.c_str(), static_cast<int>(giStatus));
                            }
                        }
                        else {
                            Iec104Utility::log_warn("%s Unexpected ACT_TERM (state: %i)", beforeLog.c_str(), self->m_interrogationRequestState);
                        }
                    }
                    else {
                        auto giStatus = self->m_client->getGiStatus();

                        if ((giStatus == IEC104Client::GiStatus::STARTED) || (giStatus == IEC104Client::GiStatus::IN_PROGRESS)) {
                            self->m_client->updateGiStatus(IEC104Client::GiStatus::FAILED);

                            self->m_client->updateQualityForDataObjectsNotReceivedInGIResponse(IEC60870_QUALITY_INVALID);

                            self->Disonnect();
                            Iec104Utility::log_debug("%s GI failed", beforeLog.c_str(), cot);
                        }
                        else {
                            Iec104Utility::log_warn("%s Unexpected GI message (giStatus: %d)", beforeLog.c_str(),
                                                    self->m_interrogationRequestState, cot);
                        }
                    }
                }
                break;

            case C_TS_TA_1:
                Iec104Utility::log_info("%s Test command with time tag CP56Time2a", beforeLog.c_str());
                break;

            default:
                Iec104Utility::log_debug("%s Type of message (%s (%i), COT: %i) not supported", beforeLog.c_str(),
                                        IEC104ClientConfig::getStringFromTypeID(typeId).c_str(), typeId, cot);
                return false;
        }
    }

    return true;
}

bool
IEC104ClientConnection::prepareConnection()
{
    std::string beforeLog = Iec104Utility::PluginName + " - IEC104ClientConnection::prepareConnection - ["
                        + m_redGroup->Name() + ", " + std::to_string(m_redGroupConnection->ConnId()) + ", "
                        + m_redGroupConnection->ServerIP() + ":" + std::to_string(m_redGroupConnection->TcpPort()) + "] -";
    bool success = false;

    if (m_connection == nullptr)
    {
        if (m_redGroup->UseTLS())
        {
            TLSConfiguration tlsConfig = TLSConfiguration_create();

            bool tlsConfigOk = true;

            string certificateStore = getDataDir() + string("/etc/certs/");
            string certificateStorePem = getDataDir() + string("/etc/certs/pem/");

            if (m_config->GetOwnCertificate().length() == 0 || m_config->GetPrivateKey().length() == 0) {
                Iec104Utility::log_error("%s No private key and/or certificate configured for client", beforeLog.c_str());
                tlsConfigOk = false;
            }
            else {
                string privateKeyFile = certificateStore + m_config->GetPrivateKey();

                if (access(privateKeyFile.c_str(), R_OK) == 0) {
                    if (TLSConfiguration_setOwnKeyFromFile(tlsConfig, privateKeyFile.c_str(), NULL) == false) {
                        Iec104Utility::log_error("%s Failed to load private key file: %s", beforeLog.c_str(), privateKeyFile.c_str());
                        tlsConfigOk = false;
                    }
                    else {
                        Iec104Utility::log_info("%s Loaded private key file: %s", beforeLog.c_str(), privateKeyFile.c_str());
                    }
                }
                else {
                    Iec104Utility::log_error("%s Failed to access private key file: %s", beforeLog.c_str(), privateKeyFile.c_str());
                    tlsConfigOk = false;
                }

                string clientCert =  m_config->GetOwnCertificate();
                bool isPemClientCertificate = clientCert.rfind(".pem") == clientCert.size() - 4;

                string clientCertFile;

                if(isPemClientCertificate)
                    clientCertFile = certificateStorePem + clientCert;
                else
                    clientCertFile = certificateStore + clientCert;


                if (access(clientCertFile.c_str(), R_OK) == 0) {
                    if (TLSConfiguration_setOwnCertificateFromFile(tlsConfig, clientCertFile.c_str()) == false) {
                        Iec104Utility::log_error("%s Failed to load client certificate file: %s", beforeLog.c_str(), clientCertFile.c_str());
                        tlsConfigOk = false;
                    }
                    else {
                        Iec104Utility::log_info("%s Loaded client certificate file: %s", beforeLog.c_str(), clientCertFile.c_str());
                    }
                }
                else {
                    Iec104Utility::log_error("%s Failed to access client certificate file: %s", beforeLog.c_str(), clientCertFile.c_str());
                    tlsConfigOk = false;
                }
            }

            if (m_config->GetRemoteCertificates().size() > 0) {
                TLSConfiguration_setAllowOnlyKnownCertificates(tlsConfig, true);

                for (std::string& remoteCert : m_config->GetRemoteCertificates())
                {
                    bool isPemRemoteCertificate = remoteCert.rfind(".pem") == remoteCert.size() - 4;

                    string remoteCertFile;

                    if(isPemRemoteCertificate)
                        remoteCertFile = certificateStorePem + remoteCert;
                    else
                        remoteCertFile = certificateStore + remoteCert;

                    if (access(remoteCertFile.c_str(), R_OK) == 0) {
                        if (TLSConfiguration_addAllowedCertificateFromFile(tlsConfig, remoteCertFile.c_str()) == false) {
                            Iec104Utility::log_warn("%s Failed to load remote certificate file: %s -> ignore certificate", beforeLog.c_str(),
                                                    remoteCertFile.c_str());
                        }
                        else {
                            Iec104Utility::log_info("%s Loaded remote certificate file: %s", beforeLog.c_str(), remoteCertFile.c_str());
                        }
                    }
                    else {
                        Iec104Utility::log_warn("%s Failed to access remote certificate file: %s -> ignore certificate", beforeLog.c_str(),
                                                remoteCertFile.c_str());
                    }
                }
            }
            else {
                Iec104Utility::log_info("%s Allowed unknown certificates", beforeLog.c_str());
                TLSConfiguration_setAllowOnlyKnownCertificates(tlsConfig, false);
            }

            if (m_config->GetCaCertificates().size() > 0) {
                TLSConfiguration_setChainValidation(tlsConfig, true);

                for (std::string& caCert : m_config->GetCaCertificates())
                {
                    bool isPemCaCertificate = caCert.rfind(".pem") == caCert.size() - 4;

                    string caCertFile;

                    if(isPemCaCertificate)
                        caCertFile = certificateStorePem + caCert;
                    else
                        caCertFile = certificateStore + caCert;

                    if (access(caCertFile.c_str(), R_OK) == 0) {
                        if (TLSConfiguration_addCACertificateFromFile(tlsConfig, caCertFile.c_str()) == false) {
                            Iec104Utility::log_warn("%s Failed to load CA certificate file: %s -> ignore certificate", beforeLog.c_str(),
                                                    caCertFile.c_str());
                        }
                        else {
                            Iec104Utility::log_info("%s Allowed CA certificate file: %s", beforeLog.c_str(), caCertFile.c_str());
                        }
                    }
                    else {
                        Iec104Utility::log_warn("%s Failed to access CA certificate file: %s -> ignore certificate", beforeLog.c_str(),
                                                caCertFile.c_str());
                    }
                }
            }
            else {
                Iec104Utility::log_info("%s Disabled chain validation", beforeLog.c_str());
                TLSConfiguration_setChainValidation(tlsConfig, false);
            }

            if (tlsConfigOk) {
                Iec104Utility::log_info("%s Creating connection with TLS configuration above", beforeLog.c_str());
                TLSConfiguration_setRenegotiationTime(tlsConfig, 60000);

                m_connection = CS104_Connection_createSecure(m_redGroupConnection->ServerIP().c_str(), m_redGroupConnection->TcpPort(), tlsConfig);

                if (m_connection) {
                    m_tlsConfig = tlsConfig;
                }
                else {
                    TLSConfiguration_destroy(tlsConfig);
                }
            }
            else {
                Iec104Utility::log_error("%s TLS configuration failed", beforeLog.c_str());
            }
        }
        else {
            Iec104Utility::log_info("%s Creating connection with TLS disabled", beforeLog.c_str());
            m_connection = CS104_Connection_create(m_redGroupConnection->ServerIP().c_str(), m_redGroupConnection->TcpPort());
        }

        if (m_connection) {
            prepareParameters();

            if (!m_redGroupConnection->ClientIP().empty()) {
                CS104_Connection_setLocalAddress(m_connection, m_redGroupConnection->ClientIP().c_str(), 0);
            }

            CS104_Connection_setASDUReceivedHandler(m_connection, m_asduReceivedHandler, this);

            CS104_Connection_setConnectionHandler(m_connection, m_connectionHandler, this);

            success = true;
            Iec104Utility::log_info("%s CS 104 connection started", beforeLog.c_str());
        }
        else {
            Iec104Utility::log_error("%s Failed to create CS 104 connection", beforeLog.c_str());
        }
    }

    return success;
}

void
IEC104ClientConnection::Start()
{
    std::string beforeLog = Iec104Utility::PluginName + " - IEC104ClientConnection::Start - ["
                        + m_redGroup->Name() + ", " + std::to_string(m_redGroupConnection->ConnId()) + ", "
                        + m_redGroupConnection->ServerIP() + ":" + std::to_string(m_redGroupConnection->TcpPort()) + "] -";
    Iec104Utility::log_info("%s Starting connection (started=%s)...", beforeLog.c_str(), m_started?"true":"false");
    if (m_started == false)
    {
        m_connect = m_redGroupConnection->Conn();

        m_started = true;

        m_conThread = std::make_shared<std::thread>(&IEC104ClientConnection::_conThread, this);
    }
    Iec104Utility::log_info("%s Connection started", beforeLog.c_str());
}

void
IEC104ClientConnection::Disonnect()
{
    std::string beforeLog = Iec104Utility::PluginName + " - IEC104ClientConnection::Disonnect - ["
                        + m_redGroup->Name() + ", " + std::to_string(m_redGroupConnection->ConnId()) + ", "
                        + m_redGroupConnection->ServerIP() + ":" + std::to_string(m_redGroupConnection->TcpPort()) + "] -";
    Iec104Utility::log_info("%s Disconnecting", beforeLog.c_str());
    m_disconnect = true;
    m_connect = false;
}

void
IEC104ClientConnection::Connect()
{
    std::string beforeLog = Iec104Utility::PluginName + " - IEC104ClientConnection::Connect - ["
                        + m_redGroup->Name() + ", " + std::to_string(m_redGroupConnection->ConnId()) + ", "
                        + m_redGroupConnection->ServerIP() + ":" + std::to_string(m_redGroupConnection->TcpPort()) + "] -";
    Iec104Utility::log_info("%s Connecting", beforeLog.c_str());
    m_disconnect = false;
    m_connect = true;
}

bool
IEC104ClientConnection::Autostart() const {
    return m_redGroupConnection->Start();
}

void
IEC104ClientConnection::Stop()
{
    std::string beforeLog = Iec104Utility::PluginName + " - IEC104ClientConnection::Stop - ["
                        + m_redGroup->Name() + ", " + std::to_string(m_redGroupConnection->ConnId()) + ", "
                        + m_redGroupConnection->ServerIP() + ":" + std::to_string(m_redGroupConnection->TcpPort()) + "] -";
    Iec104Utility::log_info("%s Stopping connection (started=%s)...", beforeLog.c_str(), m_started?"true":"false");
    if (m_started == true)
    {
        m_started = false;

        Iec104Utility::log_info("%s Waiting for connection thread", beforeLog.c_str());
        if (m_conThread != nullptr)
        {
            m_conThread->join();
            m_conThread = nullptr;
        }
    }
    Iec104Utility::log_info("%s Connection stopped", beforeLog.c_str());
}

void
IEC104ClientConnection::_conThread()
{
    std::string beforeLog = Iec104Utility::PluginName + " - IEC104ClientConnection::_conThread - ["
                        + m_redGroup->Name() + ", " + std::to_string(m_redGroupConnection->ConnId()) + ", "
                        + m_redGroupConnection->ServerIP() + ":" + std::to_string(m_redGroupConnection->TcpPort()) + "] -";
    while (m_started)
    {
        ConState oldConnectionState = m_connectionState;
        switch (m_connectionState) {

            case CON_STATE_IDLE:
                if (m_connect) {
                    m_startDtSent = false;

                    CS104_Connection con = nullptr;

                    {
                        std::lock_guard<std::mutex> lock(m_conLock);

                        con = m_connection;

                        m_connection = nullptr;
                    }

                    if (con != nullptr) {
                        CS104_Connection_destroy(con);
                    }

                    std::lock_guard<std::mutex> lock(m_conLock);
                    if (prepareConnection()) {
                        m_connectionState = CON_STATE_CONNECTING;
                        m_connecting = true;

                        m_delayExpirationTime = getMonotonicTimeInMs() + 10000;

                        CS104_Connection_connectAsync(m_connection);

                        Iec104Utility::log_info("%s Connecting...", beforeLog.c_str());
                    }
                    else {
                        m_connectionState = CON_STATE_FATAL_ERROR;
                        Iec104Utility::log_error("%s Fatal configuration error", beforeLog.c_str());
                    }
                }

                break;

            case CON_STATE_CONNECTING:
                /* wait for connected event or timeout */

                if (getMonotonicTimeInMs() > m_delayExpirationTime) {
                    Iec104Utility::log_warn("%s Timeout while connecting (%lldms)", beforeLog.c_str(), m_delayExpirationTime);
                    m_connectionState = CON_STATE_IDLE;
                }

                break;

            case CON_STATE_CONNECTED_INACTIVE:
                /* wait for Activate signal */
                m_sendConnectionStatusAudit("passive");
                break;

            case CON_STATE_CONNECTED_ACTIVE:
                m_sendConnectionStatusAudit("active");

                executePeriodicTasks();

                break;

            case CON_STATE_CLOSED:
                m_sendConnectionStatusAudit("disconnected");

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

        if (m_disconnect)
        {
            Iec104Utility::log_debug("%s Disconnect requested -> terminate connection", beforeLog.c_str());
            CS104_Connection con = nullptr;

            {
                std::lock_guard<std::mutex> lock(m_conLock);

                m_connected = false;
                m_connecting = false;
                m_disconnect = false;

                con = m_connection;

                m_connection = nullptr;
            }

            if (con) {
                CS104_Connection_destroy(con);
            }

            m_sendConnectionStatusAudit("disconnected");
        }

        if (m_connectionState != oldConnectionState) {
            Iec104Utility::log_debug("%s New internal connection state: %d", beforeLog.c_str(), m_connectionState);
        }

        Thread_sleep(50);
    }


    CS104_Connection con = nullptr;
    TLSConfiguration tlsConfig = nullptr;

    {
        std::lock_guard<std::mutex> lock(m_conLock);

        con = m_connection;
        tlsConfig = m_tlsConfig;

        m_connection = nullptr;
        m_tlsConfig = nullptr;
    }

    if (con) {
        CS104_Connection_destroy(con);
    }

    if (tlsConfig) {
        TLSConfiguration_destroy(tlsConfig);
    }
    m_sendConnectionStatusAudit("disconnected");
    Iec104Utility::log_debug("%s Connection thread terminated", beforeLog.c_str());
}


void
IEC104ClientConnection::m_sendConnectionStatusAudit(const std::string& auditType)
{
    if (auditType == m_last_audit) {
        return;
    }
    std::function<void(const std::string&, const std::string&, bool)> auditFn = Iec104Utility::audit_info;
    if (auditType == "disconnected") {
        auditFn = Iec104Utility::audit_fail;
    }
    else if (auditType == "passive" || auditType == "active") {
        auditFn = Iec104Utility::audit_success;
    }
    auditFn("SRVFL", m_client->getServiceName() + "-" + std::to_string(m_redGroup->Index()) + "-" + m_path_letter + "-" + auditType, true);
    m_last_audit = auditType;
}