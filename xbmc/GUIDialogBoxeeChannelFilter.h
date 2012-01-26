
#ifndef CGUIDIALOGBOXEECHANNELFILTER_H_
#define CGUIDIALOGBOXEECHANNELFILTER_H_

#include "GUIDialog.h"
#include "FileItem.h"
#include "Thread.h"

#include <set>

class CGUIDialogBoxeeChannelFilter : public CGUIDialog
{
public:

  CGUIDialogBoxeeChannelFilter();
  virtual ~CGUIDialogBoxeeChannelFilter();

  virtual void OnInitWindow();
  virtual void OnDeinitWindow(int nextWindowID);

  virtual bool OnMessage(CGUIMessage& message);
  virtual bool OnAction(const CAction& action);
  
  static bool Show();

protected:

private:

  bool LoadExcludedChannels();
  bool OnClick(CGUIMessage& message);

  std::set<std::string> m_excludedChannels;

  bool m_dirty;

  class CPostExcludedSorcesRequestJob : public IRunnable
  {
  public:

    CPostExcludedSorcesRequestJob(const std::string& excludedChannelsStr);
    virtual void Run();

  private:

    CStdString m_excludedChannelsStr;
  };
};

#endif /* CGUIDIALOGBOXEECHANNELFILTER_H_ */

