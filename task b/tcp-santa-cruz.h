/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015 Natale Patriciello <natale.patriciello@gmail.com>
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
 *
 */
#ifndef TCPSANTACRUZ_H
#define TCPSANTACRUZ_H

#include "tcp-congestion-ops.h"
#include "tcp-rate-ops.h"
#include "tcp-socket-state.h"

namespace ns3 {

    /**
 * \brief The Santa Cruz implementation
 *
 *
 * \see IncreaseWindow
 */
class TcpSantaCruz : public TcpNewReno
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  TcpSantaCruz ();

  /**
   * \brief Copy constructor.
   * \param sock object to copy.
   */
  TcpSantaCruz (const TcpSantaCruz& sock);

  ~TcpSantaCruz ();

  std::string GetName () const;

  virtual void IncreaseWindow (Ptr<TcpSocketState> tcb, uint32_t segmentsAcked);
  virtual uint32_t GetSsThresh (Ptr<const TcpSocketState> tcb,
                                uint32_t bytesInFlight);
  virtual Ptr<TcpCongestionOps> Fork ();

  virtual void IncreaseWindowSantaCruz (Ptr<TcpSocketState> tcb, uint32_t curr_qued_pkts, uint32_t op_point);
  virtual void DecreaseWindowSantaCruz (Ptr<TcpSocketState> tcb, uint32_t curr_qued_pkts, uint32_t op_point);

protected:
  virtual uint32_t SlowStart (Ptr<TcpSocketState> tcb, uint32_t segmentsAcked);
  virtual void CongestionAvoidance (Ptr<TcpSocketState> tcb, uint32_t segmentsAcked);
  bool is_in_slow_start {true};
};

} // namespace ns3

#endif // TCPSANTACRUZ_H