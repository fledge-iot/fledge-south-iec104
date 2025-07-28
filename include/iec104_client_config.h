#ifndef IEC104_CLIENT_CONFIG_H
#define IEC104_CLIENT_CONFIG_H


#include <map>
#include <vector>
#include <memory>
#include <utility>
#include <unordered_set>
#include <unordered_map>

#include <rapidjson/document.h>

class IEC104ClientRedGroup;

struct DataExchangeDefinition {
    int ca = 0;
    int ioa = 0;
    int typeId = 0;
    std::string label;
    int giGroups = 0;
};

// Define a custom hash function for std::pair<T, U>
template <typename T, typename U>
struct pairHash {
    size_t operator()(const std::pair<T, U>& p) const {
        size_t h1 = std::hash<T>{}(p.first);
        size_t h2 = std::hash<U>{}(p.second);
        return h1 ^ (h2 << 1); // Combine the two hash values
    }
};

class IEC104ClientConfig
{
public:
    IEC104ClientConfig() = default;
    //IEC104ClientConfig(const std::string& protocolConfig, const std::string& exchangeConfig);
    ~IEC104ClientConfig() = default;

    int LogLevel() {return 1;};

    void importProtocolConfig(const std::string& protocolConfig);
    void importExchangeConfig(const std::string& exchangeConfig);
    void importTlsConfig(const std::string& tlsConfig);

    void importRedGroup(const rapidjson::Value& redGroup);
    void importRedGroupCon(const rapidjson::Value& con, std::shared_ptr<IEC104ClientRedGroup> redundancyGroup) const;

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

    std::string& GetPrivateKey() {return m_privateKey;};
    std::string& GetOwnCertificate() {return m_ownCertificate;};
    std::vector<std::string>& GetRemoteCertificates() {return m_remoteCertificates;};
    std::vector<std::string>& GetCaCertificates() {return m_caCertificates;};

    static bool isValidIPAddress(const std::string& addrStr);

    std::vector<std::shared_ptr<IEC104ClientRedGroup>>& RedundancyGroups() {return m_redundancyGroups;};

    std::map<int, std::map<int, std::shared_ptr<DataExchangeDefinition>>>& ExchangeDefinition() {return m_exchangeDefinitions;};

    /**
     * Check if a CA / IOA pair is in the CG triggering TS address set
     *
     * @return True if the given pair is present in the address set
     */
    int valueTsAddressCgTriggering(int ca, int ioa) {
        auto it = m_cgTriggeringTsAdresses.find(std::make_pair(ca, ioa));
        if (it == m_cgTriggeringTsAdresses.end()){
            return -1;
        }
        return it->second;
    }

    std::vector<int>& ListOfCAs() {return m_listOfCAs;};

    static int getTypeIdFromString(const std::string& name);
    static std::string getStringFromTypeID(int typeId);

    std::string* checkExchangeDataLayer(int typeId, int ca, int ioa);

    std::shared_ptr<DataExchangeDefinition> getExchangeDefinitionByLabel(std::string& label);

    int GetMaxRedGroups() const {return m_max_red_groups;};

    bool isConfigComplete() const {return m_protocolConfigComplete && m_exchangeConfigComplete && m_tlsConfigComplete;};

private:

    static bool isMessageTypeMatching(int expectedType, int rcvdType);

    void deleteExchangeDefinitions();

    std::vector<std::shared_ptr<IEC104ClientRedGroup>> m_redundancyGroups;
    int m_max_red_groups = 2;

    std::map<int, std::map<int, std::shared_ptr<DataExchangeDefinition>>> m_exchangeDefinitions;

    /* Set of TS addresses that triggers a CG with its trigger value. First member of the pair is ca, second is ioa. The third is the trigger value */
    std::unordered_map<std::pair<int, int>, int, pairHash<int, int>> m_cgTriggeringTsAdresses;

    std::vector<int> m_listOfCAs;

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
    bool m_tlsConfigComplete = false; /* flag if tls configuration is read */

    std::string m_connxStatus = ""; /* "asset" name for south plugin monitoring event */

    std::string m_privateKey = "";
    std::string m_ownCertificate = "";
    std::vector<std::string> m_remoteCertificates;
    std::vector<std::string> m_caCertificates;
};

#endif /* IEC104_CLIENT_CONFIG_H */