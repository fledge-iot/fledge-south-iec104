#ifndef IEC104_CLIENT_CONFIG_H
#define IEC104_CLIENT_CONFIG_H


#include "logger.h"
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

    int CaSize() {return m_caSize;};
    int IOASize() {return m_ioaSize;};
    int AsduSize() {return m_asduSize;};

    int DefaultCa() {return m_defaultCa;};
    int TimeSyncCa() {return m_timeSyncCa;};
    int OrigAddr() {return m_origAddr;};

    bool isTimeSyncEnabled() {return m_timeSyncEnabled;};
    int TimeSyncPeriod() {return m_timeSyncPeriod;};

    bool GiForAllCa() {return m_giAllCa;};
    bool GiCycle() {return m_giCycle;};
    int GiRepeatCount() {return m_giRepeatCount;};
    int GiTime() {return m_giTime;};

    static bool isValidIPAddress(const string& addrStr);

    std::vector<IEC104ClientRedGroup*>& RedundancyGroups() {return m_redundancyGroups;};

    std::map<int, std::map<int, DataExchangeDefinition*>>& ExchangeDefinition() {return m_exchangeDefinitions;};

    static int GetTypeIdByName(const string& name);

private:

    void deleteExchangeDefinitions();

    std::vector<IEC104ClientRedGroup*> m_redundancyGroups = std::vector<IEC104ClientRedGroup*>();

    std::map<int, std::map<int, DataExchangeDefinition*>> m_exchangeDefinitions = std::map<int, std::map<int, DataExchangeDefinition*>>();

    int m_caSize = 2;
    int m_ioaSize = 3;
    int m_asduSize = 0;

    int m_defaultCa = 1; /* application_layer/default_ca */
    int m_timeSyncCa = 1; /* application_layer/time_sync_ca */
    int m_origAddr = 0; /* application_layer/orig_addr */

    bool m_timeSyncEnabled = false; /* application_layer/time_sync */
    int m_timeSyncPeriod = 0; /* application_layer/time_sync_period in s*/

    bool m_giAllCa = false; /* application_layer/gi_all_ca */
    bool m_giCycle = false; /* application_layer/gi_cycle */
    int m_giRepeatCount = 2; /* application_layer/gi_repeat_count */
    int m_giTime = 0;

    bool m_protocolConfigComplete = false; /* flag if protocol configuration is read */
    bool m_exchangeConfigComplete = false; /* flag if exchange configuration is read */

};

#endif /* IEC104_CLIENT_CONFIG_H */