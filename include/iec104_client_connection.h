#ifndef IEC104_CLIENT_CONNECTION_H
#define IEC104_CLIENT_CONNECTION_H

#include <thread>
#include <mutex>
#include <vector>
#include <atomic>

#include <lib60870/cs104_connection.h>
#include <lib60870/tls_config.h>

const int MAX_GI_PENDING = 2;

class IEC104Client;
class IEC104ClientRedGroup;
class IEC104ClientConfig;
class RedGroupCon;
class IEC104ClientConnection
{
public:

    IEC104ClientConnection(std::shared_ptr<IEC104Client> client, std::shared_ptr<IEC104ClientRedGroup> redGroup,
                            std::shared_ptr<RedGroupCon> connection, std::shared_ptr<IEC104ClientConfig> config, const std::string& pathLetter);
    ~IEC104ClientConnection();

    void Start();
    void Stop();
    void Activate();

    void Disonnect();
    void Connect();

    bool Autostart() const;
    bool Disconnected() {return ((m_connecting == false) && (m_connected == false));};
    bool Connecting() {return m_connecting;};
    bool Connected() {return m_connected;};
    bool Active() {return m_active;};

    bool sendInterrogationCommand(int ca);
    void startNewInterrogationCycle();
    bool addScheduledGI();

    bool sendSingleCommand(int ca, int ioa, bool value, bool withTime, bool select, long msTimestamp);
    bool sendDoubleCommand(int ca, int ioa, int value, bool withTime, bool select, long msTimestamp);
    bool sendStepCommand(int ca, int ioa, int value, bool withTime, bool select, long msTimestamp);
    bool sendSetpointNormalized(int ca, int ioa, float value, bool withTime, long msTimestamp);
    bool sendSetpointScaled(int ca, int ioa, int value, bool withTime, long msTimestamp);
    bool sendSetpointShort(int ca, int ioa, float value, bool withTime, long msTimestamp);

    // Getter and setter for m_giRequested
    void setGiRequested(bool value){
        m_giRequested = value;
    }

    bool getGiRequested(){
        return m_giRequested;
    }

private:

    void executePeriodicTasks();
    void prepareParameters();
    bool prepareConnection();
    void closeConnection();

    void m_sendConnectionStatusAudit(const std::string& auditType);

    typedef enum {
        CON_STATE_IDLE,
        CON_STATE_CONNECTING,
        CON_STATE_CONNECTED_INACTIVE,
        CON_STATE_CONNECTED_ACTIVE,
        CON_STATE_CLOSED,
        CON_STATE_WAIT_FOR_RECONNECT,
        CON_STATE_FATAL_ERROR
    } ConState;

    std::shared_ptr<IEC104ClientConfig> m_config;
    std::shared_ptr<IEC104ClientRedGroup> m_redGroup;
    std::shared_ptr<RedGroupCon> m_redGroupConnection;
    std::shared_ptr<IEC104Client> m_client;
    /* global state information */
    bool m_connected = false; /* connection is in connected state */
    bool m_active = false;    /* connection is connected and active */
    bool m_connecting = false; /* connection is currently connecting */

    bool m_connect = false; /* flag to indicate that the connection is to be establish */
    bool m_disconnect = false; /* flag to indicate that the connection has to be disconnected */

    std::atomic<int> m_giPendingCount{0};/* gi pending count gi pending max define by MAX_GI_PENDING*/

    int broadcastCA() const;

    std::mutex m_conLock;
    CS104_Connection m_connection = nullptr;
    TLSConfiguration m_tlsConfig = nullptr;

    ConState m_connectionState = CON_STATE_IDLE;

    bool m_started = false;
    bool m_startDtSent = false;

    bool m_giRequested = false; /* Ask for a request GI to be sent. Triggered by the receptions of messages flagges gi". */
    bool m_cnxLostStatusSent = false; /* cnxLostStatus sent after reconnect */

    bool m_timeSynchronized = false;
    bool m_timeSyncCommandSent = false;
    bool m_firstTimeSyncOperationCompleted = false;
    uint64_t m_nextTimeSync = 0;

    bool m_firstGISent = false;
    bool m_interrogationInProgress = false;
    int m_interrogationRequestState = 0; /* 0 - idle, 1 - waiting for ACT_CON, 2 - waiting for ACT_TERM */
    uint64_t m_interrogationRequestSent = 0;
    uint64_t m_nextGIStartTime = 0;
    bool m_endOfInitReceived = false;

    uint64_t m_delayExpirationTime = 0;

    std::shared_ptr<std::thread> m_conThread;
    void _conThread();

    std::vector<int>::iterator m_listOfCA_it;

    std::string m_path_letter; // A or B
    std::string m_last_audit; // Used to avoid sending the same audit multiple times in a row

    static bool m_asduReceivedHandler(void* parameter, int address, CS101_ASDU asdu);

    static void m_connectionHandler(void* parameter, CS104_Connection connection,
                                 CS104_ConnectionEvent event);
};

#endif /* IEC104_CLIENT_CONNECTION_H */