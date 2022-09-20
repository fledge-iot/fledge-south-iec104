#include "iec104.h"

#include <logger.h>
#include <ctime>

#include <lib60870/hal_time.h>
#include <lib60870/hal_thread.h>

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

IEC104ClientConnection::IEC104ClientConnection(IEC104Client* client, IEC104ClientRedGroup* redGroup, RedGroupCon* connection, IEC104ClientConfig* config)
{
    m_config = config;
    m_redGroup = redGroup;
    m_redGroupConnection = connection;
    m_client = client;
}

IEC104ClientConnection::~IEC104ClientConnection()
{
    
}

void
IEC104ClientConnection::Activate()
{
    if (m_connectionState == CON_STATE_CONNECTED_INACTIVE) {
        CS104_Connection_sendStartDT(m_connection);
        m_startDtSent = true;

        printf("Activate con to %s\n", m_redGroupConnection->ServerIP().c_str());

        m_connectionState = CON_STATE_CONNECTED_ACTIVE;
    }
}

int 
IEC104ClientConnection::broadcastCA()
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

    Logger::getLogger()->info("Connection state changed: " + std::to_string(event));

    if (event == CS104_CONNECTION_CLOSED)
    {
        self->m_connectionState = CON_STATE_CLOSED;
        self->m_connected = false;
    }
    else if (event == CS104_CONNECTION_OPENED) 
    {
        self->m_connectionState = CON_STATE_CONNECTED_INACTIVE;
        self->m_connected = true;
    }
    else if (event == CS104_CONNECTION_STARTDT_CON_RECEIVED)
    {
        self->m_nextTimeSync = getMonotonicTimeInMs();
        self->m_timeSynchronized = false;
        self->m_timeSyncCommandSent = false;
        self->m_firstTimeSyncOperationCompleted = false;
        self->m_firstGISent = false;

        self->m_connectionState = CON_STATE_CONNECTED_ACTIVE;
    }
    else if (event == CS104_CONNECTION_STOPDT_CON_RECEIVED)
    {
        self->m_connectionState = CON_STATE_CONNECTED_INACTIVE;
    }
}

bool
IEC104ClientConnection::sendInterrogationCommand(int ca)
{
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

bool
IEC104ClientConnection::sendSingleCommand(int ca, int ioa, bool value, bool withTime, bool select)
{
    bool success = false;

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

bool
IEC104ClientConnection::sendDoubleCommand(int ca, int ioa, int value, bool withTime, bool select)
{
    bool success = false;

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

bool
IEC104ClientConnection::sendStepCommand(int ca, int ioa, int value, bool withTime, bool select)
{
    bool success = false;

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

bool
IEC104ClientConnection::sendSetpointNormalized(int ca, int ioa, float value, bool withTime)
{
    bool success = false;

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

bool
IEC104ClientConnection::sendSetpointScaled(int ca, int ioa, int value, bool withTime)
{
    bool success = false;

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

bool
IEC104ClientConnection::sendSetpointShort(int ca, int ioa, float value, bool withTime)
{
    bool success = false;

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

void 
IEC104ClientConnection::prepareParameters()
{
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

    Logger::getLogger()->info("Connection (red-group: %s IP: %s port: %i) initialized", m_redGroup->Name().c_str(), m_redGroupConnection->ServerIP().c_str(), m_redGroupConnection->TcpPort());
}

void 
IEC104ClientConnection::performPeriodicTasks()
{
    /* do time synchroniation when enabled */
    if (m_config->isTimeSyncEnabled()) {

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

            int ca = m_config->TimeSyncCa();

            if (ca == -1)
                ca = m_config->DefaultCa();

            if (ca == -1)
                ca = broadcastCA();

            if (CS104_Connection_sendClockSyncCommand(m_connection, ca, &ts)) {
                Logger::getLogger()->info("Sent clock sync command ...");

                m_timeSyncCommandSent = true;
            }
            else {
                Logger::getLogger()->error("Failed to send clock sync command");
                printf("Failed to send clock sync command!\n");
            }
        }
    }
    
    if ((m_config->isTimeSyncEnabled() == false) || (m_firstTimeSyncOperationCompleted == true)) 
    {
        if (m_firstGISent == false) {

            if (m_config->GiForAllCa() == false) {
                if (sendInterrogationCommand(broadcastCA())) {
                    m_firstGISent = true;
                    m_interrogationInProgress = true;
                    m_interrogationRequestState = 1;
                    m_interrogationRequestSent = getMonotonicTimeInMs();
                }
                else {
                    Logger::getLogger()->error("Failed to send interrogation command to broadcast address");
                    printf("Failed to send interrogation command to broadcast address!\n");
                }
            }
            else {
                m_listOfCA_it = m_config->ListOfCAs().begin();

                m_firstGISent = true;

                if (m_listOfCA_it != m_config->ListOfCAs().end()) {
                    m_interrogationInProgress = true;
                }
            }
        }
        else {

            if (m_interrogationInProgress) {

                if (m_interrogationRequestState != 0) {

                    uint64_t currentTime = getMonotonicTimeInMs();

                    if (m_interrogationRequestState == 1) { /* wait for ACT_CON */
                        if (currentTime > m_interrogationRequestSent + 1000) {
                            printf("Interrogation request timed out (no ACT_CON)!\n");
                            m_interrogationRequestState = 0;
                        }
                    }
                    else if (m_interrogationRequestState == 2) { /* wait for ACT_TERM */
                        if (currentTime > m_interrogationRequestSent + 1000) {
                            printf("Interrogation request timed out!\n");
                            m_interrogationRequestState = 0;
                        }
                    }
                }
                else {

                    if (m_config->GiForAllCa() == true) {

                        if (m_listOfCA_it != m_config->ListOfCAs().end()) {
                            if (sendInterrogationCommand(*m_listOfCA_it)) {
                                printf("Sent GI request to CA=%i\n", *m_listOfCA_it);
                                m_interrogationRequestState = 1;
                            }
                            else {
                                Logger::getLogger()->error("Failed to send interrogation command");
                                printf("Failed to send interrogation command to CA=%i!\n", *m_listOfCA_it);
                            }

                            m_listOfCA_it++;
                        }
                        else {
                            m_interrogationInProgress = false;
                        }
                    }
                    else {
                        m_interrogationInProgress = false;
                    }
                }
            }
        }
    }
}

bool
IEC104ClientConnection::m_asduReceivedHandler(void* parameter, int address,
                                   CS101_ASDU asdu)
{
    IEC104ClientConnection* self = static_cast<IEC104ClientConnection*>(parameter);

    CS101_CauseOfTransmission cot = CS101_ASDU_getCOT(asdu);

    if (cot == CS101_COT_INTERROGATED_BY_STATION) {
        if (self->m_interrogationRequestState == 2) {
            self->m_interrogationRequestSent = getMonotonicTimeInMs();
        }
        else {
            printf("Unexpected interrogation response\n");
        }
    }

    if (self->m_client->handleASDU(self, asdu) == false)
    {
        /* ASDU not handled */
        switch (CS101_ASDU_getTypeID(asdu))
        {

            case M_EI_NA_1:
                Logger::getLogger()->info("Received end of initialization");
                break;

            case C_CS_NA_1:
                Logger::getLogger()->info("Received time sync response");

                if (self->m_timeSyncCommandSent == true) {

                    if (CS101_ASDU_getCOT(asdu) == CS101_COT_ACTIVATION_CON) {
                        if (CS101_ASDU_isNegative(asdu) == false) {
                            self->m_timeSyncCommandSent = false;
                            self->m_timeSynchronized = true;

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

                self->m_firstTimeSyncOperationCompleted = true;

                break;

            case C_IC_NA_1:
                { 
                    Logger::getLogger()->info("General interrogation response");
                    printf("Receivd C_IC_NA_1 with COT=%i\n", cot);

                    if (cot == CS101_COT_ACTIVATION_CON) {
                        if (self->m_interrogationRequestState == 1) {
                            self->m_interrogationRequestSent = getMonotonicTimeInMs();
                            self->m_interrogationRequestState = 2;
                        }
                        else {
                            printf("Unexpected ACT_CON\n");
                        }
                    }
                    else if (cot == CS101_COT_ACTIVATION_TERMINATION) {
                        if (self->m_interrogationRequestState == 2) {
                            self->m_interrogationRequestState = 0;
                        }
                        else {
                            printf("Unexpected ACT_TERM\n");
                        }
                    }
                }
                break;

            case C_TS_TA_1:
                Logger::getLogger()->info("Test command with time tag CP56Time2a");
                break;

            default:
                Logger::getLogger()->error("Type of message not supported");
                return false;
        }
    }

    return true;
}

bool
IEC104ClientConnection::prepareConnection()
{
    if (m_connection == nullptr)
    {
        //TODO handle TLS

        m_connection = CS104_Connection_create(m_redGroupConnection->ServerIP().c_str(), m_redGroupConnection->TcpPort());

        if (m_connection) {
            prepareParameters();

            CS104_Connection_setASDUReceivedHandler(m_connection, m_asduReceivedHandler, this);

            CS104_Connection_setConnectionHandler(m_connection, m_connectionHandler, this);

            return true;
        }
    }

    return false;
}

void
IEC104ClientConnection::Start()
{
    if (m_started == false) 
    {
        m_started = true;

        m_conThread = new std::thread(&IEC104ClientConnection::_conThread, this);
    }
}

void
IEC104ClientConnection::Stop()
{
    if (m_started == true) 
    {
        m_started = false;

        if (m_conThread != nullptr)
        {
            m_conThread->join();
            delete m_conThread;
            m_conThread = nullptr;
        }
    }
}

void
IEC104ClientConnection::_conThread()
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

                        CS104_Connection_connectAsync(m_connection);

                        Logger::getLogger()->info("Connecting");
                    }
                    else {
                        m_connectionState = CON_STATE_FATAL_ERROR;
                        printf("Fatal configuration error\n");
                        Logger::getLogger()->error("Fatal configuration error");
                    }
                    
                }
                break;

            case CON_STATE_CONNECTING:
                /* wait for connected event or timeout */

                if (getMonotonicTimeInMs() > m_delayExpirationTime) {
                    Logger::getLogger()->warn("Timeout while connecting");
                    m_connectionState = CON_STATE_IDLE;
                }

                break;

            case CON_STATE_CONNECTED_INACTIVE:

                /* wait for Activate signal */

                break;

            case CON_STATE_CONNECTED_ACTIVE:

                performPeriodicTasks();

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

        Thread_sleep(50);
    }
}
