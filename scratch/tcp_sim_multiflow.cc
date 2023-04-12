/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
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

#include "ns3/core-module.h"
#include "ns3/command-line.h"
#include "ns3/config.h"
#include "ns3/string.h"
#include "ns3/log.h"
#include "ns3/point-to-point-module.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/ssid.h"
#include "ns3/mobility-helper.h"
#include "ns3/on-off-helper.h"
#include "ns3/yans-wifi-channel.h"
#include "ns3/mobility-model.h"
#include "ns3/packet-sink.h"
#include "ns3/packet-sink-helper.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/traffic-control-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/network-module.h"

// for net anim
#include "ns3/netanim-module.h"

//flow monitor
#include "ns3/flow-monitor.h"
#include "ns3/flow-monitor-helper.h"

// Default Network Topology
//
//   Wifi 10.1.1.0
//                        AP
//  *           *    *    *
//  |           |    |    |   10.1.3.0      10.1.4.0      10.1.5.0
// n_nwifi/2... n1   n0   R0 ---------- R2----------- R3 ---------R1   n_(nwifi/2+1) ...   n_nwifi
//                   point-to-point                                |       |                  |    
//                                                                 *       *                  *    
//                                                                      Wifi 10.1.2.0


using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("tcp-simulation-script");

std::string dir;
uint32_t prev = 0;
Time prevTime = Seconds (0);

std::vector<Ptr<PacketSink>> sink_app;                         /* Pointer to the packet sink application */
// std::vector<uint64_t> lastTotalRx;                             /* The value of the last total received bytes */

// // Calculate throughput
// static void
// TraceThroughput (int num)
// {
//   Time curTime = Now ();
//   std::ofstream thr (dir + "/throughPut/throughput"+std::to_string(num)+".dat", std::ios::out | std::ios::app);
//   //std::cout<<lastTotalRx[num]<<std::endl;
//   double cur = (sink_app[num]->GetTotalRx () - lastTotalRx[num]) * (double) 8 / 1e5;     /* Convert Application RX Packets to MBits. */
//   thr <<  curTime.GetSeconds() << "s: \t" << cur <<"Mbit/s"<< std::endl;
//   lastTotalRx[num] = sink_app[num]->GetTotalRx ();
//   //std::cout<<lastTotalRx[num]<<std::endl;
//   Simulator::Schedule (MilliSeconds (100), &TraceThroughput, num);
// }

// static bool firstCwnd = true;
// static Ptr<OutputStreamWrapper> cWndStream;
// static uint32_t cWndValue;

// static void
// CwndTracer (uint32_t oldval, uint32_t newval)
// {
//   if (firstCwnd)
//     {
//       *cWndStream->GetStream () << "0.0 " << oldval << std::endl;
//       firstCwnd = false;
//     }
//   *cWndStream->GetStream () << Simulator::Now ().GetSeconds () << " " << oldval << " " << newval << std::endl;
//   cWndValue = newval;
// }

// static void
// TraceCwnd (std::string cwnd_tr_file_name)
// {
//   AsciiTraceHelper ascii;
//   cWndStream = ascii.CreateFileStream (cwnd_tr_file_name.c_str ());
//   Config::ConnectWithoutContext ("/NodeList/6/$ns3::TcpL4Protocol/SocketList/0/CongestionWindow", MakeCallback (&CwndTracer));
// }

