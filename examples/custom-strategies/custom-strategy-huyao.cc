/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
// custom-strategy.cc

#include "custom-strategy.h"
#include "ns3/ndn-fib.h"
#include "ns3/ndn-fib-entry.h"
#include "ns3/ndn-pit-entry.h"
#include "ns3/ndn-interest.h"

//huyao 130317
#include "ns3/random-variable.h"
#include "ns3/ndn-forwarding-strategy.h"
#include "ns3/ndn-l3-protocol.h"
//huyao 130502
#include "ns3/core-module.h"
#include "ns3/ndn-app-face.h"

#include <boost/ref.hpp>
#include <boost/foreach.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/lambda/bind.hpp>
namespace ll = boost::lambda;

namespace ns3 {
namespace ndn {
namespace fw {

NS_OBJECT_ENSURE_REGISTERED(CustomStrategy);

LogComponent CustomStrategy::g_log = LogComponent (CustomStrategy::GetLogName ().c_str ());

std::string
CustomStrategy::GetLogName ()
{
  return "ndn.fw.CustomStrategy";
}

TypeId
CustomStrategy::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::ndn::fw::CustomStrategy")
    .SetGroupName ("Ndn")
    .SetParent <BaseStrategy> ()
    .AddConstructor <CustomStrategy> ()

    // .AddAttribute ("Attribute", "Attribute spec",
    //                         StringValue ("DefaultValue"),
    //                         MakeStringAccessor (&BaseStrategy::m_variable),
    //                         MakeStringChecker ())
    ;
  return tid;
}

CustomStrategy::CustomStrategy ()
  : m_counter (0)
{
}

/*
bool
CustomStrategy::DoPropagateInterest (Ptr<Face> inFace,
                                     Ptr<const Interest> header,
                                     Ptr<const Packet> origPacket,
                                     Ptr<pit::Entry> pitEntry)
{
  typedef fib::FaceMetricContainer::type::index<fib::i_metric>::type FacesByMetric;
  FacesByMetric &faces = pitEntry->GetFibEntry ()->m_faces.get<fib::i_metric> ();
  FacesByMetric::iterator faceIterator = faces.begin ();

  int propagatedCount = 0;

  // forward to best-metric face
  if (faceIterator != faces.end ())
    {
      if (TrySendOutInterest (inFace, faceIterator->GetFace (), header, origPacket, pitEntry))
        propagatedCount ++;

      faceIterator ++;
    }

  // forward to second-best-metric face
  if (faceIterator != faces.end ())
    {
      if (TrySendOutInterest (inFace, faceIterator->GetFace (), header, origPacket, pitEntry))
        propagatedCount ++;

      faceIterator ++;
    }
  return propagatedCount > 0;
}
*/

bool
CustomStrategy::DoPropagateInterest (Ptr<Face> inFace,
                                     Ptr<const Interest> header,
                                     Ptr<const Packet> origPacket,
                                     Ptr<pit::Entry> pitEntry)
{
  typedef fib::FaceMetricContainer::type::index<fib::i_metric>::type FacesByMetric;
  FacesByMetric &faces = pitEntry->GetFibEntry ()->m_faces.get<fib::i_metric> ();
  FacesByMetric::iterator faceIterator = faces.begin ();

  int propagatedCount = 0;

  //huyao 130327  
  //UniformVariable var(1, 20);
  //int val_int = var.GetInteger(1, 100);
  srand((unsigned)clock());
  int val_int = rand()%100;
  
  //std::cout<<Simulator::GetContext ()<<": val_int: "<<val_int<<std::endl;
  //std::cout<<Simulator::GetContext ()<<": InterestLifetime: "<<header->GetInterestLifetime ()<<std::endl;

  //huyao 130510 不用data
  //Name name = Name ("/data");
  Name name = header->GetName ().cut (1);

  //if (pitEntry->GetFibEntry ()->m_prefix->GetLastComponent() == "") 和下面作用一样
  if (pitEntry->GetFibEntry ()->m_prefix->size() == 0) //默认prefix为“/”，flood
  {
      for (faceIterator = faces.begin(); faceIterator != faces.end(); faceIterator ++)
      {
        if (TrySendOutInterest (inFace, faceIterator->GetFace (), header, origPacket, pitEntry))
        propagatedCount ++;
      }
      std::cout<<Simulator::GetContext ()<<":flooding "<<std::endl;
  }
  else //非默认prefix（“/”）
  {
          if (faceIterator != faces.end ()) //faces不为空，实际不需要  //if (pitEntry->GetFibEntry () != NULL)
          {
              //int count = 0; //下面仿其他forwardstrategy
              //BOOST_FOREACH (const fib::FaceMetric &metricFace, pitEntry->GetFibEntry ()->m_faces.get<fib::i_metric> ())
              //{
              //  count++;
              //}

            //huyao130502 retransmitted?->flood
            //Ptr<Node> node = this->GetObject<Node> ();
            //std::string name= Names::FindName (node);
            //if (name.find("c") < name.length()) //consumer

            
                    /*bool isRetransmitted = m_detectRetransmissions && // a small guard
                                 DetectRetransmittedInterest (inFace, header, origPacket, pitEntry);
                    std::cout<<Simulator::GetContext ()<<": isRetransmitted : "<<isRetransmitted<<std::endl;
                    if (isRetransmitted)
                    {
                        Ptr<L3Protocol> ndn = this->GetObject<L3Protocol> ();
                        //包括不在fib中的所有face
                        for (uint32_t faceNum = 0; faceNum < ndn->GetNFaces (); faceNum++) 
                        {
                           Ptr<Face> outFace = ndn->GetFace (faceNum);
                           
                           if (DynamicCast<AppFace> (outFace)) //application face
                           continue;

                           if(TrySendOutInterest (inFace, outFace, header, origPacket, pitEntry))
                           {                                 
                                 propagatedCount++;
                                 //break;
                                 std::cout<<Simulator::GetContext ()<<": flood to face : "<<outFace->GetId()<<"  because of interest timed out, propagatedCount = "<<propagatedCount<<std::endl;
                           }
                        }
                        
                        NS_LOG_INFO ("Propagated to " << propagatedCount << " faces"); 
                        return propagatedCount > 0;                 
                    }*/
                    
            
                    /*bool isRetransmitted = m_detectRetransmissions && // a small guard
                                         DetectRetransmittedInterest (inFace, header, origPacket, pitEntry);
                    std::cout<<Simulator::GetContext ()<<": isRetransmitted : "<<isRetransmitted<<std::endl;
                    if (isRetransmitted)
                    {
                        Name name = Name ("/");
                        
                        FacesByMetric &faces = m_fib->Find (name)->m_faces.get<fib::i_metric> ();
                        for ( FacesByMetric::iterator faceIterator = faces.begin(); faceIterator != faces.end(); faceIterator ++)
                        {
                                if (TrySendOutInterest (inFace, faceIterator->GetFace (), header, origPacket, pitEntry))
                                {
                                        propagatedCount ++;
                                        std::cout<<Simulator::GetContext ()<<":flooding to face: " << faceIterator->GetFace ()->GetId ()<<" after interest timed out "<<std::endl;
                                }
                        }
                        
                        
                        NS_LOG_INFO ("Propagated to " << propagatedCount << " faces"); 
                        return propagatedCount > 0;
          
                    }*/
            


            if (val_int%10 == 0) //小概率forward到其他face (全部-fib)
            {
                Ptr<L3Protocol> ndn = this->GetObject<L3Protocol> ();
                //包括不在fib中的所有face
                for (uint32_t faceNum = 0; faceNum < ndn->GetNFaces (); faceNum++) 
                {
                   Ptr<Face> outFace = ndn->GetFace (faceNum);
                   bool collision = false;
                   //fib中的所有face，仿其他forwardstrategy
                   BOOST_FOREACH (const fib::FaceMetric &metricFace, pitEntry->GetFibEntry ()->m_faces.get<fib::i_metric> ()) 
                   {
                           if (metricFace.GetFace ()->GetId () == outFace->GetId())
                           {
                             collision = true;
                             break;
                           }
                   }
                   if (collision == false)
                   {
                     if(TrySendOutInterest (inFace, outFace, header, origPacket, pitEntry))
                     {                                 
                         propagatedCount++;
                         //break;
                         std::cout<<Simulator::GetContext ()<<": probe other faces with small possibility: "<<outFace->GetId()<<"  propagatedCount = "<<propagatedCount<<std::endl;
                     }
                   }
                   
                }
                
            }

              if((++faceIterator) == faces.end()) //只有一个face,faceIterator指向最后一个元素+1
              {
                    if (TrySendOutInterest (inFace, (--faceIterator)->GetFace (), header, origPacket, pitEntry)) //faceIterator指向最后一个元素
                    propagatedCount ++;
                    
                    std::cout<<Simulator::GetContext ()<<": send interest to the only one face: "<<faceIterator->GetFace ()->GetId()<<"  propagatedCount = "<<propagatedCount<<std::endl;

                    //m_fib->Find (name)->UpdateCounter (inFace); 
              }
              //if (count == 1)
              //{
              //      TrySendOutInterest (inFace, metricFace.GetFace (), header, origPacket, pitEntry);
              //      propagatedCount ++;
              //      for(int i = 0; i < 20; i++)
              //      {
              //          m_fib->Find (name)->faceCounter[i] = metricFace.GetFace ()->GetId ();
              //      }
              //}  
              else //多个face
              {
                    for (faceIterator = faces.begin(); faceIterator != faces.end(); faceIterator ++)
                    {
                       uint32_t n = m_fib->Find (name)->counter;
                       //val_int = var.GetInteger(1, 100);
                       
                       std::cout<<Simulator::GetContext ()<<": counter = "<<n<<"  val_int = "<<val_int<<std::endl;
                       
                       if (n < 20)
                       {
                          for(uint32_t i = 0; i < n; i++)
                          {
                                std::cout<<Simulator::GetContext ()<<": ["<<i<<"]: "<<m_fib->Find (name)->faceCounter[i]<<std::endl;
                          }
                       
                          if(faceIterator->GetFace ()->GetId () == m_fib->Find (name)->faceCounter[val_int%n])
                          {
                                  TrySendOutInterest (inFace, faceIterator->GetFace (), header, origPacket, pitEntry);
                                  propagatedCount ++; //TrySendOutInterest为假时不一定正确
                                  
                                 std::cout<<Simulator::GetContext ()<<": send interest to face: "<<faceIterator->GetFace ()->GetId()<<"  propagatedCount = "<<propagatedCount<<std::endl;
                                 
                                 break;
                          }
                       }
                       else
                       {
                          for(uint32_t i = 0; i < 20; i++)
                          {
                                std::cout<<Simulator::GetContext ()<<": ["<<i<<"]: "<<m_fib->Find (name)->faceCounter[i]<<std::endl;
                          }
                       
                          if(faceIterator->GetFace ()->GetId () == m_fib->Find (name)->faceCounter[val_int%20])
                          {
                                  TrySendOutInterest (inFace, faceIterator->GetFace (), header, origPacket, pitEntry);
                                  propagatedCount ++; //TrySendOutInterest为假时不一定正确
                                  
                                  std::cout<<Simulator::GetContext ()<<": send interest to face: "<<faceIterator->GetFace ()->GetId()<<"  propagatedCount = "<<propagatedCount<<std::endl;
                                  
                                  break;
                          }
                
                       }
                    } 
              }
           
          }
          /*else //fib是空表
          {
                Ptr<L3Protocol> ndn = this->GetObject<L3Protocol> ();
                for (uint32_t faceNum = 0; faceNum < ndn->GetNFaces (); faceNum++)
                {
                   Ptr<Face> outFace = ndn->GetFace (faceNum);
                   if (!TrySendOutInterest (inFace, outFace, header, origPacket, pitEntry))
                   {
                     continue;
                   }
                   propagatedCount++;
                }
                NS_LOG_INFO ("Propagated to " << propagatedCount << " faces");  
          }*/ 
  }
  NS_LOG_INFO ("Propagated to " << propagatedCount << " faces"); 
  return propagatedCount > 0;
}

void
CustomStrategy::DidSendOutInterest (Ptr<Face> inFace, Ptr<Face> outFace,
                                    Ptr<const Interest> header,
                                    Ptr<const Packet> origPacket,
                                    Ptr<pit::Entry> pitEntry)
{
  m_counter ++;
  
  //huyao130420
  m_outInterests (header, outFace);
}

void
CustomStrategy::WillEraseTimedOutPendingInterest (Ptr<pit::Entry> pitEntry)
{
  for (pit::Entry::out_container::iterator face = pitEntry->GetOutgoing ().begin ();
       face != pitEntry->GetOutgoing ().end ();
       face ++)
    {
      m_counter --;
    }

  BaseStrategy::WillEraseTimedOutPendingInterest (pitEntry);
}


void
CustomStrategy::WillSatisfyPendingInterest (Ptr<Face> inFace,
                                            Ptr<pit::Entry> pitEntry)
{
  for (pit::Entry::out_container::iterator face = pitEntry->GetOutgoing ().begin ();
       face != pitEntry->GetOutgoing ().end ();
       face ++)
    {
      m_counter --;
    }

  BaseStrategy::WillSatisfyPendingInterest (inFace, pitEntry);
}


} // namespace fw
} // namespace ndn
} // namespace ns3
