#ifndef __INFOPAGE__H__
#define __INFOPAGE__H__

#include "utils/Thread.h"
#include "StringUtils.h"
#include <map>

// This class used to register and display debug pages.
// Page should inherit InfoPageble and register itself in the singleton 
// It can also be used to create actions in different modules.

// Page indexes.
#define INFOPAGE_EXIT 0
#define INFOPAGE_APPLICATION 1
#define INFOPAGE_GUISETTINGS 2
#define INFOPAGE_X11         3
#define INFOPAGE_CSETTINGS   4



// ...


class InfoPageble
{
  public:
  virtual ~InfoPageble() {}
    virtual void GetInfoPage(CStdString *str, CStdString params) {
      *str = "GetInfoPage function not implemented.\n";
    };
};
 

class InfoPage : public CThread
{
public:
  virtual ~InfoPage();
  void Register(int index, CStdString descr, InfoPageble* p);
  void UnRegister(int index);
  virtual void Process();  
  InfoPage();  
protected:
  void PrintMenu();
  // for index keep page pointer and description.
  std::map<int, std::pair<InfoPageble*, CStdString> > pages;
};

#endif //__INFOPAGE__H__

