// Copyright © 2008 BOXEE. All rights reserved.
//
// C++ Interface: bxboxeesources
//
// Description: 
//
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef BOXEEBXBOXEESOURCES_H
#define BOXEEBXBOXEESOURCES_H

#include <vector>
#include "bxxmldocument.h"
#include "bxobject.h"

namespace BOXEE {

/**
*/
class BXBoxeeSources : public BXXMLDocument
{
public:
  BXBoxeeSources();
  virtual ~BXBoxeeSources();
  virtual bool Parse() ;	
  
  int GetNumOfSources() const;
  void AddSource(BXObject source);
  BXObject GetSource(int index) const;
  
  unsigned long GetTimeStamp() const;
  
  void Clear();
  bool IsLoaded();
  void SetLoaded(bool bLoaded=true);

protected:
  
  std::vector<BXObject>	m_sources;
  time_t 	m_timestamp;
  bool	m_bLoaded;
};

}

#endif
