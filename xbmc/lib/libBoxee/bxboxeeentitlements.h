// Copyright © 2008 BOXEE. All rights reserved.
//
// C++ Interface: bxboxeeproducts
//
// Description: 
//
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef BOXEEBXBOXEEENTITLEMENTS_H
#define BOXEEBXBOXEEENTITLEMENTS_H

#include <vector>
#include "bxxmldocument.h"
#include "bxobject.h"

namespace BOXEE {

/**
*/
class BXBoxeeEntitlements : public BXXMLDocument
{
public:
  BXBoxeeEntitlements();
  virtual ~BXBoxeeEntitlements();
  virtual bool Parse() ;	
  
  int GetNumOfEntitlements() const;
  BXObject GetEntitlement(int index) const;
  
  unsigned long GetTimeStamp() const;
  
  void Clear();
  bool IsLoaded();
  void SetLoaded(bool bLoaded=true);

protected:
  
  std::vector<BXObject>	m_entitlements;
  time_t 	m_timestamp;
  bool	m_bLoaded;
};

}

#endif
