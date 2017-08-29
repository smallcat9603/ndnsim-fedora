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

#include "ns3/point-to-point-module.h"


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

void FailLink (Ptr<NetDevice> nd, double value);
//void AddNode ();

int
main (int argc, char *argv[])
{
  CommandLine cmd;
  cmd.Parse (argc, argv);

  AnnotatedTopologyReader topologyReader ("", 1);
  topologyReader.SetFileName ("src/ndnSIM/examples/topologies/topo-application-huyao.txt");
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
  Ptr<Node> consumers[2] = { Names::Find<Node> ("E"), Names::Find<Node> ("R") };
  Ptr<Node> producers[8] = { Names::Find<Node> ("p1"), Names::Find<Node> ("p2"), Names::Find<Node> ("p3"), Names::Find<Node> ("p4"), Names::Find<Node> ("p5"), Names::Find<Node> ("p6"), Names::Find<Node> ("p7"), Names::Find<Node> ("p8") };

  ndn::AppHelper consumerHelper ("ns3::ndn::ConsumerBatches");
  consumerHelper.SetPrefix ("/data/p3");
  
  consumerHelper.SetAttribute ("Batches", StringValue ("1s 1 2s 1 3s 1 4s 1 6s 1 7s 1 8s 1 9s 1")); 
  consumerHelper.Install (consumers[0]);
  
  //consumerHelper.SetAttribute ("Batches", StringValue ("6s 1 7s 1 8s 1 9s 1")); 
  //consumerHelper.Install (consumers[1]);  
    
  //ApplicationContainer consumers = consumerHelper.Install (consumer);
  //consumers.Start (Seconds(0));
  //consumers.Stop (Seconds(15));
          
  ndn::AppHelper producerHelper ("ns3::ndn::Producer");

  for (int i = 0; i < 7; i++)
    {
          std::string prefix = "/data/"+Names::FindName (producers[i]);
          
          producerHelper.SetPrefix (prefix);  
          
          producerHelper.SetAttribute ("PayloadSize", StringValue("1024"));  
          producerHelper.Install (producers[i]);
      // when Start/Stop time is not specified, the application is running throughout the simulation
    }
    
  producerHelper.SetPrefix ("/data/p3");    
  producerHelper.SetAttribute ("PayloadSize", StringValue("1024"));  
  producerHelper.Install (producers[7]);

  // Calculate and install FIBs
  //ccnxGlobalRoutingHelper.CalculateRoutes ();

  Simulator::Stop (Seconds (10.0));

  boost::tuple< boost::shared_ptr<std::ostream>, std::list<Ptr<ndn::L3AggregateTracer> > >
  aggTracers = ndn::L3AggregateTracer::InstallAll ("aggregate-trace.txt", Seconds (0.01));

  boost::tuple< boost::shared_ptr<std::ostream>, std::list<Ptr<ndn::L3RateTracer> > >
  rateTracers = ndn::L3RateTracer::InstallAll ("rate-trace.txt", Seconds (0.01));
    
  boost::tuple< boost::shared_ptr<std::ostream>, std::list<Ptr<L2RateTracer> > >
  l2tracers = L2RateTracer::InstallAll ("drop-trace.txt", Seconds (0.01));
  
  boost::tuple< boost::shared_ptr<std::ostream>, std::list<Ptr<ndn::AppDelayTracer> > >
  tracers = ndn::AppDelayTracer::InstallAll ("app-delays-trace.txt");
  
  Ptr<Node> node1 = Names::Find<Node> ("N");
  Ptr<Node> node2 = Names::Find<Node> ("p3");
  
  Ptr<Node> node3 = Names::Find<Node> ("C");
  Ptr<Node> node4 = Names::Find<Node> ("p8");  
  
  Ptr<NetDevice> device1 = node1->GetDevice(4);
  Ptr<NetDevice> device2 = node2->GetDevice(0);
  
  Ptr<NetDevice> device3 = node3->GetDevice(5);
  Ptr<NetDevice> device4 = node4->GetDevice(0);  

  Simulator::Schedule (Seconds (5.0), FailLink, device1, 1.0);
  Simulator::Schedule (Seconds (5.0), FailLink, device2, 1.0);
  
  //Simulator::Schedule (Seconds (5.0), AddNode);
  
  Simulator::Schedule (Seconds (0.0), FailLink, device3, 1.0);
  Simulator::Schedule (Seconds (0.0), FailLink, device4, 1.0);  
  
  Simulator::Schedule (Seconds (5.0), FailLink, device3, 0.0);
  Simulator::Schedule (Seconds (5.0), FailLink, device4, 0.0);  
  
  Simulator::Run ();
  Simulator::Destroy ();

  return 0;
}

  void
  FailLink (Ptr<NetDevice> nd, double value)
  {
    Ptr<RateErrorModel> error = CreateObject<RateErrorModel> ();
    error->SetAttribute ("ErrorRate", DoubleValue (value));

    nd->SetAttribute ("ReceiveErrorModel", PointerValue (error));
  }
  
  /*void
  AddNode ()
  {
    // setting default parameters for PointToPoint links and channels
    Config::SetDefault ("ns3::PointToPointNetDevice::DataRate", StringValue ("10Mbps"));
    Config::SetDefault ("ns3::PointToPointChannel::Delay", StringValue ("10ms"));
    Config::SetDefault ("ns3::DropTailQueue::MaxPackets", StringValue ("100"));
    
    // Creating nodes
    NodeContainer nodes;
    nodes.Create (1);

    // Connecting nodes using two links
    PointToPointHelper p2p;
    p2p.Install (nodes.Get (0), Names::Find<Node> ("C"));
    
      // Install CCNx stack on all nodes
    ndn::StackHelper ccnxHelper;
    ccnxHelper.SetForwardingStrategy ("ns3::ndn::fw::CustomStrategy");
    ccnxHelper.SetDefaultRoutes (true);
    ccnxHelper.SetContentStore ("ns3::ndn::cs::Lru",
                              "MaxSize", "1"); // ! Attention ! If set to 0, then MaxSize is infinite
    ccnxHelper.Install (nodes.Get (0));
   
    ndn::AppHelper producerHelper ("ns3::ndn::Producer");
    producerHelper.SetPrefix ("/data/p3");
    producerHelper.SetAttribute ("PayloadSize", StringValue("1024"));
    producerHelper.Install (nodes.Get (0));
  }*/
