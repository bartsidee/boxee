#ifndef GUILIB_GUIMultilineEditControl_H
#define GUILIB_GUIMultilineEditControl_H

#pragma once

#include "GUIEditControl.h"

class CGUIMultilineEditControl : public CGUIEditControl
{
public:

  CGUIMultilineEditControl(int parentID, int controlID, float posX, float posY,
                  float width, float height, const CTextureInfo &textureFocus, const CTextureInfo &textureNoFocus,
                  const CLabelInfo& labelInfo, const std::string &text, const CLabelInfo& emptylabelInfo, const std::string &emptyText, int maxTextSize = -1);
  virtual bool OnAction(const CAction &action);
  void RecalcLabelPosition();

protected:
  virtual void RenderText();
  unsigned int GlobalTextCursorToLocal(unsigned int& cursor);
private:

};

#endif
