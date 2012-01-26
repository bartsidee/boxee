/*
 * PacketCache.h
 *
 *  Created on: Apr 6, 2011
 *      Author: ishai
 */

#ifndef PACKETCACHE_H_
#define PACKETCACHE_H_

#include <map>
#include <queue>
#include "DVDDemuxers/DVDDemux.h"
#include <boost/shared_ptr.hpp>

class CCacheContainer {
private:
    CDemuxStream* m_pStream;
    DemuxPacket* m_pPacket;
    bool m_is_need_free;
    double m_time;
public:
    CCacheContainer() {m_is_need_free=true; m_pStream=NULL; m_pPacket=NULL; m_time=0;}
    CCacheContainer(CDemuxStream* pStream, DemuxPacket* pPacket,  double time): m_pStream(pStream), m_pPacket(pPacket), m_time(time) {m_is_need_free=true;}
    ~CCacheContainer();
    CDemuxStream* GetStream() {return m_pStream;}
    DemuxPacket* GetPacket() {return m_pPacket;}
    double GetTime() {return m_time;}
    void DisableFree() {m_is_need_free=false;}
    // The default assignment operator is good for the STL.
};

typedef boost::shared_ptr<CCacheContainer> CACHE_CONTAINER_TYPE;
// Why do we need a shared_ptr here? since we put the CCacheContainer in an STL container -> they could be destroyed and recreated
// - and we need to call the dtor only after there is no other reference to the packet.
typedef std::deque<CACHE_CONTAINER_TYPE> CACHE_QUEUE_TYPE;

class CPacketCache {
  struct CCacheKey {
      int m_source;
      int m_id;
      bool operator<(const CCacheKey& rval) const{
          return (m_source<<16)+m_id<(rval.m_source<<16)+rval.m_id;
      }
  };  // For (source_type, stream_id).

  typedef std::map<CCacheKey, CACHE_QUEUE_TYPE> CACHE_MAP_TYPE;


  CACHE_MAP_TYPE m_cache_map;
  static const unsigned MSECS_TO_QUEUE;

public:
  CACHE_QUEUE_TYPE GetCachedPackets(int source, int id);
  void CachePacket(CDemuxStream* pStream, DemuxPacket* pPacket, double time);
};

#endif /* PACKETCACHE_H_ */
