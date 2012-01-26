#ifndef JSTEXT_H_
#define JSTEXT_H_

#define XP_UNIX

#include "jsapi.h"
#include "Text.h"

class JSText {
public:
	JSText() :
		m_pText(NULL) {
	}
	~JSText() {
		delete m_pText;
		m_pText = NULL;
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
	void setText(Text* text) {
		m_pText = text;
	}
	Text* getText() {
		return m_pText;
	}
private:
	Text* m_pText;
};


#endif /*JSTEXT_H_*/
