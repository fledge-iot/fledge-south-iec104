#include "iec104_client_redgroup.h"

RedGroupCon::RedGroupCon(const string& serverIp, int tcpPort, bool conn, bool start, const string* clientIp)
{
    m_serverIp = serverIp;
    m_tcpPort = tcpPort;
    m_start = start;
    m_conn = conn;
    m_clientIp = clientIp;
}

RedGroupCon::~RedGroupCon()
{
    if (m_clientIp != nullptr) delete m_clientIp;
}

void IEC104ClientRedGroup::AddConnection(RedGroupCon* con)
{
    m_connections.push_back(con);
}
