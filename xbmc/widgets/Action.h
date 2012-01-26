#ifndef ACTION_H_
#define ACTION_H_

#include <string>
#include "WidgetElement.h"

using namespace std;

class Action : public WidgetElement
{

public :

	enum Trigger {
		onGainFocus,
		onIdle,
		onKeyDown,
		onKeyUp,
		onKonsposeActivated,
		onKonsposeDeactivated,
		onLoad,
		onLoseFocus,
		onMouseDown,
		onMouseEnter,
		onMouseExit,
		onMouseUp,
		onPreferencesChanged,
		onRunCommandInBgComplete,
		onScreenChanged,
		onTellWidget,
		onTimer,
		onUnload,
		onWakeFromSleep,
		onWillChangePreferences,
		onYahooLoginChanged,
		UNDEFINED
	};

	static const int NO_INTERVAL = -1;

private:

	Trigger m_Trigger;
	string m_iInterval;
	string m_strFile;
	// The JavaScript inside an action
	string m_strScript;

public:
	Action();
	virtual ~Action();

	virtual void LoadFromXml(TiXmlElement* pWidgetElement);
	
	const string& getScript() {
		return m_strScript;
	}
	
	Trigger parseActionTrigger(const string & strTrigger);

	void LoadJSObject(JSContext* cx, JSObject* obj);
	
};

#endif /*ACTION_H_*/
