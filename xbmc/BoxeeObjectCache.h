//
// C++ Interface: BoxeeObjectCache
//
// Description: 
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef BOXEEOBJECTCACHE_H
#define BOXEEOBJECTCACHE_H

#include <string>
#include "lib/libBoxee/bxobject.h"

class BoxeeObjectCache
{
public:
    BoxeeObjectCache();
    virtual ~BoxeeObjectCache();

    static BOXEE::BXObject LoadFromCache(const std::string &strType, const std::string &strId);
    static bool StoreInCache(BOXEE::BXObject obj);

    static std::string GenerateCacheName(const std::string &strType, const std::string &strId);
};

#endif
