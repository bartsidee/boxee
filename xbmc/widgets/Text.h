#ifndef TEXT_H_
#define TEXT_H_

#include "WidgetElement.h"

class Text : public WidgetElement
{
	
public:
	
	static const int DEFAULT_FONT_SIZE = 10;
	static const unsigned long DEFAULT_COLOR = 0;
	
private:
	
	unsigned long m_ulColor;
	int m_iSize;
	string m_strData;
	
	/**
	 * This function receives a string representation
	 * of the color as it appears in the widget definition
	 * and returns aRGB value as unsigned long (four bytes)
	 */
	unsigned long parseColor(const char * szColor);
	void printBinary(unsigned long n);
	
public:
	Text();
	virtual ~Text();
	
	virtual void LoadFromXml(TiXmlElement* pWidgetElement);
	
	virtual void LoadJSObject(JSContext* cx, JSObject* obj);
};

#endif /*TEXT_H_*/
