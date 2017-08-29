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

// ndn-tree-tracers.cc

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/ndnSIM-module.h"

// for ndn::L3AggregateTracer
#include <ns3/ndnSIM/utils/tracers/ndn-l3-aggregate-tracer.h>

// for ndn::L3RateTracer
#include <ns3/ndnSIM/utils/tracers/ndn-l3-rate-tracer.h>

#include <ns3/ndnSIM/utils/tracers/l2-rate-tracer.h>
#include <ns3/ndnSIM/utils/tracers/ndn-app-delay-tracer.h>

using namespace ns3;

/**
 * This scenario simulates a tree topology (using topology reader module)
 *
 *    /------\      /------\      /------\      /------\
 *    |leaf-1|      |leaf-2|      |leaf-3|      |leaf-4|
 *    \------/      \------/      \------/      \------/
 *         ^          ^                ^           ^	
 *         |          |                |           |
 *    	    \        /                  \         / 
 *           \      /  			 \  	 /    10Mbps / 1ms
 *            \    /  			  \ 	/
 *             |  |  			   |   | 
 *    	       v  v                        v   v     
 *          /-------\                    /-------\
 *          | rtr-1 |                    | rtr-2 |
 *          \-------/                    \-------/
 *                ^                        ^                      
 *      	  |	 		   |
 *      	   \			  /  10 Mpbs / 1ms 
 *      	    +--------+  +--------+ 
 *      		     |  |      
 *                           v  v
 *      		  /--------\
 *      		  |  root  |
 *                        \--------/
 *
 *
 * To run scenario and see what is happening, use the following command:
 *
 *     ./waf --run=ndn-tree-tracers
 */

void FailLink (Ptr<NetDevice> nd);

int
main (int argc, char *argv[])
{
  CommandLine cmd;
  cmd.Parse (argc, argv);

  AnnotatedTopologyReader topologyReader ("", 1);
  topologyReader.SetFileName ("src/ndnSIM/examples/topologies/topo-linkfailure-huyao.txt");
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
  Ptr<Node> consumer = Names::Find<Node> ("c");
  Ptr<Node> producer = Names::Find<Node> ("p");


  ndn::AppHelper consumerHelper ("ns3::ndn::ConsumerCbr");
  consumerHelper.SetAttribute ("Frequency", StringValue ("10")); // 100 interests a second

  // Each consumer will express unique interests /root/<leaf-name>/<seq-no>
  consumerHelper.SetPrefix ("/data");
  //consumerHelper.Install (consumer);
    
  //huyao130424
  ApplicationContainer consumers = consumerHelper.Install (consumer);
  consumers.Start (Seconds(0));
  consumers.Stop (Seconds(15));
          
  ndn::AppHelper producerHelper ("ns3::ndn::Producer");
  producerHelper.SetAttribute ("PayloadSize", StringValue("1024"));  

  // Register /root prefix with global routing controller and
  // install producer that will satisfy Interests in /root namespace
  //ccnxGlobalRoutingHelper.AddOrigins ("/root", producer);
  producerHelper.SetPrefix ("/data");
  producerHelper.Install (producer);

  // Calculate and install FIBs
  //ccnxGlobalRoutingHelper.CalculateRoutes ();

  Simulator::Stop (Seconds (15.0));

  boost::tuple< boost::shared_ptr<std::ostream>, std::list<Ptr<ndn::L3AggregateTracer> > >
  aggTracers = ndn::L3AggregateTracer::InstallAll ("aggregate-trace.txt", Seconds (0.01));

  boost::tuple< boost::shared_ptr<std::ostream>, std::list<Ptr<ndn::L3RateTracer> > >
  rateTracers = ndn::L3RateTracer::InstallAll ("rate-trace.txt", Seconds (0.01));
    
  boost::tuple< boost::shared_ptr<std::ostream>, std::list<Ptr<L2RateTracer> > >
  l2tracers = L2RateTracer::InstallAll ("drop-trace.txt", Seconds (0.01));
  
  boost::tuple< boost::shared_ptr<std::ostream>, std::list<Ptr<ndn::AppDelayTracer> > >
  tracers = ndn::AppDelayTracer::InstallAll ("app-delays-trace.txt");
  
  Ptr<Node> node1 = Names::Find<Node> ("r7");
  Ptr<Node> node2 = Names::Find<Node> ("p");
  
  Ptr<NetDevice> device1 = node1->GetDevice(1);
  Ptr<NetDevice> device2 = node2->GetDevice(0);

  Simulator::Schedule (Seconds (5.0), FailLink, device1);
  Simulator::Schedule (Seconds (5.0), FailLink, device2);
  
  Simulator::Run ();
  Simulator::Destroy ();

  return 0;
}

  void
  FailLink (Ptr<NetDevice> nd)
  {
    Ptr<RateErrorModel> error = CreateObject<RateErrorModel> ();
    error->SetAttribute ("ErrorRate", DoubleValue (1.0));

    nd->SetAttribute ("ReceiveErrorModel", PointerValue (error));
  }
