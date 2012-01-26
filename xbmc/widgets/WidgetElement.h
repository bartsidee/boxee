#ifndef WIDGETELEMENT_H_
#define WIDGETELEMENT_H_

#include <iostream>
#include <map>
#include "tinyxml.h"

using namespace std;

class JSContext;
class JSObject;

/**
 * A basic building block of the widget
 * Serves as father of all widget elements
 */
class WidgetElement
{
	
public:
	enum WidgetEvent {
		MOUSE_DOWN,
		MOUSE_UP
	};
	
	static const int DEFAULT_WIDTH = -1;
	static const int DEFAULT_HEIGHT = -1;
	static const int DEFAULT_V_OFFSET = -1;
	static const int DEFAULT_H_OFFSET = -1;

private:

	string m_strName;
	int m_iWidth;
	int m_iHeight;

	int m_iVoffset;
	int m_iHoffset;

	// Holds the map of events
	map<WidgetEvent, string> events;

public:
	WidgetElement();
	virtual ~WidgetElement();

	virtual void LoadFromXml(TiXmlElement* pWidgetElement);

	// Common getters and setters
	string GetName() {
		return m_strName;
	}

	void SetName(const string& name) {
		m_strName = name;
	}

	int GetHeight() {
		return m_iHeight;
	}

	void SetHeight(int height) {
		m_iHeight = height;
	}

	int GetWidth() {
		return m_iWidth;
	}

	void SetWidth(int width) {
		m_iWidth = width;
	}

	int GetVoffset() {
		return m_iVoffset;
	}

	void SetVOffset(int iVoffset) {
		m_iVoffset = iVoffset;
	}

	int GetHoffset() {
		return m_iHoffset;
	}

	void SetHOffset(int iHoffset) {
		m_iHoffset = iHoffset;
	}
	
// JavaScript related members
protected:
	// Pointer to the JS object representing the image
	JSObject* m_pJSObject;	
	
public:
	virtual void LoadJSObject(JSContext* cx, JSObject* obj)=0;
};

#endif /*WIDGETELEMENT_H_*/
