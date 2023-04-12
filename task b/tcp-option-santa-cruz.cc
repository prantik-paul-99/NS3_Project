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

#include "tcp-option-santa-cruz.h"
#include "tcp-option.h"
#include "ns3/timer.h"
#include "ns3/log.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("TcpOptionSantaCruz");

NS_OBJECT_ENSURE_REGISTERED (TcpOptionSantaCruz);

TcpOptionSantaCruz::TcpOptionSantaCruz ()
  : TcpOption (),
  m_packet_id_data(0),
  m_packet_id_ack(0),
  m_timestamp(0)
  // m_data_copy(0),
  // m_ack_copy(0),
  // m_ack_sn(0),
  // m_window_granularity(0)
{
}

TcpOptionSantaCruz::~TcpOptionSantaCruz ()
{
}

TypeId
TcpOptionSantaCruz::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::TcpOptionSantaCruz")
    .SetParent<TcpOption> ()
    .SetGroupName ("Internet")
    .AddConstructor<TcpOptionSantaCruz> ()
  ;
  return tid;
}

TypeId
TcpOptionSantaCruz::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

void
TcpOptionSantaCruz::Print (std::ostream &os) const
{
  os << m_packet_id_data<<";"<<m_packet_id_ack << ";" << m_timestamp;
}

uint32_t
TcpOptionSantaCruz::GetSerializedSize (void) const
{
  NS_LOG_FUNCTION (this);
  uint32_t size = 0;
  size+=2; //enum and length
  size+=2; //packet_id_data
  size+=2; //packet_id_ack
  size+=4; //timestamp of receiving
  return size;
}

void
TcpOptionSantaCruz::Serialize (Buffer::Iterator start) const
{
  NS_LOG_FUNCTION (this);
  Buffer::Iterator i = start;
  i.WriteU8 (GetKind ()); // Kind
  i.WriteU8 (10); // Length
  i.WriteHtonU16 (m_packet_id_data); // packet id_data
  i.WriteHtonU16 (m_packet_id_ack); // packet id_ack
  i.WriteHtonU32 (m_timestamp); // time of sending ack
}

uint32_t
TcpOptionSantaCruz::Deserialize (Buffer::Iterator start)
{
  NS_LOG_FUNCTION (this);
  uint32_t m_size = GetSerializedSize ();

  Buffer::Iterator i = start;

  uint8_t readKind = i.ReadU8 ();
  if (readKind != GetKind ())
    {
      NS_LOG_WARN ("Malformed Santa Cruz option");
      return 0;
    }

  uint8_t size = i.ReadU8 ();
  if (size != m_size)
    {
      NS_LOG_WARN ("Malformed Santa Cruz option");
      return 0;
    }
  m_packet_id_data = i.ReadNtohU16 ();
  m_packet_id_ack = i.ReadNtohU16 ();
  m_timestamp = i.ReadNtohU32 ();

  return GetSerializedSize ();
}

uint8_t
TcpOptionSantaCruz::GetKind (void) const
{
  return TcpOption::SANTACRUZ;
  //return TcpOption::SACK;
}

uint16_t
TcpOptionSantaCruz::GetPacketIdData(void) const
{
  return m_packet_id_data;
}

uint16_t
TcpOptionSantaCruz::GetPacketIdAck(void) const
{
  return m_packet_id_ack;
}

uint32_t
TcpOptionSantaCruz::GetTimestamp (void) const
{
  return m_timestamp;
}

void
TcpOptionSantaCruz::SetPacketIdData (uint16_t id)
{
  m_packet_id_data = id;
}
void
TcpOptionSantaCruz::SetPacketIdAck (uint16_t id)
{
  m_packet_id_ack = id;
}

void
TcpOptionSantaCruz::SetTimestamp (uint32_t ts)
{
  m_timestamp = ts;
}

uint32_t
TcpOptionSantaCruz::NowToTimeStampValue ()
{
  uint64_t now = (uint64_t) Simulator::Now ().GetMilliSeconds ();

  return (now & 0xFFFFFFFF);
}

} // namespace ns3