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


/* scenario1.cc
 * Modified by: Artur André A. M. Oliveira <arturao@ime.usp.br>
 * Based on the file src/mmwave/examples/mmwave-example.cc
 * */

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/config-store.h"
#include "ns3/mmwave-helper.h"
#include <ns3/buildings-helper.h>
#include "ns3/global-route-manager.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/log.h"

using namespace ns3;
using namespace mmwave;

int
main (int argc, char *argv[])
{

  Config::SetDefault ("ns3::MmWavePhyMacCommon::ResourceBlockNum", UintegerValue (1));
  Config::SetDefault ("ns3::MmWavePhyMacCommon::ChunkPerRB", UintegerValue (72));
  Config::SetDefault ("ns3::MmWave3gppPropagationLossModel::Shadowing",
		  BooleanValue(false)); // enable or disable the shadowing effect
  CommandLine cmd;
  cmd.Parse (argc, argv);

  /* Information regarding the traces generated:
   *
   * 1. UE_1_SINR.txt : Gives the SINR for each sub-band
   *    Subframe no.  | Slot No. | Sub-band  | SINR (db)
   *
   * 2. UE_1_Tb_size.txt : Allocated transport block size
   *    Time (micro-sec)  |  Tb-size in bytes
   * */

  Ptr<MmWaveHelper> ptr_mmWave = CreateObject<MmWaveHelper> ();

  ptr_mmWave->Initialize ();

  /* A configuration example. */
  int ueNodesNum1 = 40;

  int enbNodesNum = 5;
  NodeContainer enbNodes;
  NodeContainer ueNodes1;

  enbNodes.Create(enbNodesNum);
  ueNodes1.Create (ueNodesNum1);

  Ptr<ListPositionAllocator> enbPositionAlloc = CreateObject<ListPositionAllocator> ();
 
  enbPositionAlloc->Add (Vector (0,0,3));

  MobilityHelper enbmobility;
  enbmobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  enbmobility.SetPositionAllocator (enbPositionAlloc);
  enbmobility.Install (enbNodes);
  BuildingsHelper::Install (enbNodes);


  MobilityHelper uemobility1;

  Ptr<ListPositionAllocator> uePositionAlloc1 = CreateObject<ListPositionAllocator> ();

  uePositionAlloc1->Add (Vector (15,0,3));

  uemobility1.SetMobilityModel ("ns3::ConstantPositionMobilityModel");

  uemobility1.SetPositionAllocator (uePositionAlloc1);

  uemobility1.Install (ueNodes1);

  BuildingsHelper::Install (ueNodes1);

  NetDeviceContainer enbNetDev = ptr_mmWave->InstallEnbDevice (enbNodes);
  NetDeviceContainer ueNetDev1 = ptr_mmWave->InstallUeDevice (ueNodes1);

  ptr_mmWave->AttachToClosestEnb (ueNetDev1, enbNetDev);

  ptr_mmWave->EnableTraces ();

  // Activate a data radio bearer
  enum EpsBearer::Qci q = EpsBearer::GBR_CONV_VOICE;
  EpsBearer bearer (q);
  ptr_mmWave->ActivateDataRadioBearer (ueNetDev1, bearer);

  Simulator::Stop (Seconds (10));
  Simulator::Run ();
  Simulator::Destroy ();
  return 0;
}
