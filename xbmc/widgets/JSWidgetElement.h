#ifndef JSWIDGETELEMENT_H_
#define JSWIDGETELEMENT_H_

#define XP_UNIX

#include "jsapi.h"
#include "WidgetElement.h"

class JSWidgetElement {
public:
	JSWidgetElement() //: m_pWidgetElement(NULL) 
	{
	}
	~JSWidgetElement() {
		//delete m_pWidgetElement;
		//m_pWidgetElement= NULL;
	}
	static JSClass _class;

	static JSBool JSGetProperty(JSContext *cx, JSObject *obj, jsval id,
			jsval *vp);
	static JSBool JSSetProperty(JSContext *cx, JSObject *obj, jsval id,
			jsval *vp);
//	static JSBool JSConstructor(JSContext *cx, JSObject *obj, uintN argc,
//			jsval *argv, jsval *rval);
//	static void JSDestructor(JSContext *cx, JSObject *obj);
//
//	static JSObject *JSInit(JSContext *cx, JSObject *obj, JSObject *proto = NULL);

	static JSPropertySpec _properties[];
	static JSFunctionSpec _methods[];

	// List of common properties that are available in many elements
	/*
	tracking contextMenuItems
	hAlign
	height
	hOffset
	id
	firstChild
	lastChild
	name
	nextSibling
	previousSibling
	onClick
	onContextMenu
	onDragDrop
	onDragEnter
	onDragExit
	onMouseDown
	onMouseDrag
	onMouseEnter
	onMouseExit
	onMouseMove
	onMouseUp
	onMouseWheel
	onMultiClick
	opacity
	parentNode
	style
	subviews
	superview
	tooltip
	tracking
	vAlign
	visible
	vOffset
	width
	window
	zOrder

	 */


	// Property enum
	enum {name_prop, width_prop, height_prop, voffset_prop, hoffset_prop};
	
	// List of common functions that are available in many elements
	/*
	addSubview()
	appendChild()
	convertPointFromParent()
	convertPointFromWindow()
	convertPointToParent()
	convertPointToWindow()
	getElementById()
	orderAbove()
	orderBelow()
	removeChild()
	removeFromSuperview()
	saveImageToFile()
	*/

	// Function enum
	
	



protected:
	void setWidgetElement(WidgetElement* pWidgetElement) {
		m_pWidgetElement= pWidgetElement;
	}
	WidgetElement* getWidgetElement() {
		return m_pWidgetElement;
	}
private:
	WidgetElement* m_pWidgetElement;
};


#endif /*JSWIDGETELEMENT_H_*/
