#ifndef JSWIDGET_H_
#define JSWIDGET_H_

#define XP_UNIX

#include "jsapi.h"
#include "Widget.h"

class JSWidget {
public:
	JSWidget() :
		m_pWidget(NULL) {
	}
	~JSWidget() {
		delete m_pWidget;
		m_pWidget= NULL;
	}
	static JSClass _class;

	static JSBool JSGetProperty(JSContext *cx, JSObject *obj, jsval id,
			jsval *vp);
	static JSBool JSSetProperty(JSContext *cx, JSObject *obj, jsval id,
			jsval *vp);
	static JSBool JSConstructor(JSContext *cx, JSObject *obj, uintN argc,
			jsval *argv, jsval *rval);
	static void JSDestructor(JSContext *cx, JSObject *obj);

	static JSObject *JSInit(JSContext *cx, JSObject *obj, JSObject *proto = NULL);

	static JSPropertySpec _properties[];
	static JSFunctionSpec _methods[];
	
	// Property enum
	enum {data_prop, color_prop, size_prop};
	
	// Function enum
	
	

protected:
	void setWidget(Widget *widget) {
		m_pWidget= widget;
	}
	Widget* getWidget() {
		return m_pWidget;
	}
private:
	Widget* m_pWidget;
};




#endif /*JSWIDGET_H_*/
