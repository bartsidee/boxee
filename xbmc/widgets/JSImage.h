#ifndef JSIMAGE_H_
#define JSIMAGE_H_

#define XP_UNIX

#include "jsapi.h"
#include "JSWidgetElement.h"
#include "Image.h"

class JSImage  {
public:
	JSImage() :
		m_pImage(NULL) {
	}
	~JSImage() {
		delete m_pImage;
		m_pImage = NULL;
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
	
	// Full list of image properties
	/*
	tracking clipRect
	colorize
	fillMode
	hRegistrationPoint
	hslAdjustment
	hslTinting
	loadingSrc
	missingSrc
	remoteAsync
	rotation
	src
	srcHeight
	srcWidth
	tileOrigin
	tracking
	useFileIcon
	vRegistrationPoint
	*/

	
	// Property enum
	enum {src_prop};
	
	// List of Image functions
	/*
	fade()
	moveTo()
	reload()
	slide()
	*/

	
	// Function enum

	void setImage(Image *image) {
		m_pImage = image;
	}
	Image* getImage() {
		return m_pImage;
	}
	
protected:
private:
	Image* m_pImage;
};


#endif /*JSIMAGE_H_*/
