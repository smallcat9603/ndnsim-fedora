/* -*- Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2011 University of California, Los Angeles
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

#ifndef _NDN_FIB_ENTRY_H_
#define	_NDN_FIB_ENTRY_H_

#include "ns3/ptr.h"
#include "ns3/nstime.h"
#include "ns3/ndn-face.h"
#include "ns3/ndn-name.h"
#include "ns3/ndn-limits.h"
#include "ns3/traced-value.h"

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/tag.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/composite_key.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/random_access_index.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/mem_fun.hpp>

//huyao130415
#include <ns3/simulator.h>

namespace ns3 {
namespace ndn {

class Name;
typedef Name NameComponents;

namespace fib {

/**
 * \ingroup ndn
 * \brief Structure holding various parameters associated with a (FibEntry, Face) tuple
 */
class FaceMetric
{
public:
  /**
   * @brief Color codes for FIB face status
   */
  enum Status { NDN_FIB_GREEN = 1,
                NDN_FIB_YELLOW = 2,
                NDN_FIB_RED = 3 };
public:
  /**
   * \brief Metric constructor
   *
   * \param face Face for which metric
   * \param cost Initial value for routing cost
   */
  FaceMetric (Ptr<Face> face, int32_t cost)
    : m_face (face)
    , m_status (NDN_FIB_YELLOW)
    , m_routingCost (cost)
    , m_sRtt   (Seconds (0))
    , m_rttVar (Seconds (0))
    , m_realDelay (Seconds (0))
  { }

  /**
   * \brief Comparison operator used by boost::multi_index::identity<>
   */
  bool
  operator< (const FaceMetric &fm) const { return *m_face < *fm.m_face; } // return identity of the face

  /**
   * @brief Comparison between FaceMetric and Face
   */
  bool
  operator< (const Ptr<Face> &face) const { return *m_face < *face; }

  /**
   * @brief Return Face associated with FaceMetric
   */
  Ptr<Face>
  GetFace () const { return m_face; }

  /**
   * \brief Recalculate smoothed RTT and RTT variation
   * \param rttSample RTT sample
   */
  void
  UpdateRtt (const Time &rttSample);

  /**
   * @brief Get current status of FIB entry
   */
  Status
  GetStatus () const
  {
    return m_status;
  }

  /**
   * @brief Set current status of FIB entry
   */
  void
  SetStatus (Status status)
  {
    m_status.Set (status);
  }

  /**
   * @brief Get current routing cost
   */
  int32_t
  GetRoutingCost () const
  {
    return m_routingCost;
  }

  /**
   * @brief Set routing cost
   */
  void
  SetRoutingCost (int32_t routingCost)
  {
    m_routingCost = routingCost;
  }

  /**
   * @brief Get real propagation delay to the producer, calculated based on NS-3 p2p link delays
   */
  Time
  GetRealDelay () const
  {
    return m_realDelay;
  }

  /**
   * @brief Set real propagation delay to the producer, calculated based on NS-3 p2p link delays
   */
  void
  SetRealDelay (Time realDelay)
  {
    m_realDelay = realDelay;
  }

  /**
   * @brief Get direct access to status trace
   */
  TracedValue<Status> &
  GetStatusTrace ()
  {
    return m_status;
  }

private:
  friend std::ostream& operator<< (std::ostream& os, const FaceMetric &metric);

private:
  Ptr<Face> m_face; ///< Face

  TracedValue<Status> m_status; ///< \brief Status of the next hop:
				///<		- NDN_FIB_GREEN
				///<		- NDN_FIB_YELLOW
				///<		- NDN_FIB_RED

  int32_t m_routingCost; ///< \brief routing protocol cost (interpretation of the value depends on the underlying routing protocol)

  Time m_sRtt;         ///< \brief smoothed round-trip time
  Time m_rttVar;       ///< \brief round-trip time variation

  Time m_realDelay;    ///< \brief real propagation delay to the producer, calculated based on NS-3 p2p link delays
};

/// @cond include_hidden
class i_face {};
class i_metric {};
class i_nth {};
/// @endcond


/**
 * \ingroup ndn
 * \brief Typedef for indexed face container of Entry
 *
 * Currently, there are 2 indexes:
 * - by face (used to find record and update metric)
 * - by metric (face ranking)
 * - random access index (for fast lookup on nth face). Order is
 *   maintained manually to be equal to the 'by metric' order
 */
struct FaceMetricContainer
{
  /// @cond include_hidden
  typedef boost::multi_index::multi_index_container<
    FaceMetric,
    boost::multi_index::indexed_by<
      // For fast access to elements using Face
      boost::multi_index::ordered_unique<
        boost::multi_index::tag<i_face>,
        boost::multi_index::const_mem_fun<FaceMetric,Ptr<Face>,&FaceMetric::GetFace>
      >,

      // List of available faces ordered by (status, m_routingCost)
      boost::multi_index::ordered_non_unique<
        boost::multi_index::tag<i_metric>,
        boost::multi_index::composite_key<
          FaceMetric,
          boost::multi_index::const_mem_fun<FaceMetric,FaceMetric::Status,&FaceMetric::GetStatus>,
          boost::multi_index::const_mem_fun<FaceMetric,int32_t,&FaceMetric::GetRoutingCost>
        >
      >,

      // To optimize nth candidate selection (sacrifice a little bit space to gain speed)
      boost::multi_index::random_access<
        boost::multi_index::tag<i_nth>
      >
    >
   > type;
  /// @endcond
};

/**
 * \ingroup ndn
 * \brief Structure for FIB table entry, holding indexed list of
 *        available faces and their respective metrics
 */
class Entry : public Object
{
public:
  typedef Entry base_type;

public:
  class NoFaces {}; ///< @brief Exception class for the case when FIB entry is not found

  /**
   * \brief Constructor
   * \param prefix smart pointer to the prefix for the FIB entry
   */
  Entry (const Ptr<const Name> &prefix)
  : m_prefix (prefix)
  , m_needsProbing (false)
  {
        index = 0; //huyao130415 类成员要在构造函数里初始化
  }

  /**
   * \brief Update status of FIB next hop
   * \param status Status to set on the FIB entry
   */
  void UpdateStatus (Ptr<Face> face, FaceMetric::Status status);

  /**
   * \brief Add or update routing metric of FIB next hop
   *
   * Initial status of the next hop is set to YELLOW
   */
  void AddOrUpdateRoutingMetric (Ptr<Face> face, int32_t metric);

  /**
   * \brief Set real delay to the producer
   */
  void
  SetRealDelayToProducer (Ptr<Face> face, Time delay);

  /**
   * @brief Invalidate face
   *
   * Set routing metric on all faces to max and status to RED
   */
  void
  Invalidate ();

  /**
   * @brief Update RTT averages for the face
   */
  void
  UpdateFaceRtt (Ptr<Face> face, const Time &sample);

  /**
   * \brief Get prefix for the FIB entry
   */
  const Name&
  GetPrefix () const { return *m_prefix; }

  /**
   * \brief Find "best route" candidate, skipping `skip' first candidates (modulo # of faces)
   *
   * throws Entry::NoFaces if m_faces.size()==0
   */
  const FaceMetric &
  FindBestCandidate (uint32_t skip = 0) const;

  /**
   * @brief Remove record associated with `face`
   */
  void
  RemoveFace (const Ptr<Face> &face)
  {
    m_faces.erase (face);
  }

private:
  friend std::ostream& operator<< (std::ostream& os, const Entry &entry);

public:
  Ptr<const Name> m_prefix; ///< \brief Prefix of the FIB entry
  FaceMetricContainer::type m_faces; ///< \brief Indexed list of faces

  bool m_needsProbing;      ///< \brief flag indicating that probing should be performed

  //huyao 130316
  uint32_t faceCounter[20]; 
  uint32_t counter; //UpdateCounter总共访问的次数
  uint32_t index;
  
  void
  UpdateCounter (Ptr<Face> inface)
  {
    //static uint32_t index = 0;
    //index = 0;
    faceCounter[index%20] = inface->GetId();
    
    //huyao130414
    //std::cout<<Simulator::GetContext ()<<": "<<index%20<<": "<<inface->GetId()<<std::endl;
    //for(uint32_t i = 0; i < 20; i++)
    //{
    //    std::cout<<Simulator::GetContext ()<<": ["<<i<<"]: "<<faceCounter[i]<<std::endl;
    //}
    
    index++;
    counter = index;
    
    //huyao130414
    //std::cout<<"counter="<<counter<<std::endl;
    
    
    //if (index == 20)
    //{
    //   index = 0;
    //}
  }
};

std::ostream& operator<< (std::ostream& os, const Entry &entry);
std::ostream& operator<< (std::ostream& os, const FaceMetric &metric);

} // namespace fib
} // namespace ndn
} // namespace ns3

#endif // _NDN_FIB_ENTRY_H_
