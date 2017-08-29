/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2011-2012 University of California, Los Angeles
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
 * Author: Alexander Afanasyev <alexander.afanasyev@ucla.edu>
 */
// ndn-grid.cc
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/point-to-point-layout-module.h"
#include "ns3/ndnSIM-module.h"

// for ndn::L3RateTracer
#include <ns3/ndnSIM/utils/tracers/ndn-l3-rate-tracer.h>
#include <ns3/ndnSIM/utils/tracers/ndn-l3-aggregate-tracer.h>
#include <ns3/ndnSIM/utils/tracers/l2-rate-tracer.h>
#include <ns3/ndnSIM/utils/tracers/ndn-app-delay-tracer.h>

using namespace ns3;

/**
 * This scenario simulates a grid topology (using PointToPointGrid module)
 *
 * (consumer) -- ( ) ----- ( )
 *     |          |         |
 *    ( ) ------ ( ) ----- ( )
 *     |          |         |
 *    ( ) ------ ( ) -- (producer)
 *
 * All links are 1Mbps with propagation 10ms delay. 
 *
 * FIB is populated using NdnGlobalRoutingHelper.
 *
 * Consumer requests data from producer with frequency 100 interests per second
 * (interests contain constantly increasing sequence number).
 *
 * For every received interest, producer replies with a data packet, containing
 * 1024 bytes of virtual payload.
 *
 * To run scenario and see what is happening, use the following command:
 *
 *     NS_LOG=ndn.Consumer:ndn.Producer ./waf --run=ndn-grid
 */

int
main (int argc, char *argv[])
{
  // Setting default parameters for PointToPoint links and channels
  //Config::SetDefault ("ns3::PointToPointNetDevice::DataRate", StringValue ("1Mbps"));
  //Config::SetDefault ("ns3::PointToPointChannel::Delay", StringValue ("10ms"));
  //Config::SetDefault ("ns3::DropTailQueue::MaxPackets", StringValue ("10"));

  // Read optional command-line parameters (e.g., enable visualizer with ./waf --run=<> --visualize
  CommandLine cmd;
  cmd.Parse (argc, argv);

  // Creating 3x3 topology
  //PointToPointHelper p2p;
  //PointToPointGridHelper grid (3, 3, p2p);
  //grid.BoundingBox(100,100,200,200);
  
  AnnotatedTopologyReader topologyReader ("", 25);
  topologyReader.SetFileName ("src/ndnSIM/examples/topologies/topo-grid-huyao.txt");
  topologyReader.Read ();

  // Install CCNx stack on all nodes
  ndn::StackHelper ccnxHelper;
  ccnxHelper.SetForwardingStrategy ("ns3::ndn::fw::CustomStrategy");
  
  ccnxHelper.SetDefaultRoutes (true);
  ccnxHelper.SetContentStore ("ns3::ndn::cs::Lru",
                              "MaxSize", "1"); // ! Attention ! If set to 0, then MaxSize is infinite
  ccnxHelper.InstallAll ();

  // Installing global routing interface on all nodes
  //ndn::GlobalRoutingHelper ccnxGlobalRoutingHelper;
  //ccnxGlobalRoutingHelper.InstallAll ();

  // Getting containers for the consumer/producer
  //Ptr<Node> producer = grid.GetNode (2, 2);
  //NodeContainer consumerNodes;
  //consumerNodes.Add (grid.GetNode (0,0));
  
  Ptr<Node> consumer = Names::Find<Node> ("Node0");
  Ptr<Node> producer = Names::Find<Node> ("Node8");

  if (consumer == 0 || producer == 0)
    {
      NS_FATAL_ERROR ("Error in topology: one nodes c1 or p1 is missing");
    }

  // Install CCNx applications
  std::string prefix = "/data";

  ndn::AppHelper consumerHelper ("ns3::ndn::ConsumerCbr");
  consumerHelper.SetPrefix (prefix);
  consumerHelper.SetAttribute ("Frequency", StringValue ("100")); // 100 interests a second
  //consumerHelper.Install (consumer);

  //huyao130416
  ApplicationContainer consumers = consumerHelper.Install (consumer);
  consumers.Start (Seconds(0));
  consumers.Stop (Seconds(20));

  ndn::AppHelper producerHelper ("ns3::ndn::Producer");
  producerHelper.SetPrefix (prefix);
  producerHelper.SetAttribute ("PayloadSize", StringValue("1024"));
  producerHelper.Install (producer);

  // Add /prefix origins to ndn::GlobalRouter
  //ccnxGlobalRoutingHelper.AddOrigins (prefix, producer);

  // Calculate and install FIBs
  //ccnxGlobalRoutingHelper.CalculateRoutes ();

  Simulator::Stop (Seconds (20.0));
  
  boost::tuple< boost::shared_ptr<std::ostream>, std::list<Ptr<ndn::L3RateTracer> > >
  rateTracers = ndn::L3RateTracer::InstallAll ("rate-trace.txt", Seconds (19.99999999));
  
  boost::tuple< boost::shared_ptr<std::ostream>, std::list<Ptr<ndn::L3AggregateTracer> > >
  aggTracers = ndn::L3AggregateTracer::InstallAll ("aggregate-trace.txt", Seconds (19.99999999));
  
  boost::tuple< boost::shared_ptr<std::ostream>, std::list<Ptr<L2RateTracer> > >
  l2tracers = L2RateTracer::InstallAll ("drop-trace.txt", Seconds (19.99999999));
  
  boost::tuple< boost::shared_ptr<std::ostream>, std::list<Ptr<ndn::AppDelayTracer> > >
  tracers = ndn::AppDelayTracer::InstallAll ("app-delays-trace.txt");

  Simulator::Run ();
  Simulator::Destroy ();

  return 0;
}
