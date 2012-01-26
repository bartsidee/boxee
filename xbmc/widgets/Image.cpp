#include "Image.h"
#include "Exception.h"
#include "XmlUtils.h"
#include "JSImage.h"

#include <jsapi.h>
#include <jsautocfg.h>


Image::Image()
{

}

Image::~Image()
{
}

void Image::LoadFromXml(TiXmlElement* pWidgetElement) {

	// Check the name of the element
	string name=pWidgetElement->Value();

	if (name != "image") {
		throw new Exception("Element name is not \"image\"");
	}

	// Call the parent implementation
	WidgetElement::LoadFromXml(pWidgetElement);

	// Load image attributes
	const char * szSource = XmlUtils::GetWidgetElementAttribute(pWidgetElement, "src");
	m_strSource = szSource ? szSource : "";
}

void Image::LoadJSObject(JSContext* cx, JSObject* obj) {
	
	m_pJSObject = JS_DefineObject(cx, 
			obj, 
			GetName().c_str(), 
			&JSImage::_class, 
			NULL, 
			0);
	
	// Set private data for the object
	JSImage *priv = new JSImage();
	priv->setImage(this);
	JS_SetPrivate(cx, m_pJSObject, (void *) priv);

	//	string str = "Alexx";
	//	JSString* jsString = JS_NewStringCopyZ(engine->getContext(), str.c_str());
	//	jsval val = STRING_TO_JSVAL(jsString);
	//
	//	JS_SetProperty(engine->getContext(), customerObject, "name", &val);
}

