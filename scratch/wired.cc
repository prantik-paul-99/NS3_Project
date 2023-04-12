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
#include "ns3/network-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/ipv4-global-routing-helper.h"

//flow monitor
#include "ns3/flow-monitor.h"
#include "ns3/flow-monitor-helper.h"

// Default Network Topology
//                                            LAN3 10.1.3.0
//       LAN1 10.1.1.0            R2   (total_nodes/4)  nodes (after 10.1.2.0)
//    |       |    |  |   |   /      /
// (total_nodes/2)  nodes R0 --------- R1   (total_nodes/4)  nodes (after 10.1.1.0)
//                               point-to-point          |     |    |
//                                                       LAN2 10.1.2.0
// R0 -R1 10.1.4.0
// R0 -R2 10.1.5.0
// R1 -R2 10.1.6.0

// sender    receiver
// lan1       lan2
// lan1       lan3

using namespace ns3;
using namespace std;

NS_LOG_COMPONENT_DEFINE ("wired-network-simuation");

std::string dir;
uint32_t prev = 0;
Time prevTime = Seconds (0);

std::vector<Ptr<PacketSink>> sink_app;

int 
main (int argc, char *argv[])
{
    uint32_t ntotal = 60;
    int n_flow = 20;
    int n_pkts_sec = 500;
    uint32_t pkt_sz = 1024;

    double start_time = 0;
    double stop_time = 10.0;

    std::string r0r1_speed = "10Mbps";
    std::string r0r1_delay = "5ms";
    std::string r0r2_speed = "30Mbps";
    std::string r0r2_delay = "2ms";
    std::string r1r2_speed = "20Mbps";
    std::string r1r2_delay = "3ms";

    std::string tcpTypeId = "TcpNewReno";
    std::string queueDisc = "FifoQueueDisc";
    uint32_t delAckCount = 2;

    //bool enablePcap = false;
    Time stopTime = Seconds (stop_time);

    CommandLine cmd (__FILE__);
    cmd.AddValue ("tcp_type", "Transport protocol to use: TcpNewReno, TcpBbr", tcpTypeId);
    cmd.AddValue ("delAckCount", "Delayed ACK count", delAckCount);
    cmd.AddValue ("nTotal", "Number of nodes", ntotal);
    cmd.AddValue ("stopTime", "Stop time for applications / simulation time will be stopTime + 1", stopTime);
    cmd.AddValue ("n_flow", "total number of flows to generate", n_flow);
    cmd.AddValue ("n_pkts_sec", "total number of packets per second", n_pkts_sec);
    cmd.Parse (argc,argv);

    queueDisc = std::string ("ns3::") + queueDisc;

    Config::SetDefault ("ns3::TcpL4Protocol::SocketType", StringValue ("ns3::" + tcpTypeId));
    Config::SetDefault ("ns3::TcpSocket::SndBufSize", UintegerValue (4194304));
    Config::SetDefault ("ns3::TcpSocket::RcvBufSize", UintegerValue (6291456));
    Config::SetDefault ("ns3::TcpSocket::InitialCwnd", UintegerValue (2));
    Config::SetDefault ("ns3::TcpSocket::DelAckCount", UintegerValue (delAckCount));
    Config::SetDefault ("ns3::TcpSocket::SegmentSize", UintegerValue (1448));
    Config::SetDefault ("ns3::DropTailQueue<Packet>::MaxSize", QueueSizeValue (QueueSize ("10p")));
    Config::SetDefault (queueDisc + "::MaxSize", QueueSizeValue (QueueSize ("100p")));


    ntotal = ntotal == 0 ? 1 : ntotal;

    cout<<"total nodes = "<<ntotal<<endl;
    cout<<"total flows = "<<n_flow<<endl;
    cout<<"packets per second = "<<n_pkts_sec<<endl;

    NodeContainer lan1, lan2, lan3, routers;

    routers.Create(3);

    lan1.Add (routers.Get(0));
    lan1.Create(ntotal/2);

    lan2.Add (routers.Get(1));
    lan2.Create(ntotal/4);

    lan3.Add (routers.Get(2));
    lan3.Create(ntotal/4);

    //point to point conection setting

    PointToPointHelper r0r1, r0r2, r1r2;

    r0r1.SetDeviceAttribute("DataRate", StringValue(r0r1_speed));
    r0r1.SetChannelAttribute("Delay", StringValue(r0r1_delay));

    r0r2.SetDeviceAttribute("DataRate", StringValue(r0r2_speed));
    r0r2.SetChannelAttribute("Delay", StringValue(r0r2_delay));

    r1r2.SetDeviceAttribute("DataRate", StringValue(r1r2_speed));
    r1r2.SetChannelAttribute("Delay", StringValue(r1r2_delay));

    NetDeviceContainer r0r1_net, r0r2_net, r1r2_net;

    r0r1_net = r0r1.Install(routers.Get(0),routers.Get(1));
    r0r2_net = r0r2.Install(routers.Get(0),routers.Get(2));
    r1r2_net = r1r2.Install(routers.Get(1),routers.Get(2));

    //csma connection setting

    CsmaHelper lan1_csma, lan2_csma, lan3_csma;

    //lan1_csma.SetChannelAttribute("DataRate", StringValue(n_pkts_sec/2+"Mbps"));
    lan1_csma.SetChannelAttribute("DataRate", StringValue("100Mbps"));
    lan1_csma.SetChannelAttribute("Delay", TimeValue (NanoSeconds (6560)));

    //lan2_csma.SetChannelAttribute("DataRate", StringValue(n_pkts_sec/2+"Mbps"));
    lan2_csma.SetChannelAttribute("DataRate", StringValue("75Mbps"));
    lan2_csma.SetChannelAttribute("Delay", TimeValue (NanoSeconds (10000)));

    //lan3_csma.SetChannelAttribute("DataRate", StringValue(n_pkts_sec/2+"Mbps"));
    lan3_csma.SetChannelAttribute("DataRate", StringValue("100Mbps"));
    lan3_csma.SetChannelAttribute("Delay", TimeValue (NanoSeconds (6560)));

    NetDeviceContainer lan1_net, lan2_net, lan3_net;

    lan1_net = lan1_csma.Install(lan1);
    lan2_net = lan1_csma.Install(lan2);
    lan3_net = lan1_csma.Install(lan3);

    Ptr<RateErrorModel> em = CreateObject<RateErrorModel> ();
    em->SetAttribute ("ErrorRate", DoubleValue (0.00001));
    for (uint32_t i = 0; i < lan1_net.GetN(); i++)
    {
        lan1_net.Get (i)->SetAttribute ("ReceiveErrorModel", PointerValue (em));
    }
    for (uint32_t i = 0; i < lan2_net.GetN(); i++)
    {
        lan2_net.Get (i)->SetAttribute ("ReceiveErrorModel", PointerValue (em));
    }
    for (uint32_t i = 0; i < lan3_net.GetN(); i++)
    {
        lan3_net.Get (i)->SetAttribute ("ReceiveErrorModel", PointerValue (em));
    }

    InternetStackHelper stack;
    //stack.Install (routers);
    stack.Install (lan1);
    stack.Install (lan2);
    stack.Install (lan3);

    //set ip addresses

    Ipv4AddressHelper address;
    address.SetBase ("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer lan1_interface;
    lan1_interface = address.Assign (lan1_net);

    address.SetBase ("10.1.2.0", "255.255.255.0");
    Ipv4InterfaceContainer lan2_interface;
    lan2_interface = address.Assign (lan2_net);

    address.SetBase ("10.1.3.0", "255.255.255.0");
    Ipv4InterfaceContainer lan3_interface;
    lan3_interface = address.Assign (lan3_net);

    address.SetBase ("10.1.4.0", "255.255.255.0");
    Ipv4InterfaceContainer r0r1_interface;
    r0r1_interface = address.Assign (r0r1_net);

    address.SetBase ("10.1.5.0", "255.255.255.0");
    Ipv4InterfaceContainer r0r2_interface;
    r0r2_interface = address.Assign (r0r2_net);

    address.SetBase ("10.1.6.0", "255.255.255.0");
    Ipv4InterfaceContainer r1r2_interface;
    r1r2_interface = address.Assign (r1r2_net);

    Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

    srand(time(0));

    int j = 0;
    for(j = 0; j < n_flow/4; j++)
    {
        int sender = (rand() % (ntotal/2));
        int receiver = (rand() % (ntotal/4));
        //int sender = 1, receiver = 1;
    
        // Select sender side port
        uint16_t port = 5000+j;
        //uint16_t port = 50000;

        // Install application on the sender
        OnOffHelper source ("ns3::TcpSocketFactory", InetSocketAddress (lan2_interface.GetAddress (receiver), port));
        source.SetAttribute ("PacketSize", UintegerValue (pkt_sz));
        source.SetAttribute ("MaxBytes", UintegerValue (0));
        source.SetConstantRate (DataRate (n_pkts_sec*pkt_sz*8));
        ApplicationContainer sourceApps = source.Install (lan1.Get (sender));
        sourceApps.Start (Seconds (start_time));
        //Simulator::Schedule (Seconds (0.2), &TraceCwnd, 0, 0);
        sourceApps.Stop (stopTime);

        // Install application on the receiver
        PacketSinkHelper sink ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), port));
        ApplicationContainer sinkApps = sink.Install (lan2.Get (receiver));

        Ptr<PacketSink> temp = StaticCast<PacketSink> (sinkApps.Get (0));

        sink_app.push_back(temp);
        sinkApps.Start (Seconds (start_time));
        sinkApps.Stop (stopTime);

    }

    for(j = j; j < n_flow/2; j++)
    {
        
        int sender = (rand() % (ntotal/2));
        int receiver = (rand() % (ntotal/4));
    
        // Select sender side port
        uint16_t port = 5000+j;

        // Install application on the sender
        OnOffHelper source ("ns3::TcpSocketFactory", InetSocketAddress (lan3_interface.GetAddress (receiver), port));
        source.SetAttribute ("PacketSize", UintegerValue (pkt_sz));
        source.SetAttribute ("MaxBytes", UintegerValue (0));
        source.SetConstantRate (DataRate (n_pkts_sec*pkt_sz*8));
        ApplicationContainer sourceApps = source.Install (lan1.Get (sender));
        sourceApps.Start (Seconds (start_time));
        //Simulator::Schedule (Seconds (0.2), &TraceCwnd, 0, 0);
        sourceApps.Stop (stopTime);

        // Install application on the receiver
        PacketSinkHelper sink ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), port));
        ApplicationContainer sinkApps = sink.Install (lan3.Get (receiver));

        Ptr<PacketSink> temp = StaticCast<PacketSink> (sinkApps.Get (0));

        sink_app.push_back(temp);
        sinkApps.Start (Seconds (start_time));
        sinkApps.Stop (stopTime);
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

    dir = "wired-sim-results/"+currentTime+ "/";
    std::string dirToSave = "mkdir -p " + dir;
    if (system (dirToSave.c_str ()) == -1)
    {
        exit (1);
    }
    // dirToSave = "mkdir -p " + dir+"throughPut/";
    // if (system (dirToSave.c_str ()) == -1)
    // {
    //     exit (1);
    // }

    FlowMonitorHelper flowmon;
    Ptr<FlowMonitor> monitor = flowmon.InstallAll ();
    //Simulator::Schedule (Seconds (0 + 0.000001), &TraceThroughput, monitor);

    Simulator::Stop (stopTime);

    Simulator::Run ();
    //monitor->SerializeToXmlFile(dir+"/wired_n_"+to_string(ntotal)+"_f_"+to_string(n_flow)+"_p_"+to_string(n_pkts_sec)+".xml", false, false);
    monitor->SerializeToXmlFile(dir+"/wired.xml", false, false);
    // std::vector<double> averageThroughput;
    // for(int i = 0; i < n_flow/2; i++)
    // {
    //     averageThroughput.push_back((sink_app[i]->GetTotalRx () * 8.0)/(1e3 * 10.0));
    // } 
    Simulator::Destroy ();
    // double total_throughput = 0;
    // for(int i = 0; i < n_flow/2; i++)
    // {
    //     std::cout << "\nAverage throughput: (flowpair "<<i <<") "<< averageThroughput[i]<< " Kbit/s" << std::endl;
    //     total_throughput+=averageThroughput[i];
    // }
    // std::cout << "\nAverage throughput: " << (total_throughput/(n_flow/2)) << " Kbit/s" << std::endl;
    return 0;




}