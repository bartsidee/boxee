//
// C++ Implementation: bxappboxapplications
//
// Description:
//
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include "bxappboxapplications.h"
#include "logger.h"

namespace BOXEE
{

BXAppBoxApplications::BXAppBoxApplications() : BXXMLDocument()
{
  m_bLoaded = false;
}

BXAppBoxApplications::~BXAppBoxApplications()
{

}

void BXAppBoxApplications::Clear()
{
  m_bLoaded = false;
}

bool BXAppBoxApplications::IsLoaded()
{
  return m_bLoaded;
}

void BXAppBoxApplications::SetLoaded(bool bLoaded)
{
  m_bLoaded = bLoaded;
}

}

