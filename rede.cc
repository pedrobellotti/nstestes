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
#include "ns3/applications-module.h"
#include "ns3/netanim-module.h"
#include "ns3/mobility-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("redeTeste");
/*
* C     pTp1             pTp2           pTp3      S
* n0 ------------ n1 ------------ n2 ------------ n3
*     5Mbps 2ms       10Mbps 1ms      1Mbps 5ms
*     10.1.1.0         10.1.2.0         10.1.3.0
*/
int
main (int argc, char *argv[])
{
  Time::SetResolution (Time::NS);
  LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
  LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);

  //Cria os 4 nos
  NodeContainer nodes;
  nodes.Create (4);

  //Cria as conexoes com seus atributos
  PointToPointHelper pTp1;
  pTp1.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
  pTp1.SetChannelAttribute ("Delay", StringValue ("2ms"));

  PointToPointHelper pTp2;
  pTp2.SetDeviceAttribute ("DataRate", StringValue ("10Mbps"));
  pTp2.SetChannelAttribute ("Delay", StringValue ("1ms"));

  PointToPointHelper pTp3;
  pTp3.SetDeviceAttribute ("DataRate", StringValue ("1Mbps"));
  pTp3.SetChannelAttribute ("Delay", StringValue ("5ms"));

  //Adiciona os protocolos nos nós
  InternetStackHelper stack;
  stack.Install (nodes);

  //Cria os helpers para configurar as redes
  NetDeviceContainer devices;
  Ipv4AddressHelper address;
  Ipv4InterfaceContainer interfaces;

  //Configura os ips de cada rede (sao 3 redes diferentes)
  devices = pTp1.Install (nodes.Get(0),nodes.Get(1));
  address.SetBase ("10.1.1.0", "255.255.255.0");
  interfaces = address.Assign (devices);

  devices = pTp2.Install (nodes.Get(1),nodes.Get(2));
  address.SetBase ("10.1.2.0", "255.255.255.0");
  interfaces = address.Assign (devices);

  devices = pTp3.Install (nodes.Get(2),nodes.Get(3));
  address.SetBase ("10.1.3.0", "255.255.255.0");
  interfaces = address.Assign (devices);

  //Popula a tabela de roteamento para as redes se comunicarem
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  //Seta a porta do servidor
  UdpEchoServerHelper echoServer (9);

  //Instala o servidor no ultimo no (3)
  ApplicationContainer serverApps = echoServer.Install (nodes.Get (3));
  serverApps.Start (Seconds (1.0));
  serverApps.Stop (Seconds (10.0));

  //Configura o cliente
  //Na hora de pegar o IP do servidor, usa 1 porque pega o node pela interface (que so tem 2 nos)
  UdpEchoClientHelper echoClient (interfaces.GetAddress (1), 9);
  echoClient.SetAttribute ("MaxPackets", UintegerValue (1));
  echoClient.SetAttribute ("Interval", TimeValue (Seconds (1.0)));
  echoClient.SetAttribute ("PacketSize", UintegerValue (1024));

  //Instala o cliente no primeiro nó
  ApplicationContainer clientApps = echoClient.Install (nodes.Get (0));
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
  mobility.Install (nodes);

  AnimationInterface anim ("rede.xml");
	anim.SetConstantPosition (nodes.Get(0), 0.0, 1.0);
  anim.SetConstantPosition (nodes.Get(1), 2.0, 3.0);
  anim.SetConstantPosition (nodes.Get(2), 4.0, 5.0);
  anim.SetConstantPosition (nodes.Get(3), 6.0, 7.0);

  pTp1.EnablePcapAll ("rede1");
  //pTp2.EnablePcapAll ("rede2");
  //pTp3.EnablePcapAll ("rede3");
  
  Simulator::Run ();
  Simulator::Destroy ();
  return 0;
}