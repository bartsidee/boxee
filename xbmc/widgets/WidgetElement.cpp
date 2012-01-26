#include "WidgetElement.h"

#include "XmlUtils.h"

WidgetElement::WidgetElement()
{
	m_iWidth = DEFAULT_WIDTH;
	m_iHeight = DEFAULT_HEIGHT;
}

WidgetElement::~WidgetElement()
{
}

void WidgetElement::LoadFromXml(TiXmlElement* pWidgetElement) {

	// Width
	const char * szWidth = XmlUtils::GetWidgetElementAttribute(pWidgetElement, "width");
	if (szWidth) m_iWidth = atoi(szWidth);
	else m_iWidth = DEFAULT_WIDTH;
	
	// Height
	const char * szHeight = XmlUtils::GetWidgetElementAttribute(pWidgetElement, "height");
	if (szHeight) m_iHeight = atoi(szHeight);
	else m_iHeight = DEFAULT_HEIGHT;
	
	// Vertical Offset
	const char * szVoffset = XmlUtils::GetWidgetElementAttribute(pWidgetElement, "vOffset");
	if (szVoffset) m_iVoffset = atoi(szVoffset);
	else m_iVoffset = DEFAULT_V_OFFSET;
	
	// Horizontal Offset
	const char * szHoffset = XmlUtils::GetWidgetElementAttribute(pWidgetElement, "hOffset");
	if (szHoffset) m_iHoffset = atoi(szHoffset);
	else m_iHoffset = DEFAULT_H_OFFSET;
	
	// Name
	const char * szName = XmlUtils::GetWidgetElementAttribute(pWidgetElement, "name");
	if (szName) m_strName = szName;
	else m_strName = "";
	
	// Load events
	
	// Lod OnMouseUp event
	const char * szOnMouseUpEvent = XmlUtils::GetWidgetElementAttribute(pWidgetElement, "onMouseUp");
	if (szOnMouseUpEvent) {
		events[WidgetElement::MOUSE_UP] = szOnMouseUpEvent;
		cout << "On Mouse Up event: " << events[WidgetElement::MOUSE_UP] << endl;
	}
	
}
