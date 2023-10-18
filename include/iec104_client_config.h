#ifndef IEC104_CLIENT_CONFIG_H
#define IEC104_CLIENT_CONFIG_H


#include <map>
#include <vector>

class IEC104ClientRedGroup;

struct DataExchangeDefinition {
    int ca;
    int ioa;
    int typeId;
    std::string label;
    int giGroups;
};

class IEC104ClientConfig
{
public:
    IEC104ClientConfig() {m_exchangeDefinitions.clear();};
    //IEC104ClientConfig(const std::string& protocolConfig, const std::string& exchangeConfig);
    ~IEC104ClientConfig();

    int LogLevel() {return 1;};

    void importProtocolConfig(const std::string& protocolConfig);
    void importExchangeConfig(const std::string& exchangeConfig);
    void importTlsConfig(const std::string& tlsConfig);

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
    bool GiEnabled() {return m_giEnabled;};
    int GiRepeatCount() {return m_giRepeatCount;};
    int GiTime() {return m_giTime;};
    int CmdExecTimeout() {return m_cmdExecTimeout;};

    int CmdParallel() {return m_cmdParallel;};

    std::string& GetConnxStatusSignal() {return m_connxStatus;};

    std::string& GetCnxLossStatusId() {return m_cnxLossStatusId;};

    std::string& GetPrivateKey() {return m_privateKey;};
    std::string& GetOwnCertificate() {return m_ownCertificate;};
    std::vector<std::string>& GetRemoteCertificates() {return m_remoteCertificates;};
    std::vector<std::string>& GetCaCertificates() {return m_caCertificates;};

    static bool isValidIPAddress(const std::string& addrStr);

    std::vector<IEC104ClientRedGroup*>& RedundancyGroups() {return m_redundancyGroups;};

    std::map<int, std::map<int, DataExchangeDefinition*>>& ExchangeDefinition() {return m_exchangeDefinitions;};

    std::vector<int>& ListOfCAs() {return m_listOfCAs;};

    static int GetTypeIdByName(const std::string& name);

    std::string* checkExchangeDataLayer(int typeId, int ca, int ioa);

    DataExchangeDefinition* getExchangeDefinitionByLabel(std::string& label);

    DataExchangeDefinition* getCnxLossStatusDatapoint();

private:

    static bool isMessageTypeMatching(int expectedType, int rcvdType);

    void deleteExchangeDefinitions();

    std::vector<IEC104ClientRedGroup*> m_redundancyGroups = std::vector<IEC104ClientRedGroup*>();

    std::map<int, std::map<int, DataExchangeDefinition*>> m_exchangeDefinitions = std::map<int, std::map<int, DataExchangeDefinition*>>();

    std::vector<int> m_listOfCAs = std::vector<int>();

    int m_cmdParallel = 0; /* application_layer/cmd_parallel - 0 = no limit - limits the number of commands that can be executed in parallel */
    
    int m_caSize = 2;
    int m_ioaSize = 3;
    int m_asduSize = 0;

    int m_defaultCa = 1; /* application_layer/default_ca */
    int m_timeSyncCa = 1; /* application_layer/time_sync_ca */
    int m_origAddr = 0; /* application_layer/orig_addr */

    int m_timeSyncPeriod = 0; /* application_layer/time_sync_period in s*/

    bool m_giEnabled = true; /* enable GI requests by default */
    bool m_giAllCa = false; /* application_layer/gi_all_ca */
    int m_giCycle = 0; /* application_layer/gi_cycle: cycle time in seconds (0 = cycle disabled)*/
    int m_giRepeatCount = 2; /* application_layer/gi_repeat_count */
    int m_giTime = 0; /* timeout for GI execution (timeout is for each consecutive step of the GI process)*/

    int m_cmdExecTimeout = 1000; /* timeout to wait until command execution is finished (ACT-CON/ACT-TERM received)*/

    bool m_protocolConfigComplete = false; /* flag if protocol configuration is read */
    bool m_exchangeConfigComplete = false; /* flag if exchange configuration is read */

    std::string m_connxStatus = ""; /* "asset" name for south plugin monitoring event */

    bool m_sendCnxLossStatus = false; /* send info when GI is complete after connection loss */
    std::string m_cnxLossStatusId = ""; /* assed ID of the connection loss indication data point */

    std::string m_privateKey = "";
    std::string m_ownCertificate = "";
    std::vector<std::string> m_remoteCertificates;
    std::vector<std::string> m_caCertificates;
};

#endif /* IEC104_CLIENT_CONFIG_H */