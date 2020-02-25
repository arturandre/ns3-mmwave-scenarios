/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
*   Copyright (c) 2011 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
*   Copyright (c) 2015, NYU WIRELESS, Tandon School of Engineering, New York University
*
*   This program is free software; you can redistribute it and/or modify
*   it under the terms of the GNU General Public License version 2 as
*   published by the Free Software Foundation;
*
*   This program is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with this program; if not, write to the Free Software
*   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*
*   Author: Marco Miozzo <marco.miozzo@cttc.es>
*           Nicola Baldo  <nbaldo@cttc.es>
*
*   Modified by: Marco Mezzavilla < mezzavilla@nyu.edu>
*                         Sourjya Dutta <sdutta@nyu.edu>
*                         Russell Ford <russell.ford@nyu.edu>
*                         Menglei Zhang <menglei@nyu.edu>
*/


/* scenario7.cc
 * Modified by: Artur André A. M. Oliveira <arturao@ime.usp.br>
 * Based on the files:
 * src/mmwave/examples/mmwave-tcp-building-example.cc
 * */


#include "ns3/point-to-point-module.h"
#include "ns3/mmwave-helper.h"
#include "ns3/epc-helper.h"
#include "ns3/mmwave-point-to-point-epc-helper.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/applications-module.h"
#include "ns3/point-to-point-helper.h"
#include "ns3/config-store.h"
#include <ns3/buildings-helper.h>
#include <ns3/buildings-module.h>
#include <ns3/packet.h>
#include <ns3/tag.h>
#include <ns3/queue-size.h>

using namespace ns3;
using namespace mmwave;

NS_LOG_COMPONENT_DEFINE ("mmWaveTCPExample");

class MyAppTag : public Tag
{
public:
  MyAppTag ()
  {
  }

  MyAppTag (Time sendTs) : m_sendTs (sendTs)
  {
  }

  static TypeId GetTypeId (void)
  {
    static TypeId tid = TypeId ("ns3::MyAppTag")
      .SetParent<Tag> ()
      .AddConstructor<MyAppTag> ();
    return tid;
  }

  virtual TypeId  GetInstanceTypeId (void) const
  {
    return GetTypeId ();
  }

  virtual void  Serialize (TagBuffer i) const
  {
    i.WriteU64 (m_sendTs.GetNanoSeconds ());
  }

  virtual void  Deserialize (TagBuffer i)
  {
    m_sendTs = NanoSeconds (i.ReadU64 ());
  }

  virtual uint32_t  GetSerializedSize () const
  {
    return sizeof (m_sendTs);
  }

  virtual void Print (std::ostream &os) const
  {
    std::cout << m_sendTs;
  }

  Time m_sendTs;
};


class MyApp : public Application
{
public:
  MyApp ();
  virtual ~MyApp ();
  void ChangeDataRate (DataRate rate);
  void Setup (Ptr<Socket> socket, Address address, uint32_t packetSize, uint32_t nPackets, DataRate dataRate);



private:
  virtual void StartApplication (void);
  virtual void StopApplication (void);

  void ScheduleTx (void);
  void SendPacket (void);

  Ptr<Socket>     m_socket;
  Address         m_peer;
  uint32_t        m_packetSize;
  uint32_t        m_nPackets;
  DataRate        m_dataRate;
  EventId         m_sendEvent;
  bool            m_running;
  uint32_t        m_packetsSent;
};

MyApp::MyApp ()
  : m_socket (0),
    m_peer (),
    m_packetSize (0),
    m_nPackets (0),
    m_dataRate (0),
    m_sendEvent (),
    m_running (false),
    m_packetsSent (0)
{
}

MyApp::~MyApp ()
{
  m_socket = 0;
}

void
MyApp::Setup (Ptr<Socket> socket, Address address, uint32_t packetSize, uint32_t nPackets, DataRate dataRate)
{
  m_socket = socket;
  m_peer = address;
  m_packetSize = packetSize;
  m_nPackets = nPackets;
  m_dataRate = dataRate;
}

void
MyApp::ChangeDataRate (DataRate rate)
{
  m_dataRate = rate;
}

void
MyApp::StartApplication (void)
{
  m_running = true;
  m_packetsSent = 0;
  m_socket->Bind ();
  m_socket->Connect (m_peer);
  SendPacket ();
}

void
MyApp::StopApplication (void)
{
  m_running = false;

  if (m_sendEvent.IsRunning ())
    {
      Simulator::Cancel (m_sendEvent);
    }

  if (m_socket)
    {
      m_socket->Close ();
    }
}

