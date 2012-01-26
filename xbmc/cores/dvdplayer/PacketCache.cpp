/*
 * PacketCache.cpp
 *
 *  Created on: Apr 6, 2011
 *      Author: ishai
 */

#include "PacketCache.h"
#include "DVDDemuxers/DVDDemuxUtils.h"
#include "DVDClock.h"

const unsigned CPacketCache::MSECS_TO_QUEUE = 3000000;

CACHE_QUEUE_TYPE CPacketCache::GetCachedPackets(int source, int id)
{

  CCacheKey key = {source, id};
  for(CACHE_QUEUE_TYPE::const_iterator iter=m_cache_map[key].begin(); iter != m_cache_map[key].end(); ++iter)
    (*iter)->DisableFree();  // Need to prevent the freeing of those packets - would be conducted by the proper Process method.

  CACHE_QUEUE_TYPE cached_data = m_cache_map[key];
  m_cache_map.erase(key);   // Delete all the processed packets from the cache.
  return cached_data;
}

void CPacketCache::CachePacket(CDemuxStream* pStream, DemuxPacket* pPacket,  double time)
{
  CCacheKey key = {pStream->source, pPacket->iStreamId};
  CACHE_CONTAINER_TYPE val = CACHE_CONTAINER_TYPE(new CCacheContainer(pStream, pPacket, time));

  while (!(m_cache_map[key].empty()) && (m_cache_map[key].back()->GetTime() > time))
    m_cache_map[key].pop_back();  // Delete the packets newer than the new packet (e.g. if we seek backwards, etc.).

  while (!(m_cache_map[key].empty()) && (m_cache_map[key].front()->GetTime() < time-MSECS_TO_QUEUE))
      m_cache_map[key].pop_front();  // Delete the too early packets.

  m_cache_map[key].push_back(val);  // val is now the newest packet.
}

CCacheContainer::~CCacheContainer() {
  try {
    if(m_is_need_free) {
      if (m_pPacket != NULL)
        CDVDDemuxUtils::FreeDemuxPacket(m_pPacket); // free it since we won't do anything with it
      m_pPacket = NULL;
      m_pStream = NULL;
    }
  }
  catch(...) {}
}
