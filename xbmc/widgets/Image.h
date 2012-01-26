#ifndef IMAGE_H_
#define IMAGE_H_

#include "WidgetElement.h"
#include <string>

using namespace std;

class JSContext;
class JSObject;

class Image : public WidgetElement
{
private: 

	string m_strSource;

public:
	Image();
	virtual ~Image();

	virtual void LoadFromXml(TiXmlElement* pWidgetElement);

	string GetSource() {
		return m_strSource;
	}
	void SetSource(string strSource) {
		m_strSource = strSource;
	}
	
	virtual void LoadJSObject(JSContext* cx, JSObject* obj);

};

#endif /*IMAGE_H_*/
