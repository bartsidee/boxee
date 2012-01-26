#include "JSImage.h"
#include <iostream>

using namespace std;

JSClass JSImage::_class = { "Image", JSCLASS_HAS_PRIVATE,
		JS_PropertyStub, JS_PropertyStub, JSImage::JSGetProperty,
		JSImage::JSSetProperty, JS_EnumerateStub, JS_ResolveStub,
		JS_ConvertStub, JSImage::JSDestructor };

JSObject *JSImage::JSInit(JSContext *cx, JSObject *obj, JSObject *proto) {
	JSObject *newObj = JS_InitClass(cx, obj, proto, &_class,
			JSImage::JSConstructor, 0, JSImage::_properties,
			JSImage::_methods, NULL, NULL);
	return newObj;
}

JSBool JSImage::JSConstructor(JSContext *cx, JSObject *obj, uintN argc,
		jsval *argv, jsval *rval) {
	JSImage *p = new JSImage();
	if (!p)
		return JS_FALSE;
	if (!JS_SetPrivate(cx, obj, p))
		return JS_FALSE;
	*rval = OBJECT_TO_JSVAL(obj);
	p->setImage(new Image());
	return JS_TRUE;
}

void JSImage::JSDestructor(JSContext *cx, JSObject *obj) {
	JSImage *p = (JSImage*)JS_GetPrivate(cx, obj);
	delete p;
}

JSPropertySpec JSImage::_properties[] = { { "src", src_prop,
		JSPROP_ENUMERATE }, { 0 } };

//JSFunctionSpec JSImage::_methods[] = { { "computeReduction",
//		computeReduction, 1, 0, 0 }, { 0 } };

JSFunctionSpec JSImage::_methods[] = { { 0 } };



JSBool JSImage::JSGetProperty(JSContext *cx, JSObject *obj, jsval id,
		jsval *vp) {
	if (JSVAL_IS_INT(id)) {

		JSImage *priv = (JSImage *) JS_GetPrivate(cx, obj);
		Image* image = priv->getImage();

		switch (JSVAL_TO_INT(id)) {
		case src_prop: 
		{
			std::string source = image->GetSource();
			JSString *str = JS_NewStringCopyN(cx, source.c_str(), source.length());
			*vp = STRING_TO_JSVAL(str);
			break;
		}
		
		default: 
		{
			// Call WidgetElement implementation
			JSWidgetElement::JSGetProperty(cx, obj, id, vp);
		}
		}
	}
	return JS_TRUE;
}

JSBool JSImage::JSSetProperty(JSContext *cx, JSObject *obj, jsval id,
		jsval *vp) {
	if (JSVAL_IS_INT(id)) {

		JSImage *priv = (JSImage *) JS_GetPrivate(cx, obj);
		Image* image = priv->getImage();

		switch (JSVAL_TO_INT(id)) {
		case src_prop: {

			JSString *str = JS_ValueToString(cx, *vp);
			std::string source = JS_GetStringBytes(str);
			image->SetSource(source);
			break;

		}
		default: 
			// Call WidgetElement implementation
			JSWidgetElement::JSGetProperty(cx, obj, id, vp);
		
		}
	}
	return JS_TRUE;
}


