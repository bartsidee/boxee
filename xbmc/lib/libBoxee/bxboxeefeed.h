// Copyright © 2008 BOXEE. All rights reserved.
//
// C++ Interface: bxboxeefeed
//
// Description: 
//
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef BOXEEBXBOXEEFEED_H
#define BOXEEBXBOXEEFEED_H

#include <vector>
#include "bxxmldocument.h"
#include "bxmessages.h"

namespace BOXEE 
{

/**
*/
class BXBoxeeFeed : public BXXMLDocument
{
public:
  BXBoxeeFeed();
  virtual ~BXBoxeeFeed();
  virtual bool Parse() ;	
  
  int GetNumOfActions() const;
  BXGeneralMessage GetAction(int nIndex) const;
  
  unsigned long GetTimeStamp() const;
  
  void Clear();
  bool IsLoaded();
  void SetLoaded(bool bLoaded=true);

protected:
  std::vector<BXGeneralMessage>	m_actions;
  time_t 	m_timestamp;
  bool	m_bLoaded;
};

}

#endif
