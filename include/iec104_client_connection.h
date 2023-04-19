#ifndef IEC104_CLIENT_CONNECTION_H
#define IEC104_CLIENT_CONNECTION_H

#include <thread>
#include <mutex>
#include "iec104.h"
#include <lib60870/cs104_connection.h>

class IEC104Client;

class IEC104ClientConnection
{
public:

    IEC104ClientConnection(IEC104Client* client, IEC104ClientRedGroup* redGroup, RedGroupCon* connection, IEC104ClientConfig* config);
    ~IEC104ClientConnection();

    void Start();
    void Stop();
    void Activate();

    void Disonnect();
    void Connect();

    bool Autostart() {return m_redGroupConnection->Start();};
    bool Disconnected() {return ((m_connecting == false) && (m_connected == false));};
    bool Connecting() {return m_connecting;};
    bool Connected() {return m_connected;};
    bool Active() {return m_active;};

    bool sendInterrogationCommand(int ca);

    bool sendSingleCommand(int ca, int ioa, bool value, bool withTime, bool select);
    bool sendDoubleCommand(int ca, int ioa, int value, bool withTime, bool select);
    bool sendStepCommand(int ca, int ioa, int value, bool withTime, bool select);
    bool sendSetpointNormalized(int ca, int ioa, float value, bool withTime);
    bool sendSetpointScaled(int ca, int ioa, int value, bool withTime);
    bool sendSetpointShort(int ca, int ioa, float value, bool withTime);

private:

    void executePeriodicTasks();
    void prepareParameters();
    bool prepareConnection();
    void startNewInterrogationCycle();
    void closeConnection();

    typedef enum {
        CON_STATE_IDLE,
        CON_STATE_CONNECTING,
        CON_STATE_CONNECTED_INACTIVE,
        CON_STATE_CONNECTED_ACTIVE,
        CON_STATE_CLOSED,
        CON_STATE_WAIT_FOR_RECONNECT,
        CON_STATE_FATAL_ERROR
    } ConState;

    IEC104ClientConfig* m_config;
    IEC104ClientRedGroup* m_redGroup;
    RedGroupCon* m_redGroupConnection;
    IEC104Client* m_client;

    /* global state information */
    bool m_connected = false; /* connection is in connected state */
    bool m_active = false;    /* connection is connected and active */
    bool m_connecting = false; /* connection is currently connecting */

    bool m_connect = false; /* flag to indicate that the connection is to be establish */
    bool m_disconnect = false; /* flag to indicate that the connection has to be disconnected */

    int broadcastCA();

    std::mutex m_conLock;
    CS104_Connection m_connection = nullptr;
    TLSConfiguration m_tlsConfig = nullptr;
    
    ConState m_connectionState = CON_STATE_IDLE;

    bool m_started = false;
    bool m_startDtSent = false;

    bool m_cnxLostStatusSent = false; /* cnxLostStatus sent after reconnect */

    bool m_timeSynchronized = false;
    bool m_timeSyncCommandSent = false;
    bool m_firstTimeSyncOperationCompleted = false;
    uint64_t m_nextTimeSync;

    bool m_firstGISent = false;
    bool m_interrogationInProgress = false;
    int m_interrogationRequestState = 0; /* 0 - idle, 1 - waiting for ACT_CON, 2 - waiting for ACT_TERM */
    uint64_t m_interrogationRequestSent;
    uint64_t m_nextGIStartTime;
    bool m_endOfInitReceived = false;

    uint64_t m_delayExpirationTime;

    std::thread* m_conThread = nullptr;
    void _conThread();

    std::vector<int>::iterator m_listOfCA_it;

    static bool m_asduReceivedHandler(void* parameter, int address, CS101_ASDU asdu);

    static void m_connectionHandler(void* parameter, CS104_Connection connection,
                                 CS104_ConnectionEvent event);
};

#endif /* IEC104_CLIENT_CONNECTION_H */