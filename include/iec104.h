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
#include <utility>
#include "tls_config.h"
#include "hal_time.h"
#include "hal_thread.h"
#include "cs104_connection.h"


class IEC104Client;

class IEC104
{
public:
   typedef void (*INGEST_CB)(void*, Reading);


   IEC104(const char* ip, uint16_t port);
   ~IEC104() = default;

   void        setIp(const char* ip) { m_ip = (strlen(ip) > 1) ? ip : "127.0.0.1"; }
   void        setPort(uint16_t port) { m_port = (port > 0) ? port : IEC_60870_5_104_DEFAULT_PORT; }
   void		setAssetName(const std::string& asset) { m_asset = asset; }
   void		restart();
   void        start();
   void		stop();
   void		connect();

   void		ingest(Reading& reading);
   void		registerIngest(void* data, void (*cb)(void*, Reading));

private:
   static void connectionHandler(void* parameter, CS104_Connection connection, CS104_ConnectionEvent event);
   static bool asduReceivedHandler(void* parameter, int address, CS101_ASDU asdu);

   std::string			m_asset;
   std::string         m_ip;
   uint16_t            m_port;
   CS104_Connection    m_connection;

private:
   INGEST_CB			m_ingest;     // Callback function used to send data to south service
   void* m_data;       // Ingest function data
   bool				m_connected;
   IEC104Client* m_client;
};

class IEC104Client
{
public:
   typedef struct
   {
      std::vector<Datapoint*> datapoints;
      std::vector<QualityDescriptor> qualities;
      std::vector<std::string> timestamps;
      std::vector<bool> timestamp_qualities;
   } Data;

   explicit IEC104Client(IEC104* iec104) : m_iec104(iec104) {};

   Datapoint* createDataPoint(const std::string& dataname, const long int value)
   {
      return m_createDataPoint(dataname, value);
   }

   Datapoint* createDataPoint(const std::string& dataname, const float value)
   {
      return m_createDataPoint(dataname, value);
   }

   static void addData(Data& data, Datapoint* dp, QualityDescriptor qd, CP56Time2a ts = nullptr, bool is_ts_invalid = true)
   {
      data.datapoints.push_back(dp);
      data.qualities.push_back(qd);
      data.timestamps.push_back(CP56Time2aToString(ts));
      data.timestamp_qualities.push_back(is_ts_invalid);
   }

   void sendData(Data& data);
private:
   template <class T>
   Datapoint* m_createDataPoint(const std::string& dataname, const T value) const
   {
      DatapointValue dp_value = DatapointValue(value);

      return new Datapoint(dataname, dp_value);
   }

   /* Format 2019-01-01 10:00:00.123456+08:00 */
   static std::string CP56Time2aToString(const CP56Time2a ts)
   {
      if (ts == nullptr)
         return "";

      return std::to_string(CP56Time2a_getYear(ts) + 2000) + "-" +
         std::to_string(CP56Time2a_getMonth(ts)) + "-" +
         std::to_string(CP56Time2a_getDayOfMonth(ts)) + " " +
         std::to_string(CP56Time2a_getHour(ts)) + ":" +
         std::to_string(CP56Time2a_getMinute(ts)) + ":" +
         std::to_string(CP56Time2a_getSecond(ts)) + "." +
         millisecondsToString(CP56Time2a_getMillisecond(ts));
   }

   static std::string millisecondsToString(int ms)
   {
      if (ms < 10)
         return "00" + std::to_string(ms);
      else if (ms < 100)
         return "0" + std::to_string(ms);
      else
         return std::to_string(ms);
   }

   IEC104* m_iec104;
};

#endif