void
MyApp::SendPacket (void)
{
  Ptr<Packet> packet = Create<Packet> (m_packetSize);
  MyAppTag tag (Simulator::Now ());

  m_socket->Send (packet);
  if (++m_packetsSent < m_nPackets)
    {
      ScheduleTx ();
    }
}



void
MyApp::ScheduleTx (void)
{
  if (m_running)
    {
      Time tNext (Seconds (m_packetSize * 8 / static_cast<double> (m_dataRate.GetBitRate ())));
      m_sendEvent = Simulator::Schedule (tNext, &MyApp::SendPacket, this);
    }
}

static void
CwndChange (Ptr<OutputStreamWrapper> stream, uint32_t oldCwnd, uint32_t newCwnd)
{
  *stream->GetStream () << Simulator::Now ().GetSeconds () << "\t" << oldCwnd << "\t" << newCwnd << std::endl;
}


static void
RttChange (Ptr<OutputStreamWrapper> stream, Time oldRtt, Time newRtt)
{
  *stream->GetStream () << Simulator::Now ().GetSeconds () << "\t" << oldRtt.GetSeconds () << "\t" << newRtt.GetSeconds () << std::endl;
}



static void Rx (Ptr<OutputStreamWrapper> stream, Ptr<const Packet> packet, const Address &from)
{
  *stream->GetStream () << Simulator::Now ().GetSeconds () << "\t" << packet->GetSize () << std::endl;
}

void
ChangeSpeed (Ptr<Node>  n, Vector speed)
{
  n->GetObject<ConstantVelocityMobilityModel> ()->SetVelocity (speed);
}