int
main (int argc, char *argv[])
{
  int n_wifi = 30;
  int n_flow = 25;
  int n_pkts_sec = 100;
  int n_speed = 5;
  int segment_size = 1448;

  std::string leftlink_speed = "50Mbps";
  std::string leftlink_delay = "2ms";
  std::string rightlink_speed = "40Mbps";
  std::string rightlink_delay = "3ms";
  std::string bottlenecklink_speed = "20Mbps";
  std::string bottlenecklink_delay = "5ms";


  //std::string tcpTypeId = "TcpSantaCruz";
  //std::string tcpTypeId = "TcpNewReno";
  std::string tcpTypeId = "TcpCubic";
  std::string queueDisc = "FifoQueueDisc";
  uint32_t delAckCount = 1;
  //bool bql = false;
  bool enablePcap = false;
  Time stopTime = Seconds (10);

  CommandLine cmd (__FILE__);

  cmd.AddValue ("tcp_type", "Transport protocol to use: TcpNewReno, TcpBbr", tcpTypeId);
  cmd.AddValue ("delAckCount", "Delayed ACK count", delAckCount);
  cmd.AddValue ("enablePcap", "Enable/Disable pcap file generation", enablePcap);
  cmd.AddValue ("stopTime", "Stop time for applications / simulation time will be stopTime + 1", stopTime);
  cmd.AddValue ("n_wifi", "total number of wifi nodes on both sides", n_wifi);
  cmd.AddValue ("n_flow", "total number of flows to generate", n_flow);
  cmd.AddValue ("n_pkts_sec", "total number of packets sent per second", n_pkts_sec);
  cmd.AddValue ("n_speed", "speed of wifi nodes on both sides", n_speed);

  cmd.Parse (argc, argv);

  queueDisc = std::string ("ns3::") + queueDisc;

  Config::SetDefault ("ns3::TcpL4Protocol::SocketType", StringValue ("ns3::" + tcpTypeId));
  Config::SetDefault ("ns3::TcpSocket::SndBufSize", UintegerValue (4194304));
  Config::SetDefault ("ns3::TcpSocket::RcvBufSize", UintegerValue (6291456));
  Config::SetDefault ("ns3::TcpSocket::InitialCwnd", UintegerValue (2));
  Config::SetDefault ("ns3::TcpSocket::DelAckCount", UintegerValue (delAckCount));
  Config::SetDefault ("ns3::TcpSocket::SegmentSize", UintegerValue (segment_size));
  Config::SetDefault ("ns3::DropTailQueue<Packet>::MaxSize", QueueSizeValue (QueueSize ("10p")));
  Config::SetDefault (queueDisc + "::MaxSize", QueueSizeValue (QueueSize ("100p")));

  // create nodes

  NodeContainer left_nodes, right_nodes, routers;

  routers.Create(4);
  
  left_nodes.Create(n_wifi/2);
  NodeContainer wifiApNode_left = routers.Get(0);

  YansWifiChannelHelper channel_left = YansWifiChannelHelper::Default ();
  // channel_left.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
  // channel_left.AddPropagationLoss ("ns3::FriisPropagationLossModel", "Frequency", DoubleValue (5e9));

  YansWifiPhyHelper phy_left;
  phy_left.SetChannel (channel_left.Create ());

  WifiHelper wifi_left;
  wifi_left.SetRemoteStationManager ("ns3::AarfWifiManager");

  WifiMacHelper mac_left;
  Ssid ssid_left = Ssid ("ns-3-ssid");
  mac_left.SetType ("ns3::StaWifiMac",
               "Ssid", SsidValue (ssid_left),
               "ActiveProbing", BooleanValue (false));

  NetDeviceContainer staDevices_left;
  staDevices_left = wifi_left.Install (phy_left, mac_left, left_nodes);

  mac_left.SetType ("ns3::ApWifiMac",
               "Ssid", SsidValue (ssid_left));

  NetDeviceContainer apDevices_left;
  apDevices_left = wifi_left.Install (phy_left, mac_left, wifiApNode_left);


  MobilityHelper mobility;

  // mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
  //                                "MinX", DoubleValue (0.0),
  //                                "MinY", DoubleValue (0.0),
  //                                "DeltaX", DoubleValue (5.0),
  //                                "DeltaY", DoubleValue (10.0),
  //                                "GridWidth", UintegerValue (3),
  //                                "LayoutType", StringValue ("RowFirst"));
  mobility.SetPositionAllocator ("ns3::RandomRectanglePositionAllocator");

  mobility.SetMobilityModel ("ns3::RandomWalk2dMobilityModel",  // must set some value of speed
                             "Bounds", RectangleValue (Rectangle (-50, 50, -50, 50)));
  mobility.Install (left_nodes);

  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (wifiApNode_left);

  right_nodes.Create(n_wifi/2);
  NodeContainer wifiApNode_right = routers.Get(1);

  YansWifiChannelHelper channel_right = YansWifiChannelHelper::Default ();
  // channel_right.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
  // channel_right.AddPropagationLoss ("ns3::FriisPropagationLossModel", "Frequency", DoubleValue (5e9));

  YansWifiPhyHelper phy_right;
  phy_right.SetChannel (channel_right.Create ());

  WifiHelper wifi_right;
  wifi_right.SetRemoteStationManager ("ns3::AarfWifiManager");

  WifiMacHelper mac_right;
  Ssid ssid_right = Ssid ("ns-3-ssid");
  mac_right.SetType ("ns3::StaWifiMac",
               "Ssid", SsidValue (ssid_right),
               "ActiveProbing", BooleanValue (false));

  NetDeviceContainer staDevices_right;
  staDevices_right = wifi_right.Install (phy_right, mac_right, right_nodes);

  mac_right.SetType ("ns3::ApWifiMac",
               "Ssid", SsidValue (ssid_right));

  NetDeviceContainer apDevices_right;
  apDevices_right = wifi_right.Install (phy_right, mac_right, wifiApNode_right);


  MobilityHelper mobility1;

  mobility1.SetPositionAllocator ("ns3::RandomRectanglePositionAllocator");

  mobility1.SetMobilityModel ("ns3::RandomWalk2dMobilityModel",  // must set some value of speed
                             "Bounds", RectangleValue (Rectangle (-50, 50, -50, 50)));
  mobility1.Install (right_nodes);

  mobility1.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility1.Install (wifiApNode_right);

  // create point to point (router) links 
  PointToPointHelper left_link, bottleneck_link, right_link;

  left_link.SetDeviceAttribute("DataRate", StringValue(leftlink_speed));
  left_link.SetChannelAttribute("Delay", StringValue(leftlink_delay));

  bottleneck_link.SetDeviceAttribute("DataRate", StringValue(bottlenecklink_speed));
  bottleneck_link.SetChannelAttribute("Delay", StringValue(bottlenecklink_delay));

  right_link.SetDeviceAttribute("DataRate", StringValue(rightlink_speed));
  right_link.SetChannelAttribute("Delay", StringValue(rightlink_delay));

  //Create NetDevice Containers for p2p

  NetDeviceContainer leftlink_net = left_link.Install(routers.Get(0),routers.Get(2));
  NetDeviceContainer bottlenecklink_net = bottleneck_link.Install(routers.Get(2),routers.Get(3));
  NetDeviceContainer rightlink_net = right_link.Install(routers.Get(3),routers.Get(1));

  //Install Stack
  InternetStackHelper intrnt;
  intrnt.Install(left_nodes);
  intrnt.Install(right_nodes);
  intrnt.Install(routers);

  //traffic control? from tcp-bbr

  //Assign IP Address
  Ipv4AddressHelper address;

  address.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer leftnodes_interface;
  leftnodes_interface = address.Assign (staDevices_left);
  address.Assign(apDevices_left);

  address.SetBase ("10.1.2.0", "255.255.255.0");
  Ipv4InterfaceContainer rightnodes_interface;
  rightnodes_interface = address.Assign (staDevices_right);
  address.Assign(apDevices_right);

  address.SetBase ("10.1.3.0", "255.255.255.0");
  Ipv4InterfaceContainer r0r2;
  r0r2 = address.Assign(leftlink_net);

  address.SetBase ("10.1.4.0", "255.255.255.0");
  Ipv4InterfaceContainer r2r3;
  r2r3 = address.Assign(bottlenecklink_net);

  address.SetBase ("10.1.5.0", "255.255.255.0");
  Ipv4InterfaceContainer r3r1;
  r3r1 = address.Assign(rightlink_net);

  // Populate routing tables
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  // for(int i = 0; i < n_flow; i++)
  // {
  //   lastTotalRx.push_back(0);
  // }

  srand(time(0));

  for(int i = 0; i < n_flow/2; i++)
  {
    //int sender = (rand() % (n_wifi/2));
    //int receiver = (rand() % (n_wifi/2));
    //int sender = 1, receiver = 1;
    
    // Select sender side port
    uint16_t port = 5000+i;
    //uint16_t port = 50000;

    // Install application on the sender
    OnOffHelper source ("ns3::TcpSocketFactory", InetSocketAddress (rightnodes_interface.GetAddress (i), port));
    source.SetAttribute ("MaxBytes", UintegerValue (0));
    source.SetConstantRate (DataRate ("100Kbps"));
    source.SetAttribute ("PacketSize", UintegerValue (1448));
    ApplicationContainer sourceApps = source.Install (left_nodes.Get (i));
    sourceApps.Start (Seconds (0.0));
    //Simulator::Schedule (Seconds (0.2), &TraceCwnd, 0, 0);
    sourceApps.Stop (stopTime);

    // Install application on the receiver
    PacketSinkHelper sink ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), port));
    ApplicationContainer sinkApps = sink.Install (right_nodes.Get (i));

    Ptr<PacketSink> temp = StaticCast<PacketSink> (sinkApps.Get (0));

    sink_app.push_back(temp);
    //lastTotalRx.push_back(0);
    sinkApps.Start (Seconds (0.0));
    sinkApps.Stop (stopTime);

    //throughput calculation
    //Simulator::Schedule (Seconds (0.1), &TraceThroughput, i);
  }

  for(int i = 0; i < n_flow/2; i++)
  {
    //int sender = (rand() % (n_wifi/2)) + 1;
    //int receiver = (rand() % (n_wifi/2)) + 1;
    
    // Select sender side port
    uint16_t port = 5000+i;

    // Install application on the sender
    OnOffHelper source ("ns3::TcpSocketFactory", InetSocketAddress (leftnodes_interface.GetAddress (i), port));
    source.SetAttribute ("MaxBytes", UintegerValue (0));
    source.SetConstantRate (DataRate ("100Kbps"));
    source.SetAttribute ("PacketSize", UintegerValue (1448));
    ApplicationContainer sourceApps = source.Install (right_nodes.Get (i));
    sourceApps.Start (Seconds (0.0));
    //Simulator::Schedule (Seconds (0.2), &TraceCwnd, 0, 0);
    sourceApps.Stop (stopTime);

    // Install application on the receiver
    PacketSinkHelper sink ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), port));
    ApplicationContainer sinkApps = sink.Install (left_nodes.Get (i));

    Ptr<PacketSink> temp = StaticCast<PacketSink> (sinkApps.Get (0));

    sink_app.push_back(temp);
    //lastTotalRx.push_back(0);
    sinkApps.Start (Seconds (0.0));
    sinkApps.Stop (stopTime);

    //throughput calculation
    //Simulator::Schedule (Seconds (0.1), &TraceThroughput, i);
  }

  // Create a new directory to store the output of the program
  // Naming the output directory using local system time
  time_t rawtime;
  struct tm * timeinfo;
  char buffer [80];
  time (&rawtime);
  timeinfo = localtime (&rawtime);
  strftime (buffer, sizeof (buffer), "%d-%m-%Y-%I-%M-%S", timeinfo);
  std::string currentTime (buffer);

  dir = "tcp-sim-multiflow-results/" + tcpTypeId +currentTime+ "/";
  std::string dirToSave = "mkdir -p " + dir;
  if (system (dirToSave.c_str ()) == -1)
  {
    exit (1);
  }
  // dirToSave = "mkdir -p " + dir+"throughPut/";
  // if (system (dirToSave.c_str ()) == -1)
  // {
  //   exit (1);
  // }

  //Simulator::Schedule (Seconds (0.2), &TraceCwnd, dir + "/cwnd.dat");


  // for anim
  // AnimationInterface anim("first.xml");
  // anim.SetConstantPosition(nodes.Get(0),10,10);
  // anim.SetConstantPosition(nodes.Get(1),20,20);

  //Ascii format tracing
  // AsciiTraceHelper ascii;
  // phy_left.EnableAsciiAll(ascii.CreateFileStream(dir+"phy_left.tr"));
  // phy_right.EnableAsciiAll(ascii.CreateFileStream(dir+"phy_right.tr"));
  // left_link.EnableAsciiAll(ascii.CreateFileStream(dir+"left_link.tr"));
  // right_link.EnableAsciiAll(ascii.CreateFileStream(dir+"right_link.tr"));
  // bottleneck_link.EnableAsciiAll(ascii.CreateFileStream(dir+"bottleneck_link.tr"));

  //bottleneck_link.EnablePcapAll ("tcp_sim", true);

  FlowMonitorHelper flowmon;
  Ptr<FlowMonitor> monitor = flowmon.InstallAll ();
  //Simulator::Schedule (Seconds (0 + 0.000001), &TraceThroughput, monitor);

  Simulator::Stop (stopTime + TimeStep (1));

  Simulator::Run ();
  monitor->SerializeToXmlFile(dir+"/tcp_sim_multiflow.xml", false, false);
  // std::vector<double> averageThroughput;
  // for(int i = 0; i < n_flow; i++)
  // {
  //   averageThroughput.push_back((sink_app[i]->GetTotalRx () * 8.0)/(1e6 * 10.0));
  // }
  Simulator::Destroy ();
  // double total_throughput = 0;
  // for(int i = 0; i < n_flow; i++)
  // {
  //   std::cout << "\nAverage throughput: (flowpair "<<i <<") "<< averageThroughput[i]<< " Mbit/s" << std::endl;
  //   total_throughput+=averageThroughput[i];
  // }
  // std::cout << "\nAverage throughput: " << (total_throughput/n_flow) << " Mbit/s" << std::endl;
  return 0;
}