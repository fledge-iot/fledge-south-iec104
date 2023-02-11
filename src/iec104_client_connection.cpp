#include "iec104.h"

#include <logger.h>
#include <ctime>
#include <utils.h>

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
    Stop();
}

void
IEC104ClientConnection::Activate()
{
    if (m_connectionState == CON_STATE_CONNECTED_INACTIVE) {

        m_conLock.lock();

        if (m_connection)
            CS104_Connection_sendStartDT(m_connection);

        m_conLock.unlock();

        m_startDtSent = true;

        Logger::getLogger()->info("Sent START-DT to %s\n", m_redGroupConnection->ServerIP().c_str());

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

    Logger::getLogger()->debug("Connection state changed: " + std::to_string(event));

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
}

bool
IEC104ClientConnection::sendInterrogationCommand(int ca)
{
    bool success = false;

    m_conLock.lock();

    if ((m_connection != nullptr) && (m_connectionState == CON_STATE_CONNECTED_ACTIVE)) 
    {
        if (CS104_Connection_sendInterrogationCommand(m_connection, CS101_COT_ACTIVATION, ca, IEC60870_QOI_STATION)) {
            Logger::getLogger()->debug("Interrogation command sent (CA=%i)", ca);
            success = true;
        }
        else {
            Logger::getLogger()->warn("Failed to send interrogation command");
        }
    }

    m_conLock.unlock();

    return success;
}

bool
IEC104ClientConnection::sendSingleCommand(int ca, int ioa, bool value, bool withTime, bool select)
{
    bool success = false;

    m_conLock.lock();

    if ((m_connection != nullptr) && (m_connectionState == CON_STATE_CONNECTED_ACTIVE)) 
    {
        InformationObject cmdObj = nullptr;

        if (withTime) {
            struct sCP56Time2a ts;
            
            CP56Time2a_createFromMsTimestamp(&ts, Hal_getTimeInMs());

            cmdObj = (InformationObject)SingleCommandWithCP56Time2a_create(NULL, ioa, value, select, 0, &ts);
        }
        else {
            cmdObj = (InformationObject)SingleCommand_create(NULL, ioa, value, select, 0);
        }

        if (cmdObj) {
            if (CS104_Connection_sendProcessCommandEx(m_connection, CS101_COT_ACTIVATION, ca, cmdObj)) {
                Logger::getLogger()->debug("single command sent");
                success = true;
            }

            InformationObject_destroy(cmdObj);
        }  
    }

    m_conLock.unlock();

    if (!success) Logger::getLogger()->warn("Failed to send single command");

    return success;
}

bool
IEC104ClientConnection::sendDoubleCommand(int ca, int ioa, int value, bool withTime, bool select)
{
    bool success = false;

    m_conLock.lock();

    if ((m_connection != nullptr) && (m_connectionState == CON_STATE_CONNECTED_ACTIVE)) 
    {
        InformationObject cmdObj = nullptr;

        if (withTime) {
            struct sCP56Time2a ts;
            
            CP56Time2a_createFromMsTimestamp(&ts, Hal_getTimeInMs());

            cmdObj = (InformationObject)DoubleCommandWithCP56Time2a_create(NULL, ioa, value, select, 0, &ts);
        }
        else {
            cmdObj = (InformationObject)DoubleCommand_create(NULL, ioa, value, select, 0);
        }

        if (cmdObj) {
            if (CS104_Connection_sendProcessCommandEx(m_connection, CS101_COT_ACTIVATION, ca, cmdObj)) {
                Logger::getLogger()->debug("double command sent");
                success = true;
            }

            InformationObject_destroy(cmdObj);
        }  
    }

    m_conLock.unlock();

    if (!success) Logger::getLogger()->warn("Failed to send double command");

    return success;    
}

bool
IEC104ClientConnection::sendStepCommand(int ca, int ioa, int value, bool withTime, bool select)
{
    bool success = false;

    m_conLock.lock();

    if ((m_connection != nullptr) && (m_connectionState == CON_STATE_CONNECTED_ACTIVE)) 
    {
        InformationObject cmdObj = nullptr;

        if (withTime) {
            struct sCP56Time2a ts;
            
            CP56Time2a_createFromMsTimestamp(&ts, Hal_getTimeInMs());

            cmdObj = (InformationObject)StepCommandWithCP56Time2a_create(NULL, ioa, (StepCommandValue)value, select, 0, &ts);
        }
        else {
            cmdObj = (InformationObject)StepCommand_create(NULL, ioa, (StepCommandValue)value, select, 0);
        }

        if (cmdObj) {
            if (CS104_Connection_sendProcessCommandEx(m_connection, CS101_COT_ACTIVATION, ca, cmdObj)) {
                Logger::getLogger()->debug("step command sent");
                success = true;
            }

            InformationObject_destroy(cmdObj);
        }  
    }

    m_conLock.unlock();

    if (!success) Logger::getLogger()->warn("Failed to send step command");

    return success;
}

bool
IEC104ClientConnection::sendSetpointNormalized(int ca, int ioa, float value, bool withTime)
{
    bool success = false;

    m_conLock.lock();

    if ((m_connection != nullptr) && (m_connectionState == CON_STATE_CONNECTED_ACTIVE)) 
    {
        InformationObject cmdObj = nullptr;

        if (withTime) {
            struct sCP56Time2a ts;
            
            CP56Time2a_createFromMsTimestamp(&ts, Hal_getTimeInMs());

            cmdObj = (InformationObject)SetpointCommandNormalizedWithCP56Time2a_create(NULL, ioa, value, false, 0, &ts);
        }
        else {
            cmdObj = (InformationObject)SetpointCommandNormalized_create(NULL, ioa, value, false, 0);
        }

        if (cmdObj) {
            if (CS104_Connection_sendProcessCommandEx(m_connection, CS101_COT_ACTIVATION, ca, cmdObj)) {
                Logger::getLogger()->debug("setpoint(normalized) sent");
                success = true;
            }

            InformationObject_destroy(cmdObj);
        }  
    }

    m_conLock.unlock();

    if (!success) Logger::getLogger()->warn("Failed to send setpoint(normalized)");

    return success;
}

bool
IEC104ClientConnection::sendSetpointScaled(int ca, int ioa, int value, bool withTime)
{
    bool success = false;

    m_conLock.lock();

    if ((m_connection != nullptr) && (m_connectionState == CON_STATE_CONNECTED_ACTIVE)) 
    {
        InformationObject cmdObj = nullptr;

        if (withTime) {
            struct sCP56Time2a ts;
            
            CP56Time2a_createFromMsTimestamp(&ts, Hal_getTimeInMs());

            cmdObj = (InformationObject)SetpointCommandScaledWithCP56Time2a_create(NULL, ioa, value, false, 0, &ts);
        }
        else {
            cmdObj = (InformationObject)SetpointCommandScaled_create(NULL, ioa, value, false, 0);
        }

        if (cmdObj) {
            if (CS104_Connection_sendProcessCommandEx(m_connection, CS101_COT_ACTIVATION, ca, cmdObj)) {
                Logger::getLogger()->debug("setpoint(scaled) sent");
                success = true;
            }

            InformationObject_destroy(cmdObj);
        }  
    }

    m_conLock.unlock();

    if (!success) Logger::getLogger()->warn("Failed to send setpoint(scaled)");

    return success;
}

bool
IEC104ClientConnection::sendSetpointShort(int ca, int ioa, float value, bool withTime)
{
    bool success = false;

    m_conLock.lock();

    if ((m_connection != nullptr) && (m_connectionState == CON_STATE_CONNECTED_ACTIVE)) 
    {
        InformationObject cmdObj = nullptr;

        if (withTime) {
            struct sCP56Time2a ts;
            
            CP56Time2a_createFromMsTimestamp(&ts, Hal_getTimeInMs());

            cmdObj = (InformationObject)SetpointCommandShortWithCP56Time2a_create(NULL, ioa, value, false, 0, &ts);
        }
        else {
            cmdObj = (InformationObject)SetpointCommandShort_create(NULL, ioa, value, false, 0);
        }

        if (cmdObj) {
            if (CS104_Connection_sendProcessCommandEx(m_connection, CS101_COT_ACTIVATION, ca, cmdObj)) {
                Logger::getLogger()->debug("setpoint(short) sent");
                success = true;
            }

            InformationObject_destroy(cmdObj);
        }  
    }

    m_conLock.unlock();

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
IEC104ClientConnection::startNewInterrogationCycle()
{
    /* reset end of init flag */
    m_endOfInitReceived = false;

    if (m_config->GiForAllCa() == false) {
        if (sendInterrogationCommand(broadcastCA())) {
            Logger::getLogger()->debug("Sent interrogation command to broadcase address");
            m_firstGISent = true;
            m_interrogationInProgress = true;
            m_interrogationRequestState = 1;
            m_interrogationRequestSent = getMonotonicTimeInMs();
            m_nextGIStartTime = m_interrogationRequestSent + (m_config->GiCycle() * 1000);

            m_client->updateGiStatus(IEC104Client::GiStatus::STARTED);
        }
        else {
            Logger::getLogger()->error("Failed to send interrogation command to broadcast address");
        }
    }
    else {
        m_listOfCA_it = m_config->ListOfCAs().begin();

        m_firstGISent = true;

        if (m_listOfCA_it != m_config->ListOfCAs().end()) {
            m_interrogationInProgress = true;

            m_client->updateGiStatus(IEC104Client::GiStatus::STARTED);
        }
    }
}

void 
IEC104ClientConnection::executePeriodicTasks()
{
    /* do time synchroniation when enabled */
    if (m_config->isTimeSyncEnabled()) {

        bool sendTimeSyncCommand = false;

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

            m_conLock.lock();

            m_timeSyncCommandSent = true;

            if (CS104_Connection_sendClockSyncCommand(m_connection, ca, &ts)) {
                Logger::getLogger()->info("Sent clock sync command ...");
            }
            else {
                Logger::getLogger()->error("Failed to send clock sync command");
            }

            m_conLock.unlock();
        }
    }
    
    if ((m_config->isTimeSyncEnabled() == false) || (m_firstTimeSyncOperationCompleted == true)) 
    {
        if (m_config->GiEnabled())
        {
            if (m_firstGISent == false)
            {
                startNewInterrogationCycle();
            }
            else 
            {
                uint64_t currentTime = getMonotonicTimeInMs();

                if (m_interrogationInProgress) {

                    if (m_interrogationRequestState != 0) {

                        if (m_interrogationRequestState == 1) { /* wait for ACT_CON */

                            if (m_config->GiTime() != 0) {
                                if (currentTime > m_interrogationRequestSent + (m_config->GiTime() * 1000)) {
                                    Logger::getLogger()->error("Interrogation request timed out (no ACT_CON)");

                                    m_interrogationRequestState = 0;
                                    m_nextGIStartTime = currentTime + (m_config->GiCycle() * 1000);

                                    m_client->updateGiStatus(IEC104Client::GiStatus::FAILED);
                                }
                            }
                        }
                        else if (m_interrogationRequestState == 2) { /* wait for ACT_TERM */

                            if (m_config->GiTime() != 0) {
                                if (currentTime > m_interrogationRequestSent + (m_config->GiTime() * 1000)) {
                                    Logger::getLogger()->error("Interrogation request timed out (no ACT_TERM)");

                                    m_nextGIStartTime = m_config->GiCycle();
                                    m_interrogationRequestState = 0;
                                    m_nextGIStartTime = currentTime + (m_config->GiCycle() * 1000);

                                    m_client->updateGiStatus(IEC104Client::GiStatus::FAILED);
                                }
                            }
                        }
                    }
                    else {

                        if (m_config->GiForAllCa() == true) {

                            if (m_listOfCA_it != m_config->ListOfCAs().end()) {
                                if (sendInterrogationCommand(*m_listOfCA_it)) {
                                    Logger::getLogger()->debug("Sent GI request to CA=%i", *m_listOfCA_it);
                                    m_interrogationRequestState = 1;
                                    m_interrogationRequestSent = getMonotonicTimeInMs();

                                    m_client->updateGiStatus(IEC104Client::GiStatus::STARTED); //TODO is STARTED or IN_PROGRESS?
                                }
                                else {
                                    Logger::getLogger()->error("Failed to send interrogation command to CA=%i!\n", *m_listOfCA_it);

                                    m_client->updateGiStatus(IEC104Client::GiStatus::FAILED);
                                }

                                m_listOfCA_it++;
                            }
                            else {
                                m_interrogationInProgress = false;
                                m_nextGIStartTime = currentTime + (m_config->GiCycle() * 1000);
                            }
                        }
                        else {
                            m_interrogationInProgress = false;
                        }
                    }
                }
                else 
                {
                    if ((m_config->GiCycle() > 0) && (currentTime > m_nextGIStartTime)) {
                        startNewInterrogationCycle();
                    }
                    else if (m_endOfInitReceived) {
                        startNewInterrogationCycle();
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
            Logger::getLogger()->warn("Unexpected interrogation response");
        }
    }

    if (self->m_client->handleASDU(self, asdu) == false)
    {
        /* ASDU not handled */
        switch (CS101_ASDU_getTypeID(asdu))
        {
            case M_EI_NA_1:
                Logger::getLogger()->info("Received end of initialization");
                self->m_endOfInitReceived = true;
                break;

            case C_CS_NA_1:
                Logger::getLogger()->info("Received time sync response");
                
                if (self->m_timeSyncCommandSent == true) {

                    if (CS101_ASDU_getCOT(asdu) == CS101_COT_ACTIVATION_CON) {
                        if (CS101_ASDU_isNegative(asdu) == false) {
                            self->m_timeSynchronized = true;

                            self->m_nextTimeSync = getMonotonicTimeInMs() + (self->m_config->TimeSyncPeriod() * 1000);

                            self->m_timeSyncCommandSent = false;
                        }
                        else {
                            Logger::getLogger()->error("time synchonizatation failed");
                        }
                    }
                    else if (CS101_ASDU_getCOT(asdu) == CS101_COT_UNKNOWN_TYPE_ID) {

                        Logger::getLogger()->warn("Time synchronization not supported by remote");

                        self->m_timeSynchronized = true;
                    }

                }
                else {
                    if (CS101_ASDU_getCOT(asdu) == CS101_COT_ACTIVATION_CON) {
                        Logger::getLogger()->warn("Unexpected time sync response");
                    }
                    else if (CS101_ASDU_getCOT(asdu) == CS101_COT_SPONTANEOUS) {
                        Logger::getLogger()->warn("Received remote clock time");
                    }
                    else {
                        Logger::getLogger()->warn("Unexpected time sync message");
                    }
                }

                self->m_firstTimeSyncOperationCompleted = true;

                break;

            case C_IC_NA_1:
                { 
                    Logger::getLogger()->debug("Receivd C_IC_NA_1 with COT=%i", cot);

                    if (cot == CS101_COT_ACTIVATION_CON) {
                        if (self->m_interrogationRequestState == 1) {
                            self->m_interrogationRequestState = 2;

                            if (CS101_ASDU_isNegative(asdu)) {
                                self->m_client->updateGiStatus(IEC104Client::GiStatus::FAILED);
                            }
                            else {
                                self->m_client->updateGiStatus(IEC104Client::GiStatus::STARTED); //TODO is IN_PROGRESS?
                            }
                        }
                        else {
                            Logger::getLogger()->warn("Unexpected ACT_CON");
                        }
                    }
                    else if (cot == CS101_COT_ACTIVATION_TERMINATION) {
                        if (self->m_interrogationRequestState == 2) {
                            self->m_interrogationRequestState = 0;

                            auto giStatus = self->m_client->getGiStatus();

                            if ((giStatus == IEC104Client::GiStatus::STARTED) || (giStatus == IEC104Client::GiStatus::IN_PROGRESS)) {
                                self->m_client->updateGiStatus(IEC104Client::GiStatus::FINISHED);
                            }
                        }
                        else {
                            Logger::getLogger()->warn("Unexpected ACT_TERM");
                        }
                    }
                    else {
                        auto giStatus = self->m_client->getGiStatus();

                        if ((giStatus == IEC104Client::GiStatus::STARTED) || (giStatus == IEC104Client::GiStatus::IN_PROGRESS)) {
                            self->m_client->updateGiStatus(IEC104Client::GiStatus::FAILED);
                        }
                    }
                }
                break;

            case C_TS_TA_1:
                Logger::getLogger()->info("Test command with time tag CP56Time2a");
                break;

            default:
                Logger::getLogger()->debug("Type of message (%i - COT: %i) not supported", CS101_ASDU_getTypeID(asdu), CS101_ASDU_getCOT(asdu));
                return false;
        }
    }

    return true;
}

bool
IEC104ClientConnection::prepareConnection()
{
    bool success = false;

    if (m_connection == nullptr)
    {
        if (m_redGroup->UseTLS()) 
        {
            TLSConfiguration tlsConfig = TLSConfiguration_create();

            bool tlsConfigOk = true;

            string certificateStore = getDataDir() + string("/etc/certs/");

            if (m_config->GetOwnCertificate().length() == 0 || m_config->GetPrivateKey().length() == 0) {
                Logger::getLogger()->error("No private key and/or certificate configured for client");
                tlsConfigOk = false;
            }
            else {
                string privateKeyFile = certificateStore + m_config->GetPrivateKey();

                if (access(privateKeyFile.c_str(), R_OK) == 0) {
                    if (TLSConfiguration_setOwnKeyFromFile(tlsConfig, privateKeyFile.c_str(), NULL) == false) {
                        Logger::getLogger()->error("Failed to load private key file: %s", privateKeyFile.c_str());
                        tlsConfigOk = false;
                    }
                }
                else {
                    Logger::getLogger()->error("Failed to access private key file: %s", privateKeyFile.c_str());
                    tlsConfigOk = false;
                }

                string clientCertFile = certificateStore + m_config->GetOwnCertificate();

                if (access(clientCertFile.c_str(), R_OK) == 0) {
                    if (TLSConfiguration_setOwnCertificateFromFile(tlsConfig, clientCertFile.c_str()) == false) {
                        Logger::getLogger()->error("Failed to load client certificate file: %s", clientCertFile.c_str());
                        tlsConfigOk = false;
                    }
                }
                else {
                    Logger::getLogger()->error("Failed to access client certificate file: %s", clientCertFile.c_str());
                    tlsConfigOk = false;
                }
            }

            if (m_config->GetRemoteCertificates().size() > 0) {
                TLSConfiguration_setAllowOnlyKnownCertificates(tlsConfig, true);

                for (std::string& remoteCert : m_config->GetRemoteCertificates())
                {
                    string remoteCertFile = certificateStore + remoteCert;

                    if (access(remoteCertFile.c_str(), R_OK) == 0) {
                        if (TLSConfiguration_addAllowedCertificateFromFile(tlsConfig, remoteCertFile.c_str()) == false) {
                            Logger::getLogger()->warn("Failed to load remote certificate file: %s -> ignore certificate", remoteCertFile.c_str());
                        }
                    }
                    else {
                        Logger::getLogger()->warn("Failed to access remote certificate file: %s -> ignore certificate", remoteCertFile.c_str());
                    }

                }
            }
            else {
                TLSConfiguration_setAllowOnlyKnownCertificates(tlsConfig, false);
            }

            if (m_config->GetCaCertificates().size() > 0) {
                TLSConfiguration_setChainValidation(tlsConfig, true);

                for (std::string& caCert : m_config->GetCaCertificates())
                {
                    string caCertFile = certificateStore + caCert;

                    if (access(caCertFile.c_str(), R_OK) == 0) {
                        if (TLSConfiguration_addCACertificateFromFile(tlsConfig, caCertFile.c_str()) == false) {
                            Logger::getLogger()->warn("Failed to load CA certificate file: %s -> ignore certificate", caCertFile.c_str());
                        }
                    }
                    else {
                        Logger::getLogger()->warn("Failed to access CA certificate file: %s -> ignore certificate", caCertFile.c_str());
                    }

                }
            }
            else {
                TLSConfiguration_setChainValidation(tlsConfig, false);
            }

            if (tlsConfigOk) {

                m_connection = CS104_Connection_createSecure(m_redGroupConnection->ServerIP().c_str(), m_redGroupConnection->TcpPort(), tlsConfig);
            
                if (m_connection) {
                    m_tlsConfig = tlsConfig;
                }
                else {
                    TLSConfiguration_destroy(tlsConfig);
                }
            }
            else {
                printf("TLS configuration failed\n");
                Logger::getLogger()->error("TLS configuration failed");
            }
        }
        else {
            m_connection = CS104_Connection_create(m_redGroupConnection->ServerIP().c_str(), m_redGroupConnection->TcpPort());
        }

        if (m_connection) {
            prepareParameters();

            if (m_redGroupConnection->ClientIP() != nullptr) {
                CS104_Connection_setLocalAddress(m_connection, m_redGroupConnection->ClientIP()->c_str(), 0);
            }

            CS104_Connection_setASDUReceivedHandler(m_connection, m_asduReceivedHandler, this);

            CS104_Connection_setConnectionHandler(m_connection, m_connectionHandler, this);

            success = true;
        }
        else {
            Logger::getLogger()->error("Failed to start CS 104 connection");
        }
    }

    return success;
}

void
IEC104ClientConnection::Start()
{
    if (m_started == false) 
    {
        m_connect = m_redGroupConnection->Conn();

        m_started = true;

        m_conThread = new std::thread(&IEC104ClientConnection::_conThread, this);
    }
}

void
IEC104ClientConnection::Disonnect()
{
    m_disconnect = true;
    m_connect = false;
}

void
IEC104ClientConnection::Connect()
{
    m_disconnect = false;
    m_connect = true;
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
        if (m_connect)
        {
            switch (m_connectionState) {

                case CON_STATE_IDLE:
                    {
                        m_startDtSent = false;

                        m_conLock.lock();

                        if (m_connection != nullptr) {
                            CS104_Connection_destroy(m_connection);

                            m_connection = nullptr;
                        }

                        if (prepareConnection()) {
                            m_connectionState = CON_STATE_CONNECTING;
                            m_connecting = true;

                            m_delayExpirationTime = getMonotonicTimeInMs() + 10000;

                            CS104_Connection_connectAsync(m_connection);

                            Logger::getLogger()->info("Connecting");
                        }
                        else {
                            m_connectionState = CON_STATE_FATAL_ERROR;
                            Logger::getLogger()->error("Fatal configuration error");
                        }
                        
                        m_conLock.unlock();
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

                    executePeriodicTasks();

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
        }

        if (m_disconnect) {

            m_connected = false;
            m_connecting = false;
            m_disconnect = false;

            m_conLock.lock();

            if (m_connection) {
                CS104_Connection_destroy(m_connection);
                m_connection = nullptr;
            }

            m_conLock.unlock();
        }

        Thread_sleep(50);
    }

    m_conLock.lock();

    if (m_connection) { 
        CS104_Connection_destroy(m_connection);
        m_connection = nullptr;
    }

    if (m_tlsConfig) {
        TLSConfiguration_destroy(m_tlsConfig);
        m_tlsConfig = nullptr;
    }

    m_conLock.unlock();
}
