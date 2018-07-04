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
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/csma-module.h"
#include "ns3/applications-module.h"
#include "ns3/netanim-module.h"
#include "ns3/mobility-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("redeTesteB");
/*
// C                    10.1.2.0                    S
// n0   n1   n2   n3 -------------- n4   n5   n6   n7
// |    |    |    |  point-to-point  |    |    |    |
// ================                  ================
//  LAN1 10.1.1.0                      LAN2 10.1.3.0
*/
int
main (int argc, char *argv[])
{
  Time::SetResolution (Time::NS);
  LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
  LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);

  //Cria os nos p2p
  NodeContainer p2pnodes;
  p2pnodes.Create (2);
  //Cria os nos csma (lan)
  NodeContainer lan1;
  NodeContainer lan2;
  lan1.Add(p2pnodes.Get(0));
  lan2.Add(p2pnodes.Get(1));
  lan1.Create(3);
  lan2.Create(3);

  //Cria as conexoes com seus atributos
  PointToPointHelper pTp;
  pTp.SetDeviceAttribute ("DataRate", StringValue ("10Mbps"));
  pTp.SetChannelAttribute ("Delay", StringValue ("1ms"));
  CsmaHelper csma;
  csma.SetChannelAttribute ("DataRate", StringValue ("50Mbps"));
  csma.SetChannelAttribute ("Delay", TimeValue (NanoSeconds (6560)));

  //Adiciona os protocolos nos nós
  InternetStackHelper stack;
  stack.Install (lan1);
  stack.Install (lan2);

  //Cria os helpers para configurar as redes
  NetDeviceContainer p2pdevices;
  Ipv4AddressHelper address;
  Ipv4InterfaceContainer interfaces;

  //Configura os ips de cada rede (sao 3 redes diferentes)
  p2pdevices = pTp.Install (p2pnodes.Get(0),p2pnodes.Get(1));
  address.SetBase ("10.1.2.0", "255.255.255.0");
  interfaces = address.Assign (p2pdevices);
  //Configura a rede csma (lan) 
  NetDeviceContainer csmaDevices;
  Ipv4InterfaceContainer csmaInterfaces;
  csmaDevices = csma.Install (lan1);
  address.SetBase ("10.1.1.0", "255.255.255.0");
  csmaInterfaces = address.Assign (csmaDevices);
  csmaDevices = csma.Install (lan2);
  address.SetBase ("10.1.3.0", "255.255.255.0");
  csmaInterfaces = address.Assign (csmaDevices);

  //Popula a tabela de roteamento para as redes se comunicarem
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  //Seta a porta do servidor
  UdpEchoServerHelper echoServer (9);

  //Instala o servidor no ultimo no (lan2(3))
  ApplicationContainer serverApps = echoServer.Install (lan2.Get (3));
  serverApps.Start (Seconds (1.0));
  serverApps.Stop (Seconds (10.0));

  //Configura o cliente
  //Na hora de pegar o IP do servidor, usa 3 porque pega o node pela interface
  UdpEchoClientHelper echoClient (csmaInterfaces.GetAddress (3), 9);
  echoClient.SetAttribute ("MaxPackets", UintegerValue (10));
  echoClient.SetAttribute ("Interval", TimeValue (Seconds (1.0)));
  echoClient.SetAttribute ("PacketSize", UintegerValue (1024));

  //Instala o cliente no primeiro nó
  ApplicationContainer clientApps = echoClient.Install (lan1.Get (1));
  clientApps.Start (Seconds (2.0));
  clientApps.Stop (Seconds (10.0));

  //Netanim
  MobilityHelper mobility;
  mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
                                 "MinX", DoubleValue (0.0),
                                 "MinY", DoubleValue (0.0),
                                 "DeltaX", DoubleValue (5.0),
                                 "DeltaY", DoubleValue (10.0),
                                 "GridWidth", UintegerValue (3),
                                 "LayoutType", StringValue ("RowFirst"));
  mobility.Install (lan1);
  mobility.Install (lan2);

  AnimationInterface anim ("redeB.xml");
  //Lan1
	anim.SetConstantPosition (lan1.Get(0), 6.0, 5.0); //Primeiro no p2p
  anim.SetConstantPosition (lan1.Get(1), 0.0, 5.0); //Cliente
  anim.SetConstantPosition (lan1.Get(2), 2.0, 5.0);
  anim.SetConstantPosition (lan1.Get(3), 4.0, 5.0);
  //Lan2
  anim.SetConstantPosition (lan2.Get(0), 8.0, 5.0); //Segundo no p2p
  anim.SetConstantPosition (lan2.Get(1), 10.0, 5.0);
  anim.SetConstantPosition (lan2.Get(2), 12.0, 5.0);
  anim.SetConstantPosition (lan2.Get(3), 14.0, 5.0); //Servidor
  
  csma.EnablePcapAll("LAN");
  
  Simulator::Run ();
  Simulator::Destroy ();
  return 0;
}