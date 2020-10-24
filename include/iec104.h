#ifndef _IEC104_H
#define _IEC104_H

/*
 * Fledge south service plugin
 *
 * Released under the Apache 2.0 Licence
 *
 * Author: Estelle Chigot, Lucas Barret
 */

#include <iostream>
#include <cstring>
#include <reading.h>
#include <logger.h>
#include <cstdlib>
#include <cstdio>
#include "tls_config.h"
#include "hal_time.h"
#include "hal_thread.h"
#include "cs104_connection.h"


class IEC104Client;

class IEC104
{
	public:
		IEC104(const char *ip, uint16_t port);
		~IEC104();
        void        setIp(const char *ip);
        void        setPort(uint16_t port);
		void		setAssetName(const std::string& name);
		void		restart();
		void		start();
		void		stop();
		void		ingest(std::vector<Datapoint *>  points);
		static void transferDataint(IEC104Client *client, long int a,std::string name);
		static void transferDatafloat(IEC104Client *client, float a,std::string name);
		void		registerIngest(void *data, void (*cb)(void *, Reading))
				{
					m_ingest = cb;
					m_data = data;
				}


	private:
        static bool     asduReceivedHandler (void* parameter, int address, CS101_ASDU asdu);

		std::string			m_asset;
		std::string         m_ip;
		uint16_t            m_port;
		CS104_Connection    m_connection{};
		void				(*m_ingest)(void *, Reading){};
		void				*m_data{};
		bool				m_connected{};
		IEC104Client        *m_client;



};

class IEC104Client
{
	public : 

		explicit IEC104Client(IEC104 *iec104) : m_iec104(iec104) {};

		void sendDataint(std::string dataname,long int a)
		{

        	DatapointValue value = DatapointValue(a);

        	std::vector<Datapoint *> points;

        	std::string name = dataname;

        	points.push_back(new Datapoint(name,value));

        	m_iec104->ingest(points);

        }
        void sendDatafloat(std::string dataname,float a)
		{

        	DatapointValue value = DatapointValue(a);

        	std::vector<Datapoint *> points;

        	std::string name = dataname;

        	points.push_back(new Datapoint(name,value));

        	m_iec104->ingest(points);

        }
	private:
		IEC104    *m_iec104;
};


#endif
