#ifndef WINDOW_H_
#define WINDOW_H_

#include "WidgetElement.h"

class JSContext;
class JSObject;

class Window : public WidgetElement
{

private:

	string m_strTitle;

public:
	Window();
	virtual ~Window();
	
	virtual void LoadFromXml(TiXmlElement* pWidgetElement);

	string getTitle() {
		return m_strTitle;
	}

	void setTitle(const string& title) {
		m_strTitle = title;
	}
	
	void LoadJSObject(JSContext* cx, JSObject* obj);
};

#endif /*WINDOW_H_*/
