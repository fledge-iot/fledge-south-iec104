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
    int i;
    IEC104Client* mclient;
    switch (CS101_ASDU_getTypeID(asdu)) {
        case M_ME_NB_1:
            for (i = 0; i < CS101_ASDU_getNumberOfElements(asdu); i++) {
                auto io = (MeasuredValueScaledWithCP56Time2a) CS101_ASDU_getElement(asdu, i);
                long int a = MeasuredValueScaled_getValue((MeasuredValueScaled) io);
                trans = a;
                mclient = static_cast<IEC104Client*>(parameter);
                string name = "M_ME_NB_1";
                transferDataint(mclient, a,name);

                MeasuredValueScaledWithCP56Time2a_destroy(io);
            }
            break;
        case M_SP_NA_1:
            for (i = 0; i < CS101_ASDU_getNumberOfElements(asdu); i++) {
                auto io = (SinglePointInformation) CS101_ASDU_getElement(asdu, i);
                long int a = SinglePointInformation_getValue((SinglePointInformation) io);
                mclient = static_cast<IEC104Client*>(parameter);
                transferDataint(mclient, a,"M_SP_NA_1");

                SinglePointInformation_destroy(io);
            }
            break;
        case M_SP_TB_1:
            for (i = 0; i < CS101_ASDU_getNumberOfElements(asdu); i++) {
                auto io = (SinglePointWithCP56Time2a) CS101_ASDU_getElement(asdu, i);
                long int a = SinglePointInformation_getValue((SinglePointInformation) io);
                mclient = static_cast<IEC104Client*>(parameter);
                transferDataint(mclient, a,"M_SP_TB_1");

                SinglePointWithCP56Time2a_destroy(io);
            }
            break;
        case M_DP_NA_1:
            for (i = 0; i < CS101_ASDU_getNumberOfElements(asdu); i++) {
                auto io = (DoublePointInformation) CS101_ASDU_getElement(asdu, i);
                long int a = DoublePointInformation_getValue((DoublePointInformation) io);
                mclient = static_cast<IEC104Client*>(parameter);
                transferDataint(mclient, a,"M_DP_NA_1");

                DoublePointInformation_destroy(io);
            }
            break;
        case M_DP_TB_1:
            for (i = 0; i < CS101_ASDU_getNumberOfElements(asdu); i++) {
                auto io = (DoublePointWithCP56Time2a) CS101_ASDU_getElement(asdu, i);
                long int a = DoublePointInformation_getValue((DoublePointInformation) io);
                mclient = static_cast<IEC104Client*>(parameter);
                transferDataint(mclient, a,"M_DP_TA_1");

                DoublePointWithCP56Time2a_destroy(io);
            }
            break;
        case M_ST_NA_1:
            for (i = 0; i < CS101_ASDU_getNumberOfElements(asdu); i++) {
                auto io = (StepPositionInformation) CS101_ASDU_getElement(asdu, i);
                long int a = StepPositionInformation_getValue((StepPositionInformation) io);
                mclient = static_cast<IEC104Client*>(parameter);
                transferDataint(mclient, a,"M_ST_NA_1");

                StepPositionInformation_destroy(io);
            }
            break;
        case M_ST_TB_1:
            for (i = 0; i < CS101_ASDU_getNumberOfElements(asdu); i++) {
                auto io = (StepPositionWithCP56Time2a) CS101_ASDU_getElement(asdu, i);
                long int a = StepPositionInformation_getValue((StepPositionInformation) io);
                mclient = static_cast<IEC104Client*>(parameter);
                transferDataint(mclient, a,"M_ST_TB_1");

                StepPositionWithCP56Time2a_destroy(io);
            }
            break;
        case M_ME_NA_1:
            for (i = 0; i < CS101_ASDU_getNumberOfElements(asdu); i++) {
                auto io = (MeasuredValueNormalized) CS101_ASDU_getElement(asdu, i);
                float a = MeasuredValueNormalized_getValue((MeasuredValueNormalized) io);
                mclient = static_cast<IEC104Client*>(parameter);
                transferDatafloat(mclient, a,"M_ME_NA_1");

                MeasuredValueNormalized_destroy(io);
            }
            break;
        case M_ME_TD_1:
            for (i = 0; i < CS101_ASDU_getNumberOfElements(asdu); i++) {
                auto io = (MeasuredValueNormalizedWithCP56Time2a) CS101_ASDU_getElement(asdu, i);
                float a = MeasuredValueNormalized_getValue((MeasuredValueNormalized) io);
                mclient = static_cast<IEC104Client*>(parameter);
                transferDatafloat(mclient, a,"M_ME_TD_1");

                MeasuredValueNormalizedWithCP56Time2a_destroy(io);
            }
            break;
        case M_ME_TE_1:
            for (i = 0; i < CS101_ASDU_getNumberOfElements(asdu); i++) {
                auto io = (MeasuredValueScaledWithCP56Time2a) CS101_ASDU_getElement(asdu, i);
                float a = MeasuredValueScaled_getValue((MeasuredValueScaled) io);
                mclient = static_cast<IEC104Client*>(parameter);
                transferDatafloat(mclient, a,"M_ME_TE_1");

                MeasuredValueScaledWithCP56Time2a_destroy(io);
            }
            break;
        case M_ME_NC_1:
            for (i = 0; i < CS101_ASDU_getNumberOfElements(asdu); i++) {
                auto io = (MeasuredValueShort) CS101_ASDU_getElement(asdu, i);
                float a = MeasuredValueShort_getValue((MeasuredValueShort) io);
                mclient = static_cast<IEC104Client*>(parameter);
                transferDatafloat(mclient, a,"M_ME_NC_1");

                MeasuredValueShort_destroy(io);
            }
            break;
        case M_ME_TF_1:
            for (i = 0; i < CS101_ASDU_getNumberOfElements(asdu); i++) {
                auto io = (MeasuredValueShortWithCP56Time2a) CS101_ASDU_getElement(asdu, i);
                float a = MeasuredValueShort_getValue((MeasuredValueShort) io);
                mclient = static_cast<IEC104Client*>(parameter);
                transferDatafloat(mclient, a,"M_ME_TF_1");

                MeasuredValueShortWithCP56Time2a_destroy(io);
            }
            break;
        case C_SC_TA_1:
            for (i = 0; i < CS101_ASDU_getNumberOfElements(asdu); i++) {
                auto io = (SingleCommandWithCP56Time2a) CS101_ASDU_getElement(asdu, i);
                long int a = SingleCommand_getState((SingleCommand) io);
                mclient = static_cast<IEC104Client*>(parameter);
                transferDataint(mclient, a,"C_SC_TA_1");

                SingleCommandWithCP56Time2a_destroy(io);
            }
            break;
        case C_DC_TA_1:
            for (i = 0; i < CS101_ASDU_getNumberOfElements(asdu); i++) {
                auto io = (DoubleCommandWithCP56Time2a) CS101_ASDU_getElement(asdu, i);
                long int a = DoubleCommand_getState((DoubleCommand) io);
                mclient = static_cast<IEC104Client*>(parameter);
                transferDataint(mclient, a,"C_DC_TA_1");

                DoubleCommandWithCP56Time2a_destroy(io);
            }
            break;
        case C_SE_TB_1:
            for (i = 0; i < CS101_ASDU_getNumberOfElements(asdu); i++) {
                auto io = (SetpointCommandScaledWithCP56Time2a) CS101_ASDU_getElement(asdu, i);
                long int a = SetpointCommandScaled_getValue((SetpointCommandScaled) io);
                mclient = static_cast<IEC104Client*>(parameter);
                transferDataint(mclient, a,"C_SC_TA_1");

                SetpointCommandScaledWithCP56Time2a_destroy(io);
            }
            break;
        case C_SE_TC_1:
            for (i = 0; i < CS101_ASDU_getNumberOfElements(asdu); i++) {
                auto io = (SetpointCommandShortWithCP56Time2a) CS101_ASDU_getElement(asdu, i);
                float a = SetpointCommandShort_getValue((SetpointCommandShort) io);
                mclient = static_cast<IEC104Client*>(parameter);
                transferDatafloat(mclient, a,"C_SC_TC_1");

                SetpointCommandShortWithCP56Time2a_destroy(io);
            }
            break;
        case C_SE_NC_1:
            for (i = 0; i < CS101_ASDU_getNumberOfElements(asdu); i++) {
                auto io = (SetpointCommandShort) CS101_ASDU_getElement(asdu, i);
                float a = SetpointCommandShort_getValue((SetpointCommandShort) io);
                mclient = static_cast<IEC104Client*>(parameter);
                transferDatafloat(mclient, a,"C_SE_NC_1");

                SetpointCommandShort_destroy(io);
            }
            break;
        case C_SE_NB_1:
            for (i = 0; i < CS101_ASDU_getNumberOfElements(asdu); i++) {
                auto io = (SetpointCommandScaled) CS101_ASDU_getElement(asdu, i);
                long int a = SetpointCommandScaled_getValue((SetpointCommandScaled) io);
                mclient = static_cast<IEC104Client*>(parameter);
                transferDataint(mclient, a,"C_SC_TA_1");

                SetpointCommandScaled_destroy(io);
            }
            break;
        default:
            Logger::getLogger()->error("Type of message not supported");
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
IEC104::transferDataint(IEC104Client *client, long int a, string name ) {
    client->sendDataint(name,a);
}

void
IEC104::transferDatafloat(IEC104Client *client, float a, string name ) {
    client->sendDatafloat(name,a);
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
