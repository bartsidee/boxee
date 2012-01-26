#include "JSWidgetElement.h"

#include <iostream>

using namespace std;

/*JSClass JSWidgetElement::_class = { "WidgetElement", JSCLASS_HAS_PRIVATE,
		JS_PropertyStub, JS_PropertyStub, JSWidgetElement::JSGetProperty,
		JSWidgetElement::JSSetProperty, JS_EnumerateStub, JS_ResolveStub,
		JS_ConvertStub, JSWidgetElement::JSDestructor };

JSObject *JSWidgetElement::JSInit(JSContext *cx, JSObject *obj, JSObject *proto) {
	JSObject *newObj = JS_InitClass(cx, obj, proto, &_class,
			JSWidgetElement::JSConstructor, 0, JSWidgetElement::_properties,
			JSWidgetElement::_methods, NULL, NULL);
	return newObj;
}

JSBool JSWidgetElement::JSConstructor(JSContext *cx, JSObject *obj, uintN argc,
		jsval *argv, jsval *rval) {
	JSWidgetElement *p = new JSWidgetElement();
	if (!p)
		return JS_FALSE;
	if (!JS_SetPrivate(cx, obj, p))
		return JS_FALSE;
	*rval = OBJECT_TO_JSVAL(obj);
	p->setWidgetElement(new WidgetElement());
	return JS_TRUE;
}

void JSWidgetElement::JSDestructor(JSContext *cx, JSObject *obj) {
	JSWidgetElement *p = (JSWidgetElement*)JS_GetPrivate(cx, obj);
	delete p;
}*/

JSPropertySpec JSWidgetElement::_properties[] = { { "name", name_prop,
		JSPROP_ENUMERATE }, { 0 } };

//JSFunctionSpec JSWidgetElement::_methods[] = { { "computeReduction",
//		computeReduction, 1, 0, 0 }, { 0 } };

JSFunctionSpec JSWidgetElement::_methods[] = { { 0 } };



JSBool JSWidgetElement::JSGetProperty(JSContext *cx, JSObject *obj, jsval id,
		jsval *vp) {
	if (JSVAL_IS_INT(id)) {

		JSWidgetElement *priv = (JSWidgetElement *) JS_GetPrivate(cx, obj);
		WidgetElement* widgetElement = priv->getWidgetElement();

		switch (JSVAL_TO_INT(id)) {
		case name_prop:
			std::string name = widgetElement->GetName();
			JSString *str = JS_NewStringCopyN(cx, name.c_str(), name.length());
			*vp = STRING_TO_JSVAL(str);
			break;
		}
	}
	return JS_TRUE;
}

JSBool JSWidgetElement::JSSetProperty(JSContext *cx, JSObject *obj, jsval id,
		jsval *vp) {
	if (JSVAL_IS_INT(id)) {

		JSWidgetElement *priv = (JSWidgetElement *) JS_GetPrivate(cx, obj);
		WidgetElement* widgetElement = priv->getWidgetElement();

		switch (JSVAL_TO_INT(id)) {
		case name_prop:

			JSString *str = JS_ValueToString(cx, *vp);
			std::string name = JS_GetStringBytes(str);
			widgetElement->SetName(name);
			break;

		}
	}
	return JS_TRUE;
}




