/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2011 Adrian Sai-wah Tam
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef TCP_OPTION_SANTACRUZ_H
#define TCP_OPTION_SANTACRUZ_H

#include "ns3/tcp-option.h"

namespace ns3 {

/**
 * \brief to be added later
*/
class TcpOptionSantaCruz : public TcpOption
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;

  TcpOptionSantaCruz ();
  virtual ~TcpOptionSantaCruz ();

  virtual void Print (std::ostream &os) const;
  virtual void Serialize (Buffer::Iterator start) const;
  virtual uint32_t Deserialize (Buffer::Iterator start);

  virtual uint8_t GetKind (void) const;
  virtual uint32_t GetSerializedSize (void) const;

  uint16_t GetPacketIdData (void) const;
  uint16_t GetPacketIdAck (void) const;
  uint32_t GetTimestamp (void) const;

  void SetPacketIdData (uint16_t id);
  void SetPacketIdAck (uint16_t id);
  void SetTimestamp (uint32_t ts);

  static uint32_t NowToTimeStampValue ();

  //typedef std::list<uint8_t> ackList;                           //!< Ack window bytelist

protected:
  
  uint16_t m_packet_id_data;
  uint16_t m_packet_id_ack;
  uint32_t m_timestamp;
  // uint8_t m_data_copy;
  // uint8_t m_ack_copy;
  // uint32_t m_ack_sn;
  // uint8_t m_window_granularity;
  // ackList m_ack_window;
};

} // namespace ns3

#endif /* TCP_OPTION_SANTACRUZ */