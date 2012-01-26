// Copyright © 2008 BOXEE. All rights reserved.
//
// C++ Interface: bxappboxapplications
//
// Description: 
//
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef BXAPPBOXAPPLICATIONS_H
#define BXAPPBOXAPPLICATIONS_H

#include <set>
#include "bxxmldocument.h"

namespace BOXEE
{

class BXAppBoxApplications : public BXXMLDocument
{
public:
  BXAppBoxApplications();
  virtual ~BXAppBoxApplications();

  virtual bool Parse();

  void Clear();

  bool IsLoaded();
  void SetLoaded(bool bLoaded=true);

  bool IsApplicationIdExist(std::string appId);

  int GetNumOfApplication() const;

protected:
  
  std::set<std::string> m_appBoxApplicationIdsMap;
  bool	m_bLoaded;
};

}

#endif
