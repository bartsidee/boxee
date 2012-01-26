#ifndef GUIDIALOGBOXEECHAPTERS_H
#define GUIDIALOGBOXEECHAPTERS_H

#include "GUIDialog.h"
#include "cores/dvdplayer/DVDDemuxers/DVDDemux.h"

class CGUIDialogBoxeeChapters : public CGUIDialog
{
public:
  CGUIDialogBoxeeChapters();
  virtual ~CGUIDialogBoxeeChapters();

  static bool Show(const std::vector<DemuxChapterInfo>& chapters, int& selectedChapterId, int currentSelection = -1);
  static bool Show(const std::vector<DemuxTitleInfo>& titles, int& selectedTitleId, bool bCanBrowse = false);
  virtual void OnInitWindow();
  virtual bool OnMessage(CGUIMessage& message);
  virtual bool OnAction(const CAction& action);

private:
  bool OnClick(CGUIMessage& message);
  void LoadItems();

  bool m_bConfirmed;
  int m_selectionId;
  int m_initSelection;
  std::vector<DemuxChapterInfo> m_chapters;
  std::vector<DemuxTitleInfo> m_titles;
  bool m_bAddBrowse;
};

#endif //GUIDIALOGBOXECHAPTERS_H
