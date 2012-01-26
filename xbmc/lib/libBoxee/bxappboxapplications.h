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

#include <vector>
#include "bxxmldocument.h"

namespace BOXEE
{

class BXAppBoxApplications : public BXXMLDocument
{
public:
  BXAppBoxApplications();
  virtual ~BXAppBoxApplications();

  void Clear();

  bool IsLoaded();
  void SetLoaded(bool bLoaded=true);

protected:
  
  bool	m_bLoaded;
};

}

#endif
