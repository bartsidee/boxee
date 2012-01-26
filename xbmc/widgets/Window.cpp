#include "Window.h"

#include "WidgetElementFactory.h"
#include "Exception.h"
#include "XmlUtils.h"

Window::Window()
{
}

Window::~Window()
{
}

void Window::LoadFromXml(TiXmlElement* pWidgetElement) {

	// Check the name of the element
	string name=pWidgetElement->Value();

	if (name != "window") {
		throw new Exception("Element name is not \"window\"");
	}

	// Call the parent implementation
	WidgetElement::LoadFromXml(pWidgetElement);

	// Load window attributes
	const char * szTitle = XmlUtils::GetWidgetElementAttribute(pWidgetElement, "title");
	m_strTitle = szTitle ? szTitle : "";

	WidgetElement * pElement = NULL;

	// Go over all elements in the widget
	TiXmlNode * pChild;
	for ( pChild = pWidgetElement->FirstChild(); pChild != 0; pChild = pChild->NextSibling()) 
	{
		int type = pWidgetElement->Type();

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

	// Extract the window attributes
}

void Window::LoadJSObject(JSContext* cx, JSObject* obj) {
	
}
