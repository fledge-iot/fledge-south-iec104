/*
 * Fledge south service plugin
 *
 * Released under the Apache 2.0 Licence
 *
 * Author: Estelle Chigot, Lucas Barret
 */

#include <reading.h>
#include <logger.h>
#include <config_category.h>
#include "include/iec104.h"


using namespace std;

static long int trans;
/**
 * Constructor for the iec104 plugin
 */
IEC104::IEC104(const char *ip, uint16_t port)
{
    Logger::getLogger()->debug("Constructor");
    if (strlen(ip) > 1)
        m_ip = ip;
    else{
        m_ip = "127.0.0.1";
    }

    if (port>0){
        m_port = port;
    }else{
        m_port = IEC_60870_5_104_DEFAULT_PORT;
    }

}


/**
 * Destructor for the iec104 interface
 */
IEC104::~IEC104()=default;


void
IEC104::setIp(const char *ip){
    if (strlen(ip) > 1)
        m_ip = ip;
    else{
        m_ip = "127.0.0.1";
    }
}

void
IEC104::setPort(uint16_t port){
    if (port>0){
        m_port = port;
    }else{
        m_port = IEC_60870_5_104_DEFAULT_PORT;
    }
}

bool
IEC104::asduReceivedHandler (void* parameter, int address, CS101_ASDU asdu){
    //Logger::getLogger()->debug("asduReceivedHandler");
    if (CS101_ASDU_getTypeID(asdu) == M_ME_NB_1) {
        //Logger::getLogger()->debug("M_ME_NB_1");
        int i;

        for (i = 0; i < CS101_ASDU_getNumberOfElements(asdu); i++) {

            auto io = (MeasuredValueScaledWithCP56Time2a) CS101_ASDU_getElement(asdu, i);

            long int a = MeasuredValueScaled_getValue((MeasuredValueScaled) io);

            trans = a;

            IEC104Client * mclient = static_cast<IEC104Client*>(parameter);
            string name = "M_ME_NB_1";
            transferData(mclient, a,name);

            MeasuredValueScaledWithCP56Time2a_destroy(io);
        }
        
    }
    else if (CS101_ASDU_getTypeID(asdu) == M_SP_NA_1) {
        //Logger::getLogger()->debug("M_SP_NA_1");
        int i;

        for (i = 0; i < CS101_ASDU_getNumberOfElements(asdu); i++) {


            auto io = (SinglePointInformation) CS101_ASDU_getElement(asdu, i);
            long int a = MeasuredValueScaled_getValue((MeasuredValueScaled) io);
            IEC104Client * mclient = static_cast<IEC104Client*>(parameter);
            transferData(mclient, a,"M_SP_NA_1");
            //Logger::getLogger()->debug("Data sent");
            SinglePointInformation_destroy(io);
        }
    }

    return true;
}

/**
 * Set the asset name for the asset we write
 *
 * @param asset Set the name of the asset with insert into readings
 */
void
IEC104::setAssetName(const std::string& asset)
{
	m_asset = asset;
}

void
IEC104::transferData(IEC104Client *client, long int a, string name ) {
    client->sendData(name,a);
}
/**
 * Restart the iec104 connection
 */
void
IEC104::restart()
{
	stop();
	start();
}


/**
 * Starts the plugin
 */
void
IEC104::start()
{
    Logger::getLogger()->debug("Start iec104");
    m_client = new IEC104Client(this);
    try {
    	const char* conn_ip = m_ip.c_str();
        m_connection = CS104_Connection_create(conn_ip,m_port);
        Logger::getLogger()->debug("Connection created");
    } catch (exception &e) {
        Logger::getLogger()->error("Exception while creating connection", e.what());
        throw e;
    } catch (...) {
        Logger::getLogger()->debug("Other error");
    }

    try {
        CS104_Connection_setASDUReceivedHandler(m_connection, asduReceivedHandler, static_cast<void*>(m_client));
        Logger::getLogger()->debug("Handler added");
    } catch (exception &e) {
        Logger::getLogger()->error("Exception while adding handler", e.what());
        throw e;
    }



    if (CS104_Connection_connect(m_connection)){
        Logger::getLogger()->debug("Connection started");
        m_connected = true;
        CS104_Connection_sendStartDT(m_connection);
        CS104_Connection_sendInterrogationCommand(m_connection, CS101_COT_ACTIVATION, 1, IEC60870_QOI_STATION);
    }


}

/**
 * Disconnect from the iec104 server
 */
void
IEC104::stop()
{
	if (m_connected){
        CS104_Connection_destroy(m_connection);
	}


}

/**
 * Called when a data changed event is received. This calls back to the south service
 * and adds the points to the readings queue to send.
 *
 * @param points	The points in the reading we must create
 */
void IEC104::ingest(vector<Datapoint *>	points)
{
string asset = points[0]->getName();

	(*m_ingest)(m_data, Reading(asset, points));

}
