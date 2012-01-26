#include "Text.h"
#include "Exception.h"
#include "XmlUtils.h"

Text::Text()
{
}

Text::~Text()
{
}

void Text::LoadFromXml(TiXmlElement* pWidgetElement) {
	/*
	 	<name>DilbertDaily</name>
		<data>Dilbert Daily widget by MrRamble.com</data>
		<vOffset>224</vOffset>
		<hOffset>390</hOffset>
		<size>12</size>
		<color>#000000</color>

		<onMouseUp>
			openURL('http://www.mrramble.com/?source=dilbertdaily100');
		</onMouseUp>
	 */
	
	// Check the name of the element
	string name=pWidgetElement->Value();

	if (name != "text") {
		throw new Exception("Element name is not \"text\"");
	}
	
	// Call the parent implementation
	WidgetElement::LoadFromXml(pWidgetElement);
	
	// Load text attributes
	const char * szSData = XmlUtils::GetWidgetElementAttribute(pWidgetElement, "data");
	m_strData = szSData ? szSData : "";
	
	// Load text attributes
	const char * szColor = XmlUtils::GetWidgetElementAttribute(pWidgetElement, "color");
	m_ulColor = szColor ? parseColor(szColor) : DEFAULT_COLOR;
	
	// Size
	const char * szSize = XmlUtils::GetWidgetElementAttribute(pWidgetElement, "size");
	if (szSize) m_iSize = atoi(szSize);
	else m_iSize = DEFAULT_FONT_SIZE;
	
}

unsigned long Text::parseColor(const char * szColor) {
	//scan $string "#%2x%2x%2x" r g b
	unsigned long r;
	unsigned long g;
	unsigned long b;
	
	sscanf(szColor, "#%2x%2x%2x", &r, &g, &b);
	
	printf ("Color: %d, %d, %d\n", r,g,b);
	
	unsigned long result = 0xFF;
	result = result << 24;
	result = result | (r << 16);
	result = result | (g << 8);
	result = result | (b);
	
	printBinary(result);
	
	return result;
}

void Text::printBinary(unsigned long n) {
    // print binary with leading zeros
      cout << "binary : ";
      for (int i=31; i>=0; i--) {
          int bit = ((n >> i) & 1);
          cout << bit;
      }
      cout << endl;
}

void Text::LoadJSObject(JSContext* cx, JSObject* obj) {
	
}
