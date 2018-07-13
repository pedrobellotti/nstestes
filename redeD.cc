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
#include "ns3/point-to-point-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/netanim-module.h"
#include "ns3/flow-monitor-module.h"

// Default Network Topology
//
//                                          Wifi
//  C1 C2 S3 S4 --------------*AP*  |-------------------|
//  |  |  |   |        P2P          | C3          C4    |
//  ===========                     |     S1        S2  |
//    LAN                           |-------------------|
//
//  Conexões WiFi:
//  C1 -> S1
//  C2 -> S2
//  C3 -> S3
//  C4 -> S4
//
//  IPs:
//  LAN 10.1.1.0
//  P2P 10.1.2.0
//  WIFI 10.1.3.0
//

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("redeD");

int 
main (int argc, char *argv[])
{
  LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
  LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);

  //Criando S4 e AP (conexao P2P)
  NodeContainer p2pNodes;
  p2pNodes.Create (2);

  PointToPointHelper pointToPoint;
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("10Mbps"));
  pointToPoint.SetChannelAttribute ("Delay", StringValue ("1ms"));

  NetDeviceContainer p2pDevices;
  p2pDevices = pointToPoint.Install (p2pNodes);

  //Criando C1,C2,S3,S4 (conexao LAN)
  NodeContainer csmaNodes;
  csmaNodes.Create (3);
  csmaNodes.Add (p2pNodes.Get (0));

  CsmaHelper csma;
  csma.SetChannelAttribute ("DataRate", StringValue ("100Mbps"));
  csma.SetChannelAttribute ("Delay", TimeValue (NanoSeconds (6560)));

  NetDeviceContainer csmaDevices;
  csmaDevices = csma.Install (csmaNodes);

  //Criando C3,C4,S1,S2 (conexao Wifi)
  NodeContainer wifiStaNodes;
  wifiStaNodes.Create (4);
  NodeContainer wifiApNode = p2pNodes.Get (1); //Colocando AP no AP

  YansWifiChannelHelper channel = YansWifiChannelHelper::Default ();
  YansWifiPhyHelper phy = YansWifiPhyHelper::Default ();
  phy.SetChannel (channel.Create ());

  //Criando rede Wifi
  WifiHelper wifi;
  wifi.SetRemoteStationManager ("ns3::AarfWifiManager");

  WifiMacHelper mac;
  Ssid ssid = Ssid ("ns-3-ssid");
  mac.SetType ("ns3::StaWifiMac",
               "Ssid", SsidValue (ssid),
               "ActiveProbing", BooleanValue (false));

  NetDeviceContainer staDevices;
  staDevices = wifi.Install (phy, mac, wifiStaNodes);

  mac.SetType ("ns3::ApWifiMac",
               "Ssid", SsidValue (ssid));

  NetDeviceContainer apDevices;
  apDevices = wifi.Install (phy, mac, wifiApNode);

  //Adicionando mobilidade
  MobilityHelper mobility;
  mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
                                 "MinX", DoubleValue (1.0),
                                 "MinY", DoubleValue (1.0),
                                 "DeltaX", DoubleValue (5.0),
                                 "DeltaY", DoubleValue (7.0),
                                 "GridWidth", UintegerValue (2),
                                 "LayoutType", StringValue ("RowFirst"));

  mobility.SetMobilityModel ("ns3::RandomWalk2dMobilityModel",
                             "Bounds", RectangleValue (Rectangle (0, 10, 0, 10)));
  mobility.Install (wifiStaNodes);

  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (wifiApNode);
  mobility.Install (csmaNodes);

  //Adicionando protocolos em todos os nos
  InternetStackHelper stack;
  stack.Install (csmaNodes);
  stack.Install (wifiApNode);
  stack.Install (wifiStaNodes);

  Ipv4AddressHelper address;

  address.SetBase ("10.1.2.0", "255.255.255.0");
  Ipv4InterfaceContainer p2pInterfaces;
  p2pInterfaces = address.Assign (p2pDevices);

  address.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer csmaInterfaces;
  csmaInterfaces = address.Assign (csmaDevices);

  address.SetBase ("10.1.3.0", "255.255.255.0");
  Ipv4InterfaceContainer wifiInterfaces;
  wifiInterfaces = address.Assign (staDevices);
  address.Assign (apDevices);

  //Criando aplicacoes para bulk-send (clientes)
  BulkSendHelper clienteC1 ("ns3::TcpSocketFactory",
                           InetSocketAddress (wifiInterfaces.GetAddress (0), 9));
  clienteC1.SetAttribute ("MaxBytes", UintegerValue (0));
  ApplicationContainer c1Apps = clienteC1.Install (csmaNodes.Get (0));
  c1Apps.Start (Seconds (2.0));
  c1Apps.Stop (Seconds (10.0));

  BulkSendHelper clienteC2 ("ns3::TcpSocketFactory",
                           InetSocketAddress (wifiInterfaces.GetAddress (1), 9));
  clienteC2.SetAttribute ("MaxBytes", UintegerValue (0));
  ApplicationContainer c2Apps = clienteC2.Install (csmaNodes.Get (1));
  c2Apps.Start (Seconds (2.0));
  c2Apps.Stop (Seconds (10.0));

  BulkSendHelper clienteC3 ("ns3::TcpSocketFactory",
                            InetSocketAddress (csmaInterfaces.GetAddress (2), 9));
  clienteC3.SetAttribute ("MaxBytes", UintegerValue (0));
  ApplicationContainer c3Apps = clienteC3.Install (wifiStaNodes.Get (0));
  c3Apps.Start (Seconds (2.0));
  c3Apps.Stop (Seconds (10.0));

  BulkSendHelper clienteC4 ("ns3::TcpSocketFactory",
                           InetSocketAddress (csmaInterfaces.GetAddress (3), 9));
  clienteC4.SetAttribute ("MaxBytes", UintegerValue (0));
  ApplicationContainer c4Apps = clienteC4.Install (wifiStaNodes.Get (1));
  c4Apps.Start (Seconds (2.0));
  c4Apps.Stop (Seconds (10.0));

  //Criando aplicacoes para sink (servidores)
  PacketSinkHelper sinkS1 ("ns3::TcpSocketFactory",
                          InetSocketAddress (Ipv4Address::GetAny (), 9));
  ApplicationContainer s1Apps = sinkS1.Install (wifiStaNodes.Get (0));
  s1Apps.Start (Seconds (1.0));
  s1Apps.Stop (Seconds (10.0));

  PacketSinkHelper sinkS2 ("ns3::TcpSocketFactory",
                          InetSocketAddress (Ipv4Address::GetAny (), 9));
  ApplicationContainer s2Apps = sinkS2.Install (wifiStaNodes.Get (1));
  s2Apps.Start (Seconds (1.0));
  s2Apps.Stop (Seconds (10.0));

  PacketSinkHelper sinkS3 ("ns3::TcpSocketFactory",
                          InetSocketAddress (Ipv4Address::GetAny (), 9));
  ApplicationContainer s3Apps = sinkS3.Install (csmaNodes.Get (2));
  s3Apps.Start (Seconds (1.0));
  s3Apps.Stop (Seconds (10.0));

  PacketSinkHelper sinkS4 ("ns3::TcpSocketFactory",
                          InetSocketAddress (Ipv4Address::GetAny (), 9));
  ApplicationContainer s4Apps = sinkS4.Install (csmaNodes.Get (3));
  s4Apps.Start (Seconds (1.0));
  s4Apps.Stop (Seconds (10.0));

  //NetAnim
  AnimationInterface anim ("redeD.xml");
  anim.SetConstantPosition (csmaNodes.Get(0), 4.0, 15.0); //C1
  anim.SetConstantPosition (csmaNodes.Get(1), 8.0, 15.0); //C2
  anim.SetConstantPosition (csmaNodes.Get(2), 12.0, 15.0); //S3
  anim.SetConstantPosition (csmaNodes.Get(3), 16.0, 15.0); //S4

  anim.SetConstantPosition (p2pNodes.Get(1), 10.0, 13.0); //AP

  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  //Pcap
  pointToPoint.EnablePcapAll("redeD");

  // Flow monitor
  Ptr<FlowMonitor> flowMonitor;
  FlowMonitorHelper flowHelper;
  flowMonitor = flowHelper.InstallAll();

  Simulator::Stop (Seconds (10.0));
  Simulator::Run ();
  flowMonitor->SerializeToXmlFile("flowRedeD.xml", true, true);
  Simulator::Destroy ();

  Ptr<PacketSink> sink1 = DynamicCast<PacketSink> (s1Apps.Get (0));
  Ptr<PacketSink> sink2 = DynamicCast<PacketSink> (s2Apps.Get (0));
  Ptr<PacketSink> sink3 = DynamicCast<PacketSink> (s3Apps.Get (0));
  Ptr<PacketSink> sink4 = DynamicCast<PacketSink> (s4Apps.Get (0));
  std::cout << "(S1) Total Bytes Received: " << sink1->GetTotalRx () << std::endl;
  std::cout << "(S2) Total Bytes Received: " << sink2->GetTotalRx () << std::endl;
  std::cout << "(S3) Total Bytes Received: " << sink3->GetTotalRx () << std::endl;
  std::cout << "(S4) Total Bytes Received: " << sink4->GetTotalRx () << std::endl;

  return 0;
}
