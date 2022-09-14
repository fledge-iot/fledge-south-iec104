#include "iec104_client_redgroup.h"

IEC104ClientRedGroupConnection::IEC104ClientRedGroupConnection(const string& serverIp, int tcpPort, bool conn, bool start)
{
    m_serverIp = serverIp;
    m_tcpPort = tcpPort;
    m_start = start;
    m_conn = conn;
}

void IEC104ClientRedGroup::AddConnection(IEC104ClientRedGroupConnection* con)
{
    m_connections.push_back(con);
}
