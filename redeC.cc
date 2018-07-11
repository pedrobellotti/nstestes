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
//  |  |  |  |	      P2P        		|  C3	       C4     |
//  ===========			                |	    S1	      S2  |
//     LAN				                  |-------------------|
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

NS_LOG_COMPONENT_DEFINE ("redeC");

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

  MobilityHelper mobility;

  mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
                                 "MinX", DoubleValue (0.0),
                                 "MinY", DoubleValue (0.0),
                                 "DeltaX", DoubleValue (5.0),
                                 "DeltaY", DoubleValue (10.0),
                                 "GridWidth", UintegerValue (3),
                                 "LayoutType", StringValue ("RowFirst"));

  mobility.SetMobilityModel ("ns3::RandomWalk2dMobilityModel",
                             "Bounds", RectangleValue (Rectangle (-10, 10, -10, 10)));
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

  UdpEchoServerHelper echoServer (9);

  //Instalando apps de servidor
  ApplicationContainer serverS1 = echoServer.Install (wifiStaNodes.Get (0));
  serverS1.Start (Seconds (1.0));
  serverS1.Stop (Seconds (10.0));

  ApplicationContainer serverS2 = echoServer.Install (wifiStaNodes.Get (1));
  serverS2.Start (Seconds (1.0));
  serverS2.Stop (Seconds (10.0));

  ApplicationContainer serverS3 = echoServer.Install (csmaNodes.Get (2));
  serverS3.Start (Seconds (1.0));
  serverS3.Stop (Seconds (10.0));

  ApplicationContainer serverS4 = echoServer.Install (csmaNodes.Get (3));
  serverS4.Start (Seconds (1.0));
  serverS4.Stop (Seconds (10.0));

  //Configurando clientes
  UdpEchoClientHelper echoClientC1 (wifiInterfaces.GetAddress (0), 9);
  echoClientC1.SetAttribute ("MaxPackets", UintegerValue (1));
  echoClientC1.SetAttribute ("Interval", TimeValue (Seconds (1.0)));
  echoClientC1.SetAttribute ("PacketSize", UintegerValue (256));

  UdpEchoClientHelper echoClientC2 (wifiInterfaces.GetAddress (1), 9);
  echoClientC2.SetAttribute ("MaxPackets", UintegerValue (1));
  echoClientC2.SetAttribute ("Interval", TimeValue (Seconds (1.0)));
  echoClientC2.SetAttribute ("PacketSize", UintegerValue (512));

  UdpEchoClientHelper echoClientC3 (csmaInterfaces.GetAddress (1), 9);
  echoClientC3.SetAttribute ("MaxPackets", UintegerValue (1));
  echoClientC3.SetAttribute ("Interval", TimeValue (Seconds (1.0)));
  echoClientC3.SetAttribute ("PacketSize", UintegerValue (1024));

  UdpEchoClientHelper echoClientC4 (csmaInterfaces.GetAddress (2), 9);
  echoClientC4.SetAttribute ("MaxPackets", UintegerValue (1));
  echoClientC4.SetAttribute ("Interval", TimeValue (Seconds (1.0)));
  echoClientC4.SetAttribute ("PacketSize", UintegerValue (2048));

  //Adicionando apps de cliente
  ApplicationContainer clientAppsC1 = echoClientC1.Install (csmaNodes.Get (0));
  clientAppsC1.Start (Seconds (2.0));
  clientAppsC1.Stop (Seconds (10.0));

  ApplicationContainer clientAppsC2 = echoClientC2.Install (csmaNodes.Get (1));
  clientAppsC2.Start (Seconds (2.0));
  clientAppsC2.Stop (Seconds (10.0));

  ApplicationContainer clientAppsC3 = echoClientC3.Install (wifiStaNodes.Get (0));
  clientAppsC3.Start (Seconds (2.0));
  clientAppsC3.Stop (Seconds (10.0));

  ApplicationContainer clientAppsC4 = echoClientC4.Install (wifiStaNodes.Get (1));
  clientAppsC4.Start (Seconds (2.0));
  clientAppsC4.Stop (Seconds (10.0));

  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  //NetAnim
  AnimationInterface anim ("redeC.xml");
  anim.SetConstantPosition (csmaNodes.Get(0), 4.0, 15.0); //C1
  anim.SetConstantPosition (csmaNodes.Get(1), 8.0, 15.0); //C2
  anim.SetConstantPosition (csmaNodes.Get(2), 12.0, 15.0); //S3
  anim.SetConstantPosition (csmaNodes.Get(3), 16.0, 15.0); //S4

  anim.SetConstantPosition (p2pNodes.Get(1), 10.0, 13.0); //AP

  // Flow monitor
  Ptr<FlowMonitor> flowMonitor;
  FlowMonitorHelper flowHelper;
  flowMonitor = flowHelper.InstallAll();

  Simulator::Stop (Seconds (10.0));
  Simulator::Run ();
  flowMonitor->SerializeToXmlFile("flowRedeC.xml", true, true);
  Simulator::Destroy ();
  return 0;
}
