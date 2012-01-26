#include "RemoteWrapper.h"
#include "../../guilib/common/IRServerSuite/IRServerSuite.h"
#include "RemoteControl.h"

// Implementation of the IRSS remote strategy
CRemoteWrapper g_RemoteControl;

bool CIRSSRemoteStrategy::Initialize()
{
	g_IRSSRemoteControl.Initialize();
  
  return true;
}
 
void CIRSSRemoteStrategy::Reset()
{
	g_IRSSRemoteControl.Reset();
}

void CIRSSRemoteStrategy::Update()
{
	g_IRSSRemoteControl.Update();
}

WORD CIRSSRemoteStrategy::GetButton()
{
	return g_IRSSRemoteControl.GetButton();
}
 
bool CIRSSRemoteStrategy::IsHolding()
{
	return g_IRSSRemoteControl.IsHolding();
}

// Implementation of the Boxee Remote Strategy

bool CBoxeeRemoteStrategy::Initialize()
{
	return g_BoxeeRemoteControl.Initialize();
}
 
void CBoxeeRemoteStrategy::Reset()
{
	g_BoxeeRemoteControl.Reset();
}

void CBoxeeRemoteStrategy::Update()
{
	g_BoxeeRemoteControl.Update();
}

WORD CBoxeeRemoteStrategy::GetButton()
{
	return g_BoxeeRemoteControl.GetButton();
}
 
bool CBoxeeRemoteStrategy::IsHolding()
{
	return g_BoxeeRemoteControl.IsHolding();
}

CRemoteWrapper::CRemoteWrapper()
{
	m_pRemoteStrategy = new CBoxeeRemoteStrategy();
}

CRemoteWrapper::~CRemoteWrapper()
{
	if (m_pRemoteStrategy)
		delete m_pRemoteStrategy;
}


void CRemoteWrapper::SelectStrategy(bool bPreferIRSS)
{
	if (m_pRemoteStrategy)
	{
		delete m_pRemoteStrategy;
	}

	if (bPreferIRSS)
	{
		m_pRemoteStrategy = new CIRSSRemoteStrategy();
	}
	else
	{
		m_pRemoteStrategy = new CBoxeeRemoteStrategy();
	}
}