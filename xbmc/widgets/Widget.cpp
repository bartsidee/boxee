#include "Widget.h"
#include "WidgetElementFactory.h"
#include "Exception.h"
#include "Action.h"


Widget::Widget()
{
}

Widget::~Widget()
{
	for (int i = 0; i < m_arrWidgetElements.size(); i++) {
		delete m_arrWidgetElements[i];
	}
	
	delete m_pOnLoadAction;
}

void Widget::LoadFromXml(TiXmlElement* pWidgetElement) {

	// Check the name of the element
	string name=pWidgetElement->Value();

	if (name != "widget") {
		throw new Exception("Element name is not \"widget\"");
	}
	
	WidgetElement * pElement = NULL;

	// Go over all elements in the widget
	TiXmlNode * pChild;
	for ( pChild = pWidgetElement->FirstChild(); pChild != 0; pChild = pChild->NextSibling()) 
	{
		int type = pChild->Type();
		
		const char * szElementName;

		switch ( type )
		{
		case TiXmlNode::COMMENT:
				case TiXmlNode::TEXT:
				case TiXmlNode::UNKNOWN:
			break;		
		case TiXmlNode::ELEMENT:
			// Create the element using the factory
			
			szElementName = pChild->Value();
			
			pElement = WidgetElementFactory::createWidgetElement(szElementName);
			if (!pElement) {
				cerr << "ERROR, Could not create widget element: " << szElementName << endl;
				continue;
			}
			pElement->LoadFromXml((TiXmlElement*)pChild);
			break;
		
		default:
		    break;
		}
	}

	
}

bool Widget::validateChildElement(const string& type) {
	if ((type == "about-box") || ("action") || ("hotkey") 
			|| ("preference") || ("security") || ("script")
			|| ("window"))
		return true;
	return false;
	
}