int
main (int argc, char *argv[])
{
  /*
   * scenario 1: 1 building;
   * scenario 2: 3 building;
   * scenario 3: 6 random located small building, simulate tree and human blockage.
   * */
  int scenario = 2;
  double stopTime = 25;
  double simStopTime = 25;
  bool harqEnabled = true;
  bool rlcAmEnabled = true;
  bool tcp = true;

  CommandLine cmd;

  cmd.AddValue ("simTime", "Total duration of the simulation [s])", simStopTime);

  cmd.AddValue ("harq", "Enable Hybrid ARQ", harqEnabled);
  cmd.AddValue ("rlcAm", "Enable RLC-AM", rlcAmEnabled);
  cmd.Parse (argc, argv);

  Config::SetDefault ("ns3::TcpSocket::SegmentSize", UintegerValue (1400));

  Config::SetDefault ("ns3::LteRlcUm::MaxTxBufferSize", UintegerValue (1024 * 1024));
  Config::SetDefault ("ns3::LteRlcUmLowLat::MaxTxBufferSize", UintegerValue (1024 * 1024));
  Config::SetDefault ("ns3::TcpSocket::SndBufSize", UintegerValue (131072 * 50));
  Config::SetDefault ("ns3::TcpSocket::RcvBufSize", UintegerValue (131072 * 50));
  Config::SetDefault ("ns3::MmWavePhyMacCommon::ResourceBlockNum", UintegerValue (1));
  Config::SetDefault ("ns3::MmWavePhyMacCommon::ChunkPerRB", UintegerValue (72));
  Config::SetDefault ("ns3::MmWaveHelper::RlcAmEnabled", BooleanValue (rlcAmEnabled));
  Config::SetDefault ("ns3::MmWaveHelper::HarqEnabled", BooleanValue (harqEnabled));
  Config::SetDefault ("ns3::MmWaveFlexTtiMacScheduler::HarqEnabled", BooleanValue (true));
  Config::SetDefault ("ns3::MmWaveFlexTtiMaxWeightMacScheduler::HarqEnabled", BooleanValue (true));
  Config::SetDefault ("ns3::MmWaveFlexTtiMacScheduler::HarqEnabled", BooleanValue (true));
  Config::SetDefault ("ns3::MmWaveBeamforming::LongTermUpdatePeriod", TimeValue (MilliSeconds (100.0)));
  Config::SetDefault ("ns3::LteRlcAm::PollRetransmitTimer", TimeValue (MilliSeconds (4.0)));
  Config::SetDefault ("ns3::LteRlcAm::ReorderingTimer", TimeValue (MilliSeconds (2.0)));
  Config::SetDefault ("ns3::LteRlcAm::StatusProhibitTimer", TimeValue (MilliSeconds (1.0)));
  Config::SetDefault ("ns3::LteRlcAm::ReportBufferStatusTimer", TimeValue (MilliSeconds (4.0)));
  Config::SetDefault ("ns3::LteRlcAm::MaxTxBufferSize", UintegerValue (20 * 1024 * 1024));


  Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue (TcpNewReno::GetTypeId ()));

  Ptr<MmWaveHelper> mmwaveHelper = CreateObject<MmWaveHelper> ();

  mmwaveHelper->SetAttribute ("PathlossModel", StringValue ("ns3::BuildingsObstaclePropagationLossModel"));
  mmwaveHelper->Initialize ();
  mmwaveHelper->SetHarqEnabled (true);

  Ptr<MmWavePointToPointEpcHelper>  epcHelper = CreateObject<MmWavePointToPointEpcHelper> ();
  mmwaveHelper->SetEpcHelper (epcHelper);

  Ptr<Node> pgw = epcHelper->GetPgwNode ();

  // Create a single RemoteHost
  NodeContainer remoteHostContainer;
  remoteHostContainer.Create (1);
  Ptr<Node> remoteHost = remoteHostContainer.Get (0);
  InternetStackHelper internet;
  internet.Install (remoteHostContainer);

  // Create the Internet
  PointToPointHelper p2ph;
  p2ph.SetDeviceAttribute ("DataRate", DataRateValue (DataRate ("100Gb/s")));
  p2ph.SetDeviceAttribute ("Mtu", UintegerValue (1500));
  p2ph.SetChannelAttribute ("Delay", TimeValue (Seconds (0.010)));
  NetDeviceContainer internetDevices = p2ph.Install (pgw, remoteHost);
  Ipv4AddressHelper ipv4h;
  ipv4h.SetBase ("1.0.0.0", "255.0.0.0");
  Ipv4InterfaceContainer internetIpIfaces = ipv4h.Assign (internetDevices);
  // interface 0 is localhost, 1 is the p2p device
  Ipv4Address remoteHostAddr;
  remoteHostAddr = internetIpIfaces.GetAddress (1);
  Ipv4StaticRoutingHelper ipv4RoutingHelper;
  Ptr<Ipv4StaticRouting> remoteHostStaticRouting = ipv4RoutingHelper.GetStaticRouting (remoteHost->GetObject<Ipv4> ());
  remoteHostStaticRouting->AddNetworkRouteTo (Ipv4Address ("7.0.0.0"), Ipv4Mask ("255.0.0.0"), 1);



  switch (scenario)
    {
    case 1:
      {
        Ptr < Building > building;
        building = Create<Building> ();
        building->SetBoundaries (Box (40.0,60.0,
                                      0.0, 6,
                                      0.0, 15.0));
        break;
      }
    case 2:
      {
        Ptr < Building > building1;
        building1 = Create<Building> ();
        //Sesc building
        building1->SetBoundaries(Box(0.0, 0.0, 0.0, 15.66, 34.87, 58.85));

        //Itau Cultural building
        Ptr < Building > building2;
        building2 = Create<Building>();
        building2->SetBoundaries(Box(39.03, 0.0, 0.0, 13.93, 33.68, 58.9));
        break;
      }
    case 3:
      {
        Ptr < Building > building1;
        building1 = Create<Building> ();
        building1->SetBoundaries (Box (69.5,70.0,
                                       4.5, 5.0,
                                       0.0, 1.5));

        Ptr < Building > building2;
        building2 = Create<Building> ();
        building2->SetBoundaries (Box (60.0,60.5,
                                       9.5, 10.0,
                                       0.0, 1.5));

        Ptr < Building > building3;
        building3 = Create<Building> ();
        building3->SetBoundaries (Box (54.0,54.5,
                                       5.5, 6.0,
                                       0.0, 1.5));
        Ptr < Building > building4;
        building1 = Create<Building> ();
        building1->SetBoundaries (Box (60.0,60.5,
                                       6.0, 6.5,
                                       0.0, 1.5));

        Ptr < Building > building5;
        building2 = Create<Building> ();
        building2->SetBoundaries (Box (70.0,70.5,
                                       0.0, 0.5,
                                       0.0, 1.5));

        Ptr < Building > building6;
        building3 = Create<Building> ();
        building3->SetBoundaries (Box (50.0,50.5,
                                       4.0, 4.5,
                                       0.0, 1.5));
        break;
      }
    default:
      {
        NS_FATAL_ERROR ("Invalid scenario");
      }
    }


  NodeContainer ueNodes;
  NodeContainer enbNodes;
  enbNodes.Create (2);
  ueNodes.Create (2);

  Ptr<ListPositionAllocator> enbPositionAlloc = CreateObject<ListPositionAllocator> ();
  enbPositionAlloc->Add (Vector (0.0, -10.0, 3.0));
  enbPositionAlloc->Add (Vector (40.0, -10.0, 3.0));
  MobilityHelper enbmobility;
  enbmobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  enbmobility.SetPositionAllocator (enbPositionAlloc);
  enbmobility.Install (enbNodes);
  BuildingsHelper::Install (enbNodes);
  MobilityHelper uemobility;
  uemobility.SetMobilityModel ("ns3::ConstantVelocityMobilityModel");
  uemobility.Install (ueNodes);

  ueNodes.Get (0)->GetObject<MobilityModel> ()->SetPosition (Vector (-20, 40, 1));
  ueNodes.Get (1)->GetObject<MobilityModel> ()->SetPosition (Vector (60, 40, 1));
  ueNodes.Get (0)->GetObject<ConstantVelocityMobilityModel> ()->SetVelocity (Vector (0, 0, 0));
  ueNodes.Get (1)->GetObject<ConstantVelocityMobilityModel> ()->SetVelocity (Vector (0, 0, 0));

  for (int i = 2; i < 22; i++)
  {
    Simulator::Schedule (Seconds (i), &ChangeSpeed, ueNodes.Get (0), Vector (4, 0, 0));
    Simulator::Schedule (Seconds (i), &ChangeSpeed, ueNodes.Get (1), Vector (-4, 0, 0));
  }

  Simulator::Schedule (Seconds (22), &ChangeSpeed, ueNodes.Get (0), Vector (0, 0, 0));
  Simulator::Schedule (Seconds (22), &ChangeSpeed, ueNodes.Get (1), Vector (0, 0, 0));

  BuildingsHelper::Install (ueNodes);

  // Install LTE Devices to the nodes
  NetDeviceContainer enbDevs = mmwaveHelper->InstallEnbDevice (enbNodes);
  NetDeviceContainer ueDevs = mmwaveHelper->InstallUeDevice (ueNodes);

  // Install the IP stack on the UEs
  // Assign IP address to UEs, and install applications
  internet.Install (ueNodes);
  Ipv4InterfaceContainer ueIpIface;
  ueIpIface = epcHelper->AssignUeIpv4Address (NetDeviceContainer (ueDevs));

  mmwaveHelper->AttachToClosestEnb (ueDevs, enbDevs);
  mmwaveHelper->EnableTraces ();

  // Set the default gateway for the UE
  Ptr<Node> ueNode0 = ueNodes.Get (0);
  Ptr<Node> ueNode1 = ueNodes.Get (1);

  Ptr<Ipv4StaticRouting> ueStaticRouting1 = ipv4RoutingHelper.GetStaticRouting (ueNode0->GetObject<Ipv4> ());
  Ptr<Ipv4StaticRouting> ueStaticRouting2 = ipv4RoutingHelper.GetStaticRouting (ueNode1->GetObject<Ipv4> ());
  ueStaticRouting1->SetDefaultRoute (epcHelper->GetUeDefaultGatewayAddress (), 1);
  ueStaticRouting2->SetDefaultRoute (epcHelper->GetUeDefaultGatewayAddress (), 1);

  if (tcp)
    {
      // Install and start applications on UEs and remote host
      uint16_t sinkPort = 20000;

      Address sinkAddress0 (InetSocketAddress (ueIpIface.GetAddress (0), sinkPort));
      Address sinkAddress1 (InetSocketAddress (ueIpIface.GetAddress (1), sinkPort));
      PacketSinkHelper packetSinkHelper ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), sinkPort));
      ApplicationContainer sinkApps0 = packetSinkHelper.Install (ueNodes.Get (0));
      ApplicationContainer sinkApps1 = packetSinkHelper.Install (ueNodes.Get (1));

      sinkApps0.Start (Seconds (0.));
      sinkApps1.Start (Seconds (0.));
      sinkApps0.Stop (Seconds (simStopTime));
      sinkApps1.Stop (Seconds (simStopTime));

      Ptr<Socket> ns3TcpSocket0 = Socket::CreateSocket (remoteHostContainer.Get (0), TcpSocketFactory::GetTypeId ());
      Ptr<Socket> ns3TcpSocket1 = Socket::CreateSocket (remoteHostContainer.Get (0), TcpSocketFactory::GetTypeId ());
      Ptr<MyApp> app0 = CreateObject<MyApp> ();
      Ptr<MyApp> app1 = CreateObject<MyApp> ();
      app0->Setup (ns3TcpSocket0, sinkAddress0, 1400, 5000000, DataRate ("1000Mb/s"));
      app1->Setup (ns3TcpSocket1, sinkAddress1, 1400, 5000000, DataRate ("1000Mb/s"));

      remoteHostContainer.Get (0)->AddApplication (app0);
      remoteHostContainer.Get (0)->AddApplication (app1);
      AsciiTraceHelper asciiTraceHelper;
      Ptr<OutputStreamWrapper> stream10 = asciiTraceHelper.CreateFileStream ("mmWave-tcp-window-newreno0.txt");
      Ptr<OutputStreamWrapper> stream11 = asciiTraceHelper.CreateFileStream ("mmWave-tcp-window-newreno1.txt");
      ns3TcpSocket0->TraceConnectWithoutContext ("CongestionWindow", MakeBoundCallback (&CwndChange, stream10));
      ns3TcpSocket1->TraceConnectWithoutContext ("CongestionWindow", MakeBoundCallback (&CwndChange, stream11));

      Ptr<OutputStreamWrapper> stream40 = asciiTraceHelper.CreateFileStream ("mmWave-tcp-rtt-newreno0.txt");
      Ptr<OutputStreamWrapper> stream41 = asciiTraceHelper.CreateFileStream ("mmWave-tcp-rtt-newreno1.txt");
      ns3TcpSocket0->TraceConnectWithoutContext ("RTT", MakeBoundCallback (&RttChange, stream40));
      ns3TcpSocket1->TraceConnectWithoutContext ("RTT", MakeBoundCallback (&RttChange, stream41));

      Ptr<OutputStreamWrapper> stream20 = asciiTraceHelper.CreateFileStream ("mmWave-tcp-data-newreno0.txt");
      Ptr<OutputStreamWrapper> stream21 = asciiTraceHelper.CreateFileStream ("mmWave-tcp-data-newreno1.txt");
      sinkApps0.Get (0)->TraceConnectWithoutContext ("Rx",MakeBoundCallback (&Rx, stream20));
      sinkApps1.Get (0)->TraceConnectWithoutContext ("Rx",MakeBoundCallback (&Rx, stream21));

      app0->SetStartTime (Seconds (0.1));
      app1->SetStartTime (Seconds (0.1));
      app0->SetStopTime (Seconds (stopTime));
      app1->SetStopTime (Seconds (stopTime));
    }
  else
    {
      // Install and start applications on UEs and remote host
      uint16_t sinkPort = 20000;

      Address sinkAddress (InetSocketAddress (ueIpIface.GetAddress (0), sinkPort));
      PacketSinkHelper packetSinkHelper ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), sinkPort));
      ApplicationContainer sinkApps = packetSinkHelper.Install (ueNodes.Get (0));

      sinkApps.Start (Seconds (0.));
      sinkApps.Stop (Seconds (simStopTime));

      Ptr<Socket> ns3UdpSocket = Socket::CreateSocket (remoteHostContainer.Get (0), UdpSocketFactory::GetTypeId ());
      Ptr<MyApp> app = CreateObject<MyApp> ();
      app->Setup (ns3UdpSocket, sinkAddress, 1400, 5000000, DataRate ("1000Mb/s"));

      remoteHostContainer.Get (0)->AddApplication (app);
      AsciiTraceHelper asciiTraceHelper;
      Ptr<OutputStreamWrapper> stream2 = asciiTraceHelper.CreateFileStream ("mmWave-udp-data-am.txt");
      sinkApps.Get (0)->TraceConnectWithoutContext ("Rx",MakeBoundCallback (&Rx, stream2));

      app->SetStartTime (Seconds (0.1));
      app->SetStopTime (Seconds (stopTime));

    }

  BuildingsHelper::MakeMobilityModelConsistent ();
  Config::Set ("/NodeList/*/DeviceList/*/TxQueue/MaxSize", QueueSizeValue (QueueSize ("1000000p")));

  Simulator::Stop (Seconds (simStopTime));
  Simulator::Run ();
  Simulator::Destroy ();

  return 0;

}