#include "Action.h"
#include "Exception.h"
#include "XmlUtils.h"

Action::Action()
{
}

Action::~Action()
{
}

void Action::LoadFromXml(TiXmlElement* pWidgetElement) {
	
	// Check the name of the element
	string name=pWidgetElement->Value();

	if (name != "action") {
		throw new Exception("Element name is not \"action\"");
	}
	
	// TODO: We do not call parent implementation here since this is not required
	// check if the existing hierarchy is good in this case or maybe another base class / interface
	// should be added
	
	// Load trigger attribute
	const char * szTrigger = XmlUtils::GetWidgetElementAttribute(pWidgetElement, "trigger");
	m_Trigger = parseActionTrigger(szTrigger);
	
	// Load interval attributes
	const char * szInterval = XmlUtils::GetWidgetElementAttribute(pWidgetElement, "interval");
	m_iInterval = szInterval ? atoi(szInterval): NO_INTERVAL;
	
	// Load file attributes
	const char * szFile= XmlUtils::GetWidgetElementAttribute(pWidgetElement, "file");
	m_strFile = szFile ? szFile : "";
	
	m_strScript = pWidgetElement->Value();
	
	
}

Action::Trigger Action::parseActionTrigger(const string & strTrigger) {
	if (strTrigger == "onLoad") {
		return Action::onLoad;
	}
	else {
		return Action::UNDEFINED;
	}
}

void Action::LoadJSObject(JSContext* cx, JSObject* obj) {
	
}
