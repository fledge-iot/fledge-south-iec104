#include "iec104_client_config.h"

#include "rapidjson/document.h"
#include "rapidjson/error/en.h"

#include <lib60870/cs104_connection.h>

#include <arpa/inet.h>

#include <algorithm>

#define JSON_EXCHANGED_DATA "exchanged_data"
#define JSON_DATAPOINTS "datapoints"
#define JSON_PROTOCOLS "protocols"
#define JSON_LABEL "label"

#define PROTOCOL_IEC104 "iec104"
#define JSON_PROT_NAME "name"
#define JSON_PROT_ADDR "address"
#define JSON_PROT_TYPEID "typeid"

using namespace rapidjson;

IEC104ClientConfig::~IEC104ClientConfig()
{
    for (auto const &element : m_exchangeDefinitions) {
        for (auto const &elem2 : element.second) {
            delete elem2.second;
        }
    }
}

// Map of all handled ASDU types by the plugin
static map<string, int> mapAsduTypeId = {
    {"M_ME_NB_1", M_ME_NB_1},
    {"M_SP_NA_1", M_SP_NA_1},
    {"M_SP_TB_1", M_SP_TB_1},
    {"M_DP_NA_1", M_DP_NA_1},
    {"M_DP_TB_1", M_DP_TB_1},
    {"M_ST_NA_1", M_ST_NA_1},
    {"M_ST_TB_1", M_ST_TB_1},
    {"M_ME_NA_1", M_ME_NA_1},
    {"M_ME_TD_1", M_ME_TD_1},
    {"M_ME_TE_1", M_ME_TE_1},
    {"M_ME_NC_1", M_ME_NC_1},
    {"M_ME_TF_1", M_ME_TF_1},
    {"C_SC_NA_1", C_SC_NA_1},
    {"C_SC_TA_1", C_SC_TA_1},
    {"C_DC_NA_1", C_DC_NA_1},
    {"C_DC_TA_1", C_DC_TA_1},
    {"C_RC_NA_1", C_RC_NA_1},
    {"C_RC_TA_1", C_RC_TA_1},
    {"C_SE_NA_1", C_SE_NA_1},
    {"C_SE_TA_1", C_SE_TA_1},
    {"C_SE_NB_1", C_SE_NB_1},
    {"C_SE_TB_1", C_SE_TB_1},
    {"C_SE_NC_1", C_SE_NC_1},
    {"C_SE_TC_1", C_SE_TC_1}
};

int
IEC104ClientConfig::GetTypeIdByName(const string& name)
{
    int typeId = 0;

    typeId = mapAsduTypeId[name];

    return typeId;
}

bool IEC104ClientConfig::isMessageTypeMatching(int expectedType, int rcvdType)
{
    if (expectedType == rcvdType) {
        return true; /* direct match */
    }

    switch (expectedType) {

        case M_SP_NA_1:
            if ((rcvdType == M_SP_TA_1) || (rcvdType == M_SP_TB_1)) {
                return true;
            }

            break;

        case M_SP_TA_1:
            if ((rcvdType == M_SP_NA_1) || (rcvdType == M_SP_TB_1)) {
                return true;
            }

            break;

        case M_SP_TB_1:
            if ((rcvdType == M_SP_NA_1) || (rcvdType == M_SP_TA_1)) {
                return true;
            }

            break;

        case M_DP_NA_1:
            if ((rcvdType == M_DP_TA_1) || (rcvdType == M_DP_TB_1)) {
                return true;
            }

            break;

        case M_DP_TA_1:
            if ((rcvdType == M_DP_NA_1) || (rcvdType == M_DP_TB_1)) {
                return true;
            }

            break;

        case M_DP_TB_1:
            if ((rcvdType == M_DP_NA_1) || (rcvdType == M_DP_TA_1)) {
                return true;
            }

            break;

        case M_ME_NA_1:
            if ((rcvdType == M_ME_TA_1) || (rcvdType == M_ME_TD_1) || (rcvdType == M_ME_ND_1)) {
                return true;
            }

            break;

        case M_ME_TA_1:
            if ((rcvdType == M_ME_NA_1) || (rcvdType == M_ME_TD_1) || (rcvdType == M_ME_ND_1)) {
                return true;
            }

            break;

        case M_ME_TD_1:
            if ((rcvdType == M_ME_TA_1) || (rcvdType == M_ME_NA_1) || (rcvdType == M_ME_ND_1)) {
                return true;
            }

            break;

        case M_ME_ND_1:
            if ((rcvdType == M_ME_TA_1) || (rcvdType == M_ME_TD_1) || (rcvdType == M_ME_NA_1)) {
                return true;
            }

            break;

        case M_ME_NB_1:
            if ((rcvdType == M_ME_TB_1) || (rcvdType == M_ME_TE_1)) {
                return true;
            }

            break;

        case M_ME_TB_1:
            if ((rcvdType == M_ME_NB_1) || (rcvdType == M_ME_TE_1)) {
                return true;
            }

            break;

        case M_ME_TE_1:
            if ((rcvdType == M_ME_TB_1) || (rcvdType == M_ME_NB_1)) {
                return true;
            }

            break;

        case M_ME_NC_1:
            if ((rcvdType == M_ME_TC_1) || (rcvdType == M_ME_TF_1)) {
                return true;
            }

            break;

        case M_ME_TC_1:
            if ((rcvdType == M_ME_NC_1) || (rcvdType == M_ME_TF_1)) {
                return true;
            }

            break;

        case M_ME_TF_1:
            if ((rcvdType == M_ME_TC_1) || (rcvdType == M_ME_NC_1)) {
                return true;
            }

            break;


        case M_ST_NA_1:
            if ((rcvdType == M_ST_TA_1) || (rcvdType == M_ST_TB_1)) {
                return true;
            }

            break;

        case M_ST_TA_1:
            if ((rcvdType == M_ST_NA_1) || (rcvdType == M_ST_TB_1)) {
                return true;
            }

            break;

        case M_ST_TB_1:
            if ((rcvdType == M_ST_TA_1) || (rcvdType == M_ST_NA_1)) {
                return true;
            }

            break;


        default:
            //Type not supported
            break;
    }   

    return false;
}

std::string*
IEC104ClientConfig::checkExchangeDataLayer(int typeId, int ca, int ioa)
{
    auto& def = ExchangeDefinition()[ca][ioa];

    if (def != nullptr) {
        // check if message type is matching the exchange definition
        if (isMessageTypeMatching(def->typeId, typeId)) {
            return &(def->label);
        }
    }

    return nullptr;
}

bool
IEC104ClientConfig::isValidIPAddress(const string& addrStr)
{
    // see https://stackoverflow.com/questions/318236/how-do-you-validate-that-a-string-is-a-valid-ipv4-address-in-c
    struct sockaddr_in sa;
    int result = inet_pton(AF_INET, addrStr.c_str(), &(sa.sin_addr));

    return (result == 1);
}

void IEC104ClientConfig::importProtocolConfig(const string& protocolConfig)
{
    m_protocolConfigComplete = false;

    Document document;

    if (document.Parse(const_cast<char*>(protocolConfig.c_str())).HasParseError()) {
        Logger::getLogger()->fatal("Parsing error in protocol configuration");

        printf("Parsing error in protocol configuration\n");

        return;
    }

    if (!document.IsObject()) {
        return;
    }

    if (!document.HasMember("protocol_stack") || !document["protocol_stack"].IsObject()) {
        return;
    }

    const Value& protocolStack = document["protocol_stack"];

    if (!protocolStack.HasMember("transport_layer") || !protocolStack["transport_layer"].IsObject()) {
        Logger::getLogger()->fatal("transport layer configuration is missing");
    
        return;
    }

    if (!protocolStack.HasMember("application_layer") || !protocolStack["application_layer"].IsObject()) {
        Logger::getLogger()->fatal("appplication layer configuration is missing");
    
        return;
    }

    const Value& transportLayer = protocolStack["transport_layer"];
    const Value& applicationLayer = protocolStack["application_layer"];

    if (transportLayer.HasMember("redundancy_groups")) {

        if (transportLayer["redundancy_groups"].IsArray()) {

            const Value& redundancyGroups = transportLayer["redundancy_groups"];

            for (const Value& redGroup : redundancyGroups.GetArray()) {
                
                char* redGroupName = NULL;

                if (redGroup.HasMember("rg_name")) {
                    if (redGroup["rg_name"].IsString()) {
                        string rgName = redGroup["rg_name"].GetString();

                        redGroupName = strdup(rgName.c_str());
                    }
                }

                IEC104ClientRedGroup* redundancyGroup = new IEC104ClientRedGroup();

                printf("Adding red group with name: %s\n", redGroupName);

                free(redGroupName);

                if (redGroup.HasMember("connections")) {
                    if (redGroup["connections"].IsArray()) {
                        for (const Value& con : redGroup["connections"].GetArray()) {
                            if (con.HasMember("srv_ip")) {
                                if (con["srv_ip"].IsString()) {
                                    string srvIp = con["srv_ip"].GetString();

                                    if (isValidIPAddress(srvIp)) {

                                        printf("  add to group: %s\n", srvIp.c_str());

                                        int tcpPort = 2404;
                                        bool start = true;
                                        bool conn = true;
                                       
                                        string* clientIp = nullptr;

                                        if (con.HasMember("clt_ip")) {
                                            if (con["clt_ip"].IsString()) {
                                                string cltIpStr = con["clt_ip"].GetString();

                                                if (isValidIPAddress(cltIpStr)) {
                                                    clientIp = new string(cltIpStr);
                                                }
                                                else {
                                                    printf("clt_ip %s is not a valid IP address -> ignore\n", srvIp.c_str());
                                                    Logger::getLogger()->error("clt_ip %s is not a valid IP address -> ignore", srvIp.c_str());
                                                }
                                            }
                                        }

                                        if (con.HasMember("port")) {
                                            if (con["port"].IsInt()) {
                                                int tcpPortVal = con["port"].GetInt();

                                                if (tcpPortVal > 0 && tcpPortVal < 65636) {
                                                    tcpPort = tcpPortVal;
                                                }
                                                else {
                                                    Logger::getLogger()->error("%s has no valied TCP port defined -> using default port(2404)", srvIp.c_str());
                                                }
                                            }
                                            else {
                                                printf("transport_layer.port has invalid type -> using default port\n");
                                                Logger::getLogger()->warn("transport_layer.port has invalid type -> using default port");
                                            }
                                        }

                                        if (con.HasMember("conn")) {
                                            if (con["conn"].IsBool()) {
                                                conn = con["conn"].GetBool();
                                            }
                                        }

                                        if (con.HasMember("start")) {
                                            if (con["start"].IsBool()) {
                                                start = con["start"].GetBool();
                                            }
                                        }

                                        RedGroupCon* connection = new RedGroupCon(srvIp, tcpPort, conn, start, clientIp);

                                        redundancyGroup->AddConnection(connection);

                                    }
                                    else {
                                        printf("srv_ip %s is not a valid IP address -> ignore\n", srvIp.c_str());
                                        Logger::getLogger()->error("srv_ip %s is not a valid IP address -> ignore", srvIp.c_str());
                                    }

                                }
                            }
                        }
                    }
                }

                if (redGroup.HasMember("k_value")) {
                    if (redGroup["k_value"].IsInt()) {
                        int kValue = redGroup["k_value"].GetInt();

                        if (kValue > 0 && kValue < 32768) {
                            redundancyGroup->K(kValue);
                        }
                        else {
                            Logger::getLogger()->warn("redGroup.k_value value out of range-> using default value");
                        }
                    }
                    else {
                        printf("redGroup.k_value has invalid type -> using default value\n");
                        Logger::getLogger()->warn("redGroup.k_value has invalid type -> using default value");
                    }
                }

                if (redGroup.HasMember("w_value")) {
                    if (redGroup["w_value"].IsInt()) {
                        int wValue = redGroup["w_value"].GetInt();

                        if (wValue > 0 && wValue < 32768) {
                            redundancyGroup->W(wValue);
                        }
                        else {
                            Logger::getLogger()->warn("redGroup.w_value value out of range-> using default value");
                        }
                    }
                    else {
                        printf("redGroup.w_value has invalid type -> using default value\n");
                        Logger::getLogger()->warn("redGroup.w_value has invalid type -> using default value");
                    }
                }

                if (redGroup.HasMember("t0_timeout")) {
                    if (redGroup["t0_timeout"].IsInt()) {
                        int t0Timeout = redGroup["t0_timeout"].GetInt();

                        if (t0Timeout > 0 && t0Timeout < 256) {
                            redundancyGroup->T0(t0Timeout);
                        }
                        else {
                            Logger::getLogger()->warn("redGroup.t0_timeout value out of range-> using default value");
                        }
                    }
                    else {
                        printf("redGroup.t0_timeout has invalid type -> using default value\n");
                        Logger::getLogger()->warn("redGroup.t0_timeout has invalid type -> using default value");
                    }
                }

                if (redGroup.HasMember("t1_timeout")) {
                    if (redGroup["t1_timeout"].IsInt()) {
                        int t1Timeout = redGroup["t1_timeout"].GetInt();

                        if (t1Timeout > 0 && t1Timeout < 256) {
                            redundancyGroup->T1(t1Timeout);
                        }
                        else {
                            Logger::getLogger()->warn("redGroup.t1_timeout value out of range-> using default value");
                        }
                    }
                    else {
                        printf("redGroup.t1_timeout has invalid type -> using default value\n");
                        Logger::getLogger()->warn("redGroup.t1_timeout has invalid type -> using default value");
                    }
                }

                if (redGroup.HasMember("t2_timeout")) {
                    if (redGroup["t2_timeout"].IsInt()) {
                        int t2Timeout = redGroup["t2_timeout"].GetInt();

                        if (t2Timeout > 0 && t2Timeout < 256) {
                            redundancyGroup->T2(t2Timeout);
                        }
                        else {
                            Logger::getLogger()->warn("redGroup.t2_timeout value out of range-> using default value");
                        }
                    }
                    else {
                        printf("redGroup.t2_timeout has invalid type -> using default value\n");
                        Logger::getLogger()->warn("redGroup.t2_timeout has invalid type -> using default value");
                    }
                }

                if (redGroup.HasMember("t3_timeout")) {
                    if (redGroup["t3_timeout"].IsInt()) {
                        int t3Timeout = redGroup["t3_timeout"].GetInt();

                        if (t3Timeout > -1) {
                            redundancyGroup->T3(t3Timeout);
                        }
                        else {
                            Logger::getLogger()->warn("redGroup.t3_timeout value out of range-> using default value");
                        }
                    }
                    else {
                        printf("redGroup.t3_timeout has invalid type -> using default value\n");
                        Logger::getLogger()->warn("redGroup.t3_timeout has invalid type -> using default value");
                    }
                }

                if (redGroup.HasMember("tls")) {
                    if (redGroup["tls"].IsBool()) {
                        redundancyGroup->UseTLS(redGroup["tls"].GetBool());
                    }
                    else {
                        printf("redGroup.tls has invalid type -> not using TLS\n");
                        Logger::getLogger()->warn("redGroup.tls has invalid type -> not using TLS");
                    }
                }

                m_redundancyGroups.push_back(redundancyGroup);
            }
        }
        else {
            Logger::getLogger()->fatal("redundancy_groups is not an array -> ignore redundancy groups");
        }
    }

    /* Application layer parameters */

    if (applicationLayer.HasMember("orig_addr")) {
        if (applicationLayer["orig_addr"].IsInt()) {
            int origAddr = applicationLayer["orig_addr"].GetInt();

            if (origAddr >= 0 && origAddr <= 255) {
                m_origAddr = origAddr;
            }
            else {
                printf("application_layer.orig_addr has invalid value -> using default value (0)\n");
                Logger::getLogger()->warn("application_layer.orig_addr has invalid value -> using default value (0)");
            }
        }
        else {
            printf("application_layer.orig_addr has invalid type -> using default value (0)\n");
            Logger::getLogger()->warn("application_layer.orig_addr has invalid type -> using default value (0)");
        }
    }

    if (applicationLayer.HasMember("ca_asdu_size")) {
        if (applicationLayer["ca_asdu_size"].IsInt()) {
            int caSize = applicationLayer["ca_asdu_size"].GetInt();

            if (caSize > 0 && caSize < 3) {
                m_caSize = caSize;
            }
            else {
                printf("application_layer.ca_asdu_size has invalid value -> using default value (2)\n");
                Logger::getLogger()->warn("application_layer.ca_asdu_size has invalid value -> using default value (2");
            }
        }
        else {
            printf("application_layer.ca_asdu_size has invalid type -> using default value (2)\n");
            Logger::getLogger()->warn("application_layer.ca_asdu_size has invalid type -> using default value (2)");
        }
    }

    if (applicationLayer.HasMember("ioaddr_size")) {
        if (applicationLayer["ioaddr_size"].IsInt()) {
            int ioaSize = applicationLayer["ioaddr_size"].GetInt();

            if (ioaSize > 0 && ioaSize < 4) {
                m_ioaSize = ioaSize;
            }
            else {
                printf("application_layer.ioaddr_size has invalid value -> using default value (3)\n");
                Logger::getLogger()->warn("application_layer.ioaddr_size has invalid value -> using default value (3)");
            }
        }
        else {
            printf("application_layer.ioaddr_size has invalid type -> using default value (3)\n");
            Logger::getLogger()->warn("application_layer.ioaddr_size has invalid type -> using default value (3)");
        }
    }

    if (applicationLayer.HasMember("asdu_size")) {
        if (applicationLayer["asdu_size"].IsInt()) {
            int asduSize = applicationLayer["asdu_size"].GetInt();

            if (asduSize == 0 || (asduSize > 10 && asduSize < 254)) {
                m_asduSize = asduSize;
            }
            else {
                printf("application_layer.asdu_size has invalid value -> using default value (3)\n");
                Logger::getLogger()->warn("application_layer.asdu_size has invalid value -> using default value (3)");
            }
        }
        else {
            printf("application_layer.asdu_size has invalid type -> using default value (3)\n");
            Logger::getLogger()->warn("application_layer.asdu_size has invalid type -> using default value (3)");
        }
    }

    m_timeSyncEnabled = false;

    if (applicationLayer.HasMember("time_sync")) {
        if (applicationLayer["time_sync"].IsInt()) {
            int timeSyncValue = applicationLayer["time_sync"].GetInt();

            if (timeSyncValue < 0) {
                printf("application_layer.time_sync has invalid value -> using default value (0)\n");
                Logger::getLogger()->warn("application_layer.time_sync has invalid value -> using default value (0)");
            }
            else if (timeSyncValue > 0) {
                m_timeSyncEnabled = true;
                m_timeSyncPeriod = timeSyncValue;
            }

        }
        else {
            printf("application_layer.time_sync has invalid type -> using default value (0)\n");
            Logger::getLogger()->warn("application_layer.time_sync has invalid type -> using default value (0)");
        }
    }

    if (applicationLayer.HasMember("gi_all_ca")) {
        if (applicationLayer["gi_all_ca"].IsBool()) {
            m_giAllCa = applicationLayer["gi_all_ca"].GetBool();
        }
        else {
            printf("applicationLayer.gi_all_ca has invalid type -> use broadcast CA\n");
            Logger::getLogger()->warn("applicationLayer.gi_all_ca has invalid type -> use broadcast CA");
        }
    }

    if (applicationLayer.HasMember("gi_time")) {
        if (applicationLayer["gi_time"].IsInt()) {
            int giTime = applicationLayer["gi_time"].GetInt();

            if (giTime >= 0) {
                m_giTime = giTime;
            }
            else {
                printf("application_layer.gi_time has invalid value -> using default value (0)\n");
                Logger::getLogger()->warn("application_layer.gi_time has invalid value -> using default value (0)");
            }
        }
        else {
            printf("application_layer.gi_time has invalid type -> using default value (0)\n");
            Logger::getLogger()->warn("application_layer.gi_time has invalid type -> using default value (0)");
        }
    }

    if (applicationLayer.HasMember("gi_cycle")) {
        if (applicationLayer["gi_cycle"].IsInt()) {
            int giCycle = applicationLayer["gi_cycle"].GetInt();

            if (giCycle >= 0) {
                m_giCycle = giCycle;
            }
            else {
                printf("application_layer.gi_cycle has invalid value -> using default value (0)\n");
                Logger::getLogger()->warn("application_layer.gi_cycle has invalid value -> using default value (0)");
            }
        }
        else {
            printf("application_layer.gi_cycle has invalid type -> using default value (0)\n");
            Logger::getLogger()->warn("application_layer.gi_cycle has invalid type -> using default value (0)");
        }
    }

    if (applicationLayer.HasMember("gi_repeat_count")) {
        if (applicationLayer["gi_repeat_count"].IsInt()) {
            int giRepeatCount = applicationLayer["gi_repeat_count"].GetInt();

            if (giRepeatCount >= 0) {
                m_giRepeatCount = giRepeatCount;
            }
            else {
                printf("application_layer.gi_repeat_count has invalid value -> using default value (2)\n");
                Logger::getLogger()->warn("application_layer.gi_repeat_count has invalid value -> using default value (2)");
            }
        }
        else {
            printf("application_layer.gi_repeat_count has invalid type -> using default value (2)\n");
            Logger::getLogger()->warn("application_layer.gi_repeat_count has invalid type -> using default value (2)");
        }
    }

    m_protocolConfigComplete = true;
}

void IEC104ClientConfig::deleteExchangeDefinitions()
{
    for (auto const& exchangeDefintions : m_exchangeDefinitions) {
        for (auto const& dpPair : exchangeDefintions.second) {
            DataExchangeDefinition* dp = dpPair.second;

            delete dp;
        }
    }

    m_exchangeDefinitions.clear();
}

void IEC104ClientConfig::importExchangeConfig(const string& exchangeConfig)
{
    m_exchangeConfigComplete = false;

    deleteExchangeDefinitions();

    Document document;

    if (document.Parse(const_cast<char*>(exchangeConfig.c_str())).HasParseError()) {
        Logger::getLogger()->fatal("Parsing error in data exchange configuration");

        printf("Parsing error in data exchange configuration\n");

        return;
    }

    if (!document.IsObject())
        return;

    if (!document.HasMember(JSON_EXCHANGED_DATA) || !document[JSON_EXCHANGED_DATA].IsObject()) {
        return;
    }

    const Value& exchangeData = document[JSON_EXCHANGED_DATA];

    if (!exchangeData.HasMember(JSON_DATAPOINTS) || !exchangeData[JSON_DATAPOINTS].IsArray()) {

        return;
    }

    const Value& datapoints = exchangeData[JSON_DATAPOINTS];

    for (const Value& datapoint : datapoints.GetArray()) {

        if (!datapoint.IsObject()) return;

        if (!datapoint.HasMember(JSON_LABEL) || !datapoint[JSON_LABEL].IsString()) return;

        string label = datapoint[JSON_LABEL].GetString();

        if (!datapoint.HasMember(JSON_PROTOCOLS) || !datapoint[JSON_PROTOCOLS].IsArray()) return;

        for (const Value& protocol : datapoint[JSON_PROTOCOLS].GetArray()) {
            
            if (!protocol.HasMember(JSON_PROT_NAME) || !protocol[JSON_PROT_NAME].IsString()) return;
            
            string protocolName = protocol[JSON_PROT_NAME].GetString();

            if (protocolName == PROTOCOL_IEC104) {

                if (!protocol.HasMember(JSON_PROT_ADDR) || !protocol[JSON_PROT_ADDR].IsString()) return;
                if (!protocol.HasMember(JSON_PROT_TYPEID) || !protocol[JSON_PROT_TYPEID].IsString()) return;

                string address = protocol[JSON_PROT_ADDR].GetString();
                string typeIdStr = protocol[JSON_PROT_TYPEID].GetString();

                printf("  address: %s type: %s\n", address.c_str(), typeIdStr.c_str());

                size_t sepPos = address.find("-");

                if (sepPos != std::string::npos) {
                    std::string caStr = address.substr(0, sepPos);
                    std::string ioaStr = address.substr(sepPos + 1);

                    int ca = std::stoi(caStr);
                    int ioa = std::stoi(ioaStr);

                    printf("    CA: %i IOA: %i\n", ca, ioa);

                    DataExchangeDefinition* def = new DataExchangeDefinition;

                    if (def) {
                        def->ca = ca;
                        def->ioa = ioa;
                        def->label = label;
                        def->typeId = 0;
                        def->typeId = IEC104ClientConfig::GetTypeIdByName(typeIdStr);
                        ExchangeDefinition()[ca][ioa] = def;
                    }
                }
            }
        }
    }

    for (auto& element : ExchangeDefinition()) {
        int ca = element.first;

        if (std::find(m_listOfCAs.begin(), m_listOfCAs.end(), ca) == m_listOfCAs.end()) {
            m_listOfCAs.push_back(ca);
        }
    }

    m_exchangeConfigComplete = true;
}
