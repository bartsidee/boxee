// Copyright © 2008 BOXEE. All rights reserved.
//
// C++ Interface: bxboxeeapps
//
// Description: 
//
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef BOXEEBXBOXEESUBSCRIPTIONS_H
#define BOXEEBXBOXEESUBSCRIPTIONS_H

#include <vector>
#include "bxxmldocument.h"
#include "bxobject.h"

namespace BOXEE
{

class BXBoxeeSubscriptions : public BXXMLDocument
{
public:
  BXBoxeeSubscriptions();
  virtual ~BXBoxeeSubscriptions();
  virtual bool Parse() ;	
  
  int GetNumOfSubscriptions() const;
  BXObject GetSubscription(int index) const;
  
  unsigned long GetTimeStamp() const;
  
  void Clear();
  bool IsLoaded();
  void SetLoaded(bool bLoaded=true);

protected:
  
  std::vector<BXObject>	m_subscriptions;
  time_t 	m_timestamp;
  bool	m_bLoaded;
};

}

#endif
