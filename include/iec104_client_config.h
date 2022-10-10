#ifndef IEC104_CLIENT_CONFIG_H
#define IEC104_CLIENT_CONFIG_H


#include <logger.h>
#include <map>
#include <vector>
#include "iec104_client_redgroup.h"

using namespace std;

typedef struct {
    int ca;
    int ioa;
    int typeId;
    std::string label;
} DataExchangeDefinition;

class IEC104ClientConfig
{
public:
    IEC104ClientConfig() {m_exchangeDefinitions.clear();};
    //IEC104ClientConfig(const string& protocolConfig, const string& exchangeConfig);
    ~IEC104ClientConfig();

    int LogLevel() {return 1;};

    void importProtocolConfig(const string& protocolConfig);
    void importExchangeConfig(const string& exchangeConfig);
    void importTlsConfig(const string& tlsConfig);

    int CaSize() {return m_caSize;};
    int IOASize() {return m_ioaSize;};
    int AsduSize() {return m_asduSize;};

    int DefaultCa() {return m_defaultCa;};
    int TimeSyncCa() {return m_timeSyncCa;};
    int OrigAddr() {return m_origAddr;};

    bool isTimeSyncEnabled() {return (m_timeSyncPeriod > 0);};
    int TimeSyncPeriod() {return m_timeSyncPeriod;};

    bool GiForAllCa() {return m_giAllCa;};
    int GiCycle() {return m_giCycle;};
    bool GiEnabled() {return true;};
    int GiRepeatCount() {return m_giRepeatCount;};
    int GiTime() {return m_giTime;};

    std::string& GetPrivateKeyFile() {return m_privateKeyFile;};
    std::string& GetClientCertFile() {return m_clientCertFile;};
    std::string& GetServerCertFile() {return m_serverCertFile;};
    std::string& GetCaCertFile() {return m_caCertFile;};

    static bool isValidIPAddress(const string& addrStr);

    std::vector<IEC104ClientRedGroup*>& RedundancyGroups() {return m_redundancyGroups;};

    std::map<int, std::map<int, DataExchangeDefinition*>>& ExchangeDefinition() {return m_exchangeDefinitions;};

    std::vector<int>& ListOfCAs() {return m_listOfCAs;};

    static int GetTypeIdByName(const string& name);

    std::string* checkExchangeDataLayer(int typeId, int ca, int ioa);

private:

    static bool isMessageTypeMatching(int expectedType, int rcvdType);

    void deleteExchangeDefinitions();

    std::vector<IEC104ClientRedGroup*> m_redundancyGroups = std::vector<IEC104ClientRedGroup*>();

    std::map<int, std::map<int, DataExchangeDefinition*>> m_exchangeDefinitions = std::map<int, std::map<int, DataExchangeDefinition*>>();

    std::vector<int> m_listOfCAs = std::vector<int>();
    
    int m_caSize = 2;
    int m_ioaSize = 3;
    int m_asduSize = 0;

    int m_defaultCa = 1; /* application_layer/default_ca */
    int m_timeSyncCa = 1; /* application_layer/time_sync_ca */
    int m_origAddr = 0; /* application_layer/orig_addr */

    int m_timeSyncPeriod = 0; /* application_layer/time_sync_period in s*/

    bool m_giAllCa = false; /* application_layer/gi_all_ca */
    int m_giCycle = 0; /* application_layer/gi_cycle: cycle time in seconds (0 = cycle disabled)*/
    int m_giRepeatCount = 2; /* application_layer/gi_repeat_count */
    int m_giTime = 0;

    bool m_protocolConfigComplete = false; /* flag if protocol configuration is read */
    bool m_exchangeConfigComplete = false; /* flag if exchange configuration is read */

    std::string m_privateKeyFile = "";
    std::string m_clientCertFile = "";
    std::string m_serverCertFile = "";
    std::string m_caCertFile = "";
};

#endif /* IEC104_CLIENT_CONFIG_H */