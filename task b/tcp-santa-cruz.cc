/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014 Natale Patriciello, <natale.patriciello@gmail.com>
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

#include "tcp-santa-cruz.h"
#include "tcp-socket-state.h"

#include "ns3/log.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("TcpSantaCruz");
NS_OBJECT_ENSURE_REGISTERED (TcpSantaCruz);

TypeId
TcpSantaCruz::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::TcpSantaCruz")
    .SetParent<TcpNewReno> ()
    .SetGroupName ("Internet")
    .AddConstructor<TcpSantaCruz> ()
  ;
  return tid;
}

TcpSantaCruz::TcpSantaCruz (void)
  : TcpNewReno ()
{
  NS_LOG_FUNCTION (this);
}

TcpSantaCruz::TcpSantaCruz (const TcpSantaCruz& sock)
  : TcpNewReno (sock)
{
  NS_LOG_FUNCTION (this);
}

TcpSantaCruz::~TcpSantaCruz (void)
{
}

/**
 * \brief TcpSantaCruz slow start algorithm
 *
 *
 * \param tcb internal congestion state
 * \param segmentsAcked count of segments acked
 * \return the number of segments not considered for increasing the cWnd
 */
uint32_t
TcpSantaCruz::SlowStart (Ptr<TcpSocketState> tcb, uint32_t segmentsAcked)
{
  NS_LOG_FUNCTION (this << tcb << segmentsAcked);

  if (segmentsAcked >= 1)
    {
      tcb->m_cWnd += tcb->m_segmentSize;
      NS_LOG_INFO ("In SlowStart, updated to cwnd " << tcb->m_cWnd << " ssthresh " << tcb->m_ssThresh);
      return segmentsAcked - 1;
    }

  return 0;
}

/**
 * \brief TcpSantaCruz congestion avoidance
 *
 * During congestion avoidance, cwnd is incremented by roughly 1 full-sized
 * segment per round-trip time (RTT).
 *
 * \param tcb internal congestion state
 * \param segmentsAcked count of segments acked
 */
void
TcpSantaCruz::CongestionAvoidance (Ptr<TcpSocketState> tcb, uint32_t segmentsAcked)
{
  NS_LOG_FUNCTION (this << tcb << segmentsAcked);

  if (segmentsAcked > 0)
    {
      double adder = static_cast<double> (tcb->m_segmentSize * tcb->m_segmentSize) / tcb->m_cWnd.Get ();
      adder = std::max (1.0, adder);
      tcb->m_cWnd += static_cast<uint32_t> (adder);
      NS_LOG_INFO ("In CongAvoid, updated to cwnd " << tcb->m_cWnd <<
                   " ssthresh " << tcb->m_ssThresh);
    }
}

/**
 * \brief Try to increase the cWnd following the TcpSantaCruz specification
 *
 * \see SlowStart
 * \see CongestionAvoidance
 *
 * \param tcb internal congestion state
 * \param segmentsAcked count of segments acked  //virtual 
 */
void
TcpSantaCruz::IncreaseWindow (Ptr<TcpSocketState> tcb, uint32_t segmentsAcked)
{
  NS_LOG_FUNCTION (this << tcb << segmentsAcked);

  // if(is_in_slow_start)
  // {
  //   if (tcb->m_cWnd < tcb->m_ssThresh)
  //   {
  //     segmentsAcked = SlowStart (tcb, segmentsAcked);
  //     is_in_slow_start = true;
  //   }
  //   else if (tcb->m_cWnd >= tcb->m_ssThresh)
  //   {
  //     CongestionAvoidance (tcb, segmentsAcked);
  //     is_in_slow_start = false;
  //   }
  // }
  // else
  // {
  //   if (tcb->m_cWnd >= tcb->m_ssThresh)
  //   {
  //     CongestionAvoidance (tcb, segmentsAcked);
  //     is_in_slow_start = false;
  //   }
  //   else if (tcb->m_cWnd < tcb->m_ssThresh)
  //   {
  //     segmentsAcked = SlowStart (tcb, segmentsAcked);
  //     is_in_slow_start = true;
  //   }
  // }

  if (tcb->m_cWnd < tcb->m_ssThresh)
    {
      segmentsAcked = SlowStart (tcb, segmentsAcked);
    }

  if (tcb->m_cWnd >= tcb->m_ssThresh)
    {
      CongestionAvoidance (tcb, segmentsAcked);
    }

}

void TcpSantaCruz::IncreaseWindowSantaCruz (Ptr<TcpSocketState> tcb, uint32_t curr_qued_pkts, uint32_t op_point)
{
  //NS_LOG_UNCOND ("Santa Cruz Window Increase");

  tcb->m_cWnd += tcb->m_segmentSize;
  if ((tcb->m_cWnd >= tcb->m_ssThresh) || (curr_qued_pkts > op_point/2))
  {
    tcb->m_cWnd -= tcb->m_segmentSize;
    is_in_slow_start = false;
  }
}
void TcpSantaCruz::DecreaseWindowSantaCruz (Ptr<TcpSocketState> tcb, uint32_t curr_qued_pkts, uint32_t op_point)
{
  //NS_LOG_UNCOND ("Santa Cruz Window Decrease");
  if(tcb->m_cWnd >= 2*tcb->m_segmentSize) tcb->m_cWnd -= tcb->m_segmentSize;

  if ((tcb->m_cWnd < tcb->m_ssThresh) && (curr_qued_pkts <= op_point/2))
  {
    is_in_slow_start = true;
  }
}

std::string
TcpSantaCruz::GetName () const
{
  return "TcpSantaCruz";
}

uint32_t
TcpSantaCruz::GetSsThresh (Ptr<const TcpSocketState> state,
                         uint32_t bytesInFlight)
{
  NS_LOG_FUNCTION (this << state << bytesInFlight);

  return std::max (2 * state->m_segmentSize, bytesInFlight / 2);
}

Ptr<TcpCongestionOps>
TcpSantaCruz::Fork ()
{
  return CopyObject<TcpSantaCruz> (this);
}
} // namespace ns3
