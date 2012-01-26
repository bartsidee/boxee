/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "GUISettings.h"
#include "GUIDialogKeyboard.h"
#include "GUILabelControl.h"
#include "GUIButtonControl.h"
#include "GUIEditControl.h"
#include "GUIDialogNumeric.h"
#include "GUIDialogOK.h"
#include "GUIUserMessages.h"
#include "GUIWindowManager.h"
#include "utils/RegExp.h"
#include "GUIPassword.h"
#include "utils/md5.h"
#include "utils/TimeUtils.h"
#include "xbox/XKGeneral.h"
#include "Application.h"
#include "Clipboard.h"
#include "AdvancedSettings.h"
#include "LocalizeStrings.h"
#include "AdvancedSettings.h"
#include "LocalizeStrings.h"
#include "StringUtils.h"
#include "GUIButtonControl.h"
#include "lib/libBoxee/bxconfiguration.h"
#include "lib/libBoxee/boxee.h"
#include "GUIDialogOK2.h"
#include "GUIDialogProgress.h"

#ifdef __APPLE__
#include "CocoaInterface.h"
#endif

#define USERNAME_REGEXP_PATTERN        "^[a-zA-Z0-9_.]*$"

// Symbol mapping (based on MS virtual keyboard - may need improving)
static char symbol_map[33] = ")!@#$%^&*([]{}-_=+;:\'\",.<>/?\\|`~";

#define FIRST_LINE 351
#define LAST_LINE  356

#define CTL_BUTTON_DONE       302
#define CTL_BUTTON_CAPS       303
#define CTL_BUTTON_SYMBOLS    304
#define CTL_BUTTON_IP_ADDRESS 307
#define CTL_BUTTON_SPACE      32

#define CTL_LABEL_HEADING     311

#define CTL_BUTTON_BACKSPACE    8

#define SEARCH_DELAY         1000
#define REMOTE_SMS_DELAY     1000

#define KEYS_IN_ROW           9

#define SYMBOLS_ON               "symbols-on"
#define CAPS_ON               "caps-on"
#define THREE_LINES           "3-lines"
#define FOUR_LINES            "4-lines"


using namespace BOXEE;

CGUIDialogKeyboard::CGUIDialogKeyboard(void)
: CGUIDialog(WINDOW_DIALOG_KEYBOARD, "DialogKeyboard.xml")
{
  m_bIsConfirmed = false;
  m_bShift = false;
  m_hiddenInput = false;
  m_filtering = FILTERING_NONE;
  m_keyType = LOWER;
  m_strHeading = "";
  m_lastRemoteClickTime = 0;
  m_pEdit = NULL;
}

CGUIDialogKeyboard::CGUIDialogKeyboard(DWORD dwID, const CStdString &xmlFile)
: CGUIDialog(dwID, xmlFile)
{
  m_bIsConfirmed = false;
  m_bShift = false;
  m_hiddenInput = false;
  m_filtering = FILTERING_NONE;
  m_keyType = LOWER;
  m_strHeading = "";
  m_lastRemoteClickTime = 0;
  m_pEdit = NULL;
}

CGUIDialogKeyboard::~CGUIDialogKeyboard(void)
{}

void CGUIDialogKeyboard::OnInitWindow()
{
  CGUIDialog::OnInitWindow();

  m_bIsConfirmed = false;

  // set alphabetic (capitals)
  UpdateButtons();

  m_pEdit = ((CGUIEditControl*)GetControl(CTL_LABEL_EDIT));
  if (m_pEdit)
  {
    // Set existing text into the control
    CStdString utf8String;
    g_charsetConverter.wToUTF8(m_strEdit, utf8String);
    m_pEdit->SetLabel2(utf8String);
    
    if (m_hiddenInput)
    {
      m_pEdit->SetInputType(CGUIEditControl::INPUT_TYPE_PASSWORD, 0);
    }
  }

  // set heading
  if (!m_strHeading.IsEmpty())
  {
    SET_CONTROL_LABEL(CTL_LABEL_HEADING, m_strHeading);
    SET_CONTROL_VISIBLE(CTL_LABEL_HEADING);
  }
  else
  {
    SET_CONTROL_HIDDEN(CTL_LABEL_HEADING);
  }

  // We dont need to set the focus on the edit control, remove this comment
  SET_CONTROL_FOCUS(CTL_LABEL_EDIT, 1);
  
  if (!m_strDoneText.IsEmpty())
  {
    SET_CONTROL_LABEL(CTL_BUTTON_DONE,m_strDoneText);
}
  else
  {
    SET_CONTROL_LABEL(CTL_BUTTON_DONE,g_localizeStrings.Get(20177));
  } 
}

bool CGUIDialogKeyboard::OnAction(const CAction &action)
{
  CGUIEditControl* pEdit = ((CGUIEditControl*)GetControl(CTL_LABEL_EDIT));
  if (!pEdit)
  {
    return false;
  }
  
  if ( pEdit->HasFocus() && ( action.id == ACTION_ENTER || action.id == ACTION_SELECT_ITEM || action.id == ACTION_NEXT_CONTROL))
  {
    OnOK();
    return true;
  }
  else if (action.id == ACTION_BUILT_IN_FUNCTION)
  {
    return true;  // pretend we handled it
  }
  else if (action.id == ACTION_SYMBOLS)
  {
    OnSymbols();
    return true;
  }
  else if (action.id == ACTION_PASTE)
  {
    CStdString currentText = m_pEdit->GetLabel2();
    CStdString paste;
    if (CClipboard::Paste(paste))
    {
      m_pEdit->SetLabel2(currentText + paste);
    }

    return true;
  }
  else if (action.id >= REMOTE_0 && action.id <= REMOTE_9)
  {
    OnRemoteNumberClick(action.id);
    return true;
  }
  else if (action.id >= KEY_VKEY && action.id < KEY_ASCII)
  { // input from the keyboard (vkey, not ascii)
    
    // return focus to the edit control
    SET_CONTROL_FOCUS(CTL_LABEL_EDIT, 1);
    
    uint8_t b = action.id & 0xFF;
    if (b == 0x25) 
    {
      return pEdit->OnAction(action); // left
    }
    else if (b == 0x27) 
    {
      return pEdit->OnAction(action); // right
    }
    else if (b == 0x0D) 
    {
      OnOK();         // enter
    }
    else if (b == 0x08) 
    {
      return pEdit->OnAction(action); // backspace
    }
    else if (b == 0x1B) 
    {
      Close();        // escape
    }
    else if (b == 0x20) 
    {
      Character(b);   // space
    }
    return true;
  }
  else if (action.id >= KEY_ASCII)
  { // input from the keyboard
    
    // return focus to the edit control
    SET_CONTROL_FOCUS(CTL_LABEL_EDIT, 1);

    switch (action.unicode)
    {
    case 13:  // enter
    case 10:  // enter
      OnOK();
      break;
    case 8:   // backspace
      Backspace();
      break;
    case 27:  // escape
      Close();
      break;
    default:  //use character input
      {
      //Highlight(action.id & 0xFF);
        if (action.unicode < 128)
        {
          CAction a(action);
          CStdString q="a";
          q[0]= (char)action.unicode;
          CStdString qwertyChar = g_application.GetKeyboards().GetActiveKeyboard().GetQwertyCharacter(q); 

          CStdStringW newStr;
          g_charsetConverter.utf8ToW(qwertyChar, newStr);
          
          a.unicode = newStr[0];
          return m_pEdit->OnAction(a);
    }
        else
        {
          return m_pEdit->OnAction(action);
        }
      }
    }
    return true;
  }
  return CGUIDialog::OnAction(action);
}

bool CGUIDialogKeyboard::OnMessage(CGUIMessage& message)
{
  CGUIDialog::OnMessage(message);

  int nMsgID = message.GetMessage();
  int iControl = message.GetSenderId();

  switch ( nMsgID )
  {
  case GUI_MSG_WINDOW_DEINIT:
  {
    m_strDoneText.Empty();
    break;
  }
  case GUI_MSG_CLICKED:
  {
    switch (iControl)
    {
    case CTL_BUTTON_DONE:
      OnOK();
      break;

    case CTL_BUTTON_CAPS:
      if (m_keyType == LOWER)
      {
        m_keyType = CAPS;
        SetProperty(CAPS_ON, true);
      }
      else if (m_keyType == CAPS)
      {
        m_keyType = LOWER;
        SetProperty(CAPS_ON, false);
      }
      UpdateButtons();
      break;

    case CTL_BUTTON_SYMBOLS:
      OnSymbols();
      break;

    case CTL_BUTTON_IP_ADDRESS:
      OnIPAddress();
      break;

    case CTL_LABEL_EDIT:
      break; 

    case CTL_BUTTON_BACKSPACE:
      Backspace();
      break;

    default:
      m_lastRemoteKeyClicked = 0;
      OnClickButton(iControl);
      break;
    }
  }
  break;
  }

  return true;
}

void CGUIDialogKeyboard::SetDoneText(const CStdString& aTextString)
{
  m_strDoneText = aTextString;
}

void CGUIDialogKeyboard::SetText(const CStdString& aTextString)
{
  m_strEdit.Empty();
  g_charsetConverter.utf8ToW(aTextString, m_strEdit);
}

CStdString CGUIDialogKeyboard::GetText() const
{
  CStdString utf8String;
  g_charsetConverter.wToUTF8(m_strEdit, utf8String);
  return utf8String;
}

CStdString CGUIDialogKeyboard::GetCurrentText() const
{
  const CGUIEditControl* pEdit = ((const CGUIEditControl*)GetControl(CTL_LABEL_EDIT));
  if (pEdit)
  {
    return pEdit->GetLabel2();
  }
  return StringUtils::EmptyString;
}

void CGUIDialogKeyboard::Character(WCHAR ch)
{
  if (!ch) return;

  //Character(action.unicode);
  CAction action;
  action.id = KEY_UNICODE;
  action.unicode = ch;
  m_pEdit->OnAction(action);
}

void CGUIDialogKeyboard::Render()
{
  // reset the hide state of the label when the remote
  // sms style input times out
  if (m_lastRemoteClickTime && m_lastRemoteClickTime + REMOTE_SMS_DELAY < CTimeUtils::GetFrameTime())
  {
    // finished inputting a sms style character - turn off our shift and symbol states
    ResetShiftAndSymbols();
  }
  CGUIDialog::Render();
}

void CGUIDialogKeyboard::UpdateLabel() // FIXME seems to be called twice for one USB SDL keyboard action/character
{
  CGUIEditControl* pEdit = ((CGUIEditControl*)GetControl(CTL_LABEL_EDIT));
  if (pEdit)
  {
    CStdStringW edit = m_strEdit;
    if (m_hiddenInput)
    { // convert to *'s
      edit.Empty();
      if (m_lastRemoteClickTime + REMOTE_SMS_DELAY > CTimeUtils::GetFrameTime() && m_strEdit.size())
      { // using the remove to input, so display the last key input
        edit.append(m_strEdit.size() - 1, L'*');
        edit.append(1, m_strEdit[m_strEdit.size() - 1]);
      }
      else
        edit.append(m_strEdit.size(), L'*');
    }

    // convert back to utf8
    CStdString utf8Edit;
    g_charsetConverter.wToUTF8(edit, utf8Edit);
    m_pEdit->SetLabel2(utf8Edit);

    // Send off a search message
    unsigned int now = CTimeUtils::GetFrameTime();
    // don't send until the REMOTE_SMS_DELAY has passed
    if (m_lastRemoteClickTime && m_lastRemoteClickTime + REMOTE_SMS_DELAY >= now)
      return;
    if (m_filtering == FILTERING_CURRENT)
    { // send our filter message
      CGUIMessage message(GUI_MSG_NOTIFY_ALL, GetID(), 0, GUI_MSG_FILTER_ITEMS);
      message.SetStringParam(utf8Edit);
      g_windowManager.SendMessage(message);
    }
    if (m_filtering == FILTERING_SEARCH)
    { // send our search message
      CGUIMessage message(GUI_MSG_NOTIFY_ALL, GetID(), 0, GUI_MSG_SEARCH_UPDATE);
      message.SetStringParam(utf8Edit);
      g_windowManager.SendMessage(message);
    }
  }
}

void CGUIDialogKeyboard::Backspace()
{
  CAction action;
  action.id = ACTION_BACKSPACE;
  m_pEdit->OnAction(action);
}

void CGUIDialogKeyboard::OnClickButton(int iButtonControl)
{
  CStdStringW newString;
  if (iButtonControl == CTL_BUTTON_SPACE)
  {
    newString = L" ";
  }
  else
  {
    CGUIButtonControl *btn = (CGUIButtonControl *)GetControl(iButtonControl);
    if (btn && btn->GetControlType() == GUICONTROL_BUTTON)
    {
      CStdString c = btn->GetLabel();    
      g_charsetConverter.utf8ToW(c, newString);
  }
    else
    {
      return;
}
  }

  if (newString.size() > 1)
  {
    CStdStringW text = m_pEdit->GetUnicodeLabel();
    int position = m_pEdit->GetCursorPosition();
    text.Insert(position, newString);
    
    CStdString utf8String;
    g_charsetConverter.wToUTF8(text, utf8String);
  
    m_pEdit->SetLabel2(utf8String);
    m_pEdit->SetCursorPosition(position + newString.size());
  }
  else
  {
    Character(newString[0]);
  }
}

void CGUIDialogKeyboard::OnRemoteNumberClick(int key)
{
  unsigned int now = CTimeUtils::GetFrameTime();

  if (m_lastRemoteClickTime)
  { // a remote key has been pressed
    if (key != m_lastRemoteKeyClicked || m_lastRemoteClickTime + REMOTE_SMS_DELAY < now)
    { // a different key was clicked than last time, or we have timed out
      m_lastRemoteKeyClicked = key;
      m_indexInSeries = 0;
      // reset our shift and symbol states, and update our label to ensure the search filter is sent
      ResetShiftAndSymbols();
      UpdateLabel();
    }
    else
    { // same key as last time within the appropriate time period
      m_indexInSeries++;
      Backspace();
    }
  }
  else
  { // key is pressed for the first time
    m_lastRemoteKeyClicked = key;
    m_indexInSeries = 0;
  }

  int arrayIndex = key - REMOTE_0;
  m_indexInSeries = m_indexInSeries % strlen(s_charsSeries[arrayIndex]);
  m_lastRemoteClickTime = now;

  // Select the character that will be pressed
  const char* characterPressed = s_charsSeries[arrayIndex];
  characterPressed += m_indexInSeries;

  // use caps where appropriate
  char ch = *characterPressed;
  bool caps = (m_keyType == CAPS && !m_bShift) || (m_keyType == LOWER && m_bShift);
  if (!caps && *characterPressed >= 'A' && *characterPressed <= 'Z')
    ch += 32;
  Character(ch);
}

void CGUIDialogKeyboard::Highlight(char ch)
{
  // TODO: Not fully implemented
  SET_CONTROL_FOCUS(ch,1);
}

WCHAR CGUIDialogKeyboard::GetCharacter(int iButton)
{
  if (iButton == CTL_BUTTON_SPACE)
  {
    return 32; // Space
    }
  
  CGUIButtonControl *btn = (CGUIButtonControl *)GetControl(iButton);
  if (btn && btn->GetControlType() == GUICONTROL_BUTTON)
  {
    CStdString c = btn->GetLabel();    
    CStdStringW newStr;
    g_charsetConverter.utf8ToW(c, newStr);
    return newStr[0];
    }
  
  return 0;
}

void CGUIDialogKeyboard::UpdateButtons()
{
  if (m_keyType == CAPS)
  {
    CGUIMessage msg(GUI_MSG_SELECTED, GetID(), CTL_BUTTON_CAPS);
    OnMessage(msg);
  }
  else
  {
    CGUIMessage msg(GUI_MSG_DESELECTED, GetID(), CTL_BUTTON_CAPS);
    OnMessage(msg);
  }
  if (m_keyType == SYMBOLS)
  {
    CGUIMessage msg(GUI_MSG_SELECTED, GetID(), CTL_BUTTON_SYMBOLS);
    OnMessage(msg);
  }
  else
  {
    CGUIMessage msg(GUI_MSG_DESELECTED, GetID(), CTL_BUTTON_SYMBOLS);
    OnMessage(msg);
  }
  char szLabel[2];
  szLabel[0] = 32;
  szLabel[1] = 0;
  CStdString aLabel = szLabel;

  // set numerals
  for (int iButton = 48; iButton <= 57; iButton++)
  {
    if (m_keyType == SYMBOLS)
      aLabel[0] = symbol_map[iButton - 48];
    else
      aLabel[0] = iButton;
    SetControlLabel(iButton, aLabel);
  }

  // hide all character-buttons and only show the relevant ones (according to the language)
  // character buttons are numbered 65-120
  for (int iButton = 65; iButton <= 120; iButton++)
  {
    SET_CONTROL_HIDDEN(iButton);
  }

  // set correct alphabet characters...
  int numOfKeys = ((m_keyType == SYMBOLS)?90:(65+g_application.GetKeyboards().GetActiveKeyboard().GetMaxID())) - 64;

  for (int iButton = 65; iButton <= ((m_keyType == SYMBOLS)?90:(65+g_application.GetKeyboards().GetActiveKeyboard().GetMaxID())); iButton++)
  {
    SET_CONTROL_VISIBLE(iButton);
    
    // set the correct case...
    if ((m_keyType == CAPS && m_bShift) || (m_keyType == LOWER && !m_bShift))
    { // make lower case
      aLabel = g_application.GetKeyboards().GetActiveKeyboard().GetCharacter(iButton - 65,false);
    }
    else if (m_keyType == SYMBOLS)
    {
      aLabel[0] = symbol_map[iButton - 65 + 10];
    }
    else
    {
      aLabel = g_application.GetKeyboards().GetActiveKeyboard().GetCharacter(iButton - 65,true);
    }
    SetControlLabel(iButton, aLabel);
  }
  
  // add extra keys for foreign languages
  int numOfEmptyKeys = KEYS_IN_ROW - (numOfKeys % KEYS_IN_ROW);
  if (numOfEmptyKeys == KEYS_IN_ROW)
    numOfEmptyKeys = 0;
  for (int iButton = 65 + numOfKeys; iButton < 65 + numOfKeys + numOfEmptyKeys; iButton++)
  {
    SET_CONTROL_VISIBLE(iButton);
    SetControlLabel(iButton, "");
  }

  if (numOfKeys>36)
  {
    SetProperty(THREE_LINES, false);
    SetProperty(FOUR_LINES, false);
  }
  else if (numOfKeys>27 && numOfKeys<=36)
  {
    SetProperty(THREE_LINES, false);
    SetProperty(FOUR_LINES, true);
  }
  else
  {
    SetProperty(THREE_LINES, true);
    SetProperty(FOUR_LINES, false);
  }

  HideShowButtonLines();
  }

// Show keyboard with initial value (aTextString) and replace with result string.
// Returns: true  - successful display and input (empty result may return true or false depending on parameter)
//          false - unsucessful display of the keyboard or cancelled editing
bool CGUIDialogKeyboard::ShowAndGetInput(CStdString& aTextString, const CStdString &strHeading, bool allowEmptyResult, bool hiddenInput /* = false */)
{
  CGUIDialogKeyboard *pKeyboard = (CGUIDialogKeyboard*)g_windowManager.GetWindow(WINDOW_DIALOG_KEYBOARD);

  if (!pKeyboard)
    return false;

  // setup keyboard
  pKeyboard->Initialize();
  pKeyboard->CenterWindow();
  pKeyboard->SetHeading(strHeading);
  pKeyboard->SetHiddenInput(hiddenInput);
  pKeyboard->SetText(aTextString);
  
  // do this using a thread message to avoid render() conflicts
  ThreadMessage tMsg(TMSG_DIALOG_DOMODAL, WINDOW_DIALOG_KEYBOARD, g_windowManager.GetActiveWindow());
  g_application.getApplicationMessenger().SendMessage(tMsg, true);
  pKeyboard->Close();

  // If have text - update this.
  if (pKeyboard->IsConfirmed())
  {
    aTextString = pKeyboard->GetText();
    if (!allowEmptyResult && aTextString.IsEmpty())
    {
      return false;
    }

    return true;
  }
  else
  {
    return false;
  }
}

bool CGUIDialogKeyboard::ShowAndGetInput(CStdString& aTextString, bool allowEmptyResult)
{
  return ShowAndGetInput(aTextString, "", allowEmptyResult) != 0;
}

// Shows keyboard and prompts for a password.
// Differs from ShowAndVerifyNewPassword() in that no second verification is necessary.
bool CGUIDialogKeyboard::ShowAndGetNewPassword(CStdString& newPassword, const CStdString &heading, bool allowEmpty)
{
  return ShowAndGetInput(newPassword, heading, allowEmpty, true);
}

// Shows keyboard and prompts for a password.
// Differs from ShowAndVerifyNewPassword() in that no second verification is necessary.
bool CGUIDialogKeyboard::ShowAndGetNewPassword(CStdString& newPassword)
{
  CStdString heading = g_localizeStrings.Get(12340);
  return ShowAndGetNewPassword(newPassword, heading, false);
}

// \brief Show keyboard twice to get and confirm a user-entered password string.
// \param newPassword Overwritten with user input if return=true.
// \param heading Heading to display
// \param allowEmpty Whether a blank password is valid or not.
// \return true if successful display and user input entry/re-entry. false if unsucessful display, no user input, or canceled editing.
bool CGUIDialogKeyboard::ShowAndVerifyNewPassword(CStdString& newPassword, const CStdString &heading, bool allowEmpty)
{
  // Prompt user for password input
  CStdString userInput = "";
  if (!ShowAndGetInput(userInput, heading, allowEmpty, true))
  { // user cancelled, or invalid input
    return false;
  }
  // success - verify the password
  CStdString checkInput = "";
  if (!ShowAndGetInput(checkInput, g_localizeStrings.Get(12341), allowEmpty, true))
  { // user cancelled, or invalid input
    return false;
  }
  // check the password
  if (checkInput == userInput)
  {
    XBMC::MD5 md5state;
    md5state.append(userInput);
    md5state.getDigest(newPassword);
    newPassword.ToLower();
    return true;
  }
  CGUIDialogOK::ShowAndGetInput(12341, 12344, 0, 0);
  return false;
}

// \brief Show keyboard twice to get and confirm a user-entered password string.
// \param strNewPassword Overwritten with user input if return=true.
// \return true if successful display and user input entry/re-entry. false if unsucessful display, no user input, or canceled editing.
bool CGUIDialogKeyboard::ShowAndVerifyNewPassword(CStdString& newPassword)
{
  CStdString heading = g_localizeStrings.Get(12340);
  return ShowAndVerifyNewPassword(newPassword, heading, false);
}

// \brief Show keyboard and verify user input against strPassword.
// \param strPassword Value to compare against user input.
// \param dlgHeading String shown on dialog title. Converts to localized string if contains a positive integer.
// \param iRetries If greater than 0, shows "Incorrect password, %d retries left" on dialog line 2, else line 2 is blank.
// \return 0 if successful display and user input. 1 if unsucessful input. -1 if no user input or canceled editing.
int CGUIDialogKeyboard::ShowAndVerifyPassword(CStdString& strPassword, const CStdString& strHeading, int iRetries)
{
  CStdString strHeadingTemp;
  if (1 > iRetries && strHeading.size())
    strHeadingTemp = strHeading;
  else
    strHeadingTemp.Format("%s - %i %s", g_localizeStrings.Get(12326).c_str(), g_guiSettings.GetInt("masterlock.maxretries") - iRetries, g_localizeStrings.Get(12343).c_str());

  CStdString strUserInput = "";
  if (!ShowAndGetInput(strUserInput, strHeadingTemp, false, true))  //bool hiddenInput = false/true ? TODO: GUI Setting to enable disable this feature y/n?
    return -1; // user canceled out

  if (!strPassword.IsEmpty())
  {
    if (strPassword == strUserInput)
      return 0;

    CStdString md5pword2;
    XBMC::MD5 md5state;
    md5state.append(strUserInput);
    md5state.getDigest(md5pword2);
    if (strPassword.Equals(md5pword2))
      return 0;     // user entered correct password
    else return 1;  // user must have entered an incorrect password
  }
  else
  {
    if (!strUserInput.IsEmpty())
    {
      XBMC::MD5 md5state;
      md5state.append(strUserInput);
      md5state.getDigest(strPassword);
      strPassword.ToLower();
      return 0; // user entered correct password
    }
    else return 1;
  }
}

bool CGUIDialogKeyboard::ShowAndGetNewUsername(CStdString& customUsername)
{
  CLog::Log(LOGDEBUG,"CGUIDialogKeyboard::ShowAndGetNewUsername - Enter function with [customUsername=%s] (cnu)",customUsername.c_str());

  CGUIDialogKeyboard* pKeyboard = (CGUIDialogKeyboard*)g_windowManager.GetWindow(WINDOW_DIALOG_KEYBOARD);

  if (!pKeyboard)
  {
    CLog::Log(LOGERROR,"CGUIDialogKeyboard::ShowAndGetNewUsername - FAILED to get CGUIDialogKeyboard object (cnu)");
    return false;
  }

  // setup keyboard
  pKeyboard->Initialize();
  pKeyboard->CenterWindow();
  pKeyboard->SetHeading(g_localizeStrings.Get(1014));
  pKeyboard->SetHiddenInput(false);
  pKeyboard->SetText(customUsername);

  bool exit = false;

  while(!exit)
  {
    // do this using a thread message to avoid render() conflicts
    ThreadMessage tMsg(TMSG_DIALOG_DOMODAL, WINDOW_DIALOG_KEYBOARD, g_windowManager.GetActiveWindow());
    g_application.getApplicationMessenger().SendMessage(tMsg, true);

    if (!pKeyboard->IsConfirmed())
    {
      CLog::Log(LOGERROR,"CGUIDialogKeyboard::ShowAndGetNewUsername - Keyboard was canceled (cnu)");
      customUsername = "";
      exit = true;
      continue;
    }

    customUsername = pKeyboard->GetText();

    // verify the username is valid
    CStdString strRegExpr = USERNAME_REGEXP_PATTERN;
    CStdString strToCheck = customUsername;
    strToCheck.ToLower();

    CRegExp reg;
    reg.RegComp(strRegExpr);
    int pos = reg.RegFind(strToCheck);

    if (pos == -1)
    {
      CStdString message;
      message.Format(g_localizeStrings.Get(53445),customUsername);
      CGUIDialogOK2::ShowAndGetInput(g_localizeStrings.Get(53701),message);
      continue;
    }

    // verify the username is available
    BXXMLDocument doc;
    doc.SetCredentials(BOXEE::Boxee::GetInstance().GetCredentials());

    CStdString strUrl = BXConfiguration::GetInstance().GetStringParam("Boxee.Check.New.Username","http://app.boxee.tv/api/checkid?userid=");
    strUrl += customUsername;

    CGUIDialogProgress* progress = (CGUIDialogProgress *)g_windowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
    if (progress)
    {
      progress->StartModal();
      progress->Progress();
    }

    bool checkSucceeded = doc.LoadFromURL(strUrl);

    if (progress)
    {
      progress->Close();
    }

    if (!checkSucceeded)
    {
      CStdString message;
      message.Format(g_localizeStrings.Get(53447),customUsername);
      CGUIDialogOK2::ShowAndGetInput(g_localizeStrings.Get(53701),message);
      continue;
    }

    const TiXmlElement* pRootElement = doc.GetDocument().RootElement();
    if (!pRootElement)
    {
      CLog::Log(LOGERROR,"CGUIDialogKeyboard::ShowAndGetNewUsername - FAILED to get root element (cnu)");
      return false;
    }

    if (strcmp(pRootElement->Value(),"boxee") != 0)
    {
      CLog::Log(LOGERROR,"CGUIDialogKeyboard::ShowAndGetNewUsername - Root element isn't <boxee>. <%s> (cnu)",pRootElement->Value());
      return false;
    }

    bool isUsernameAvailable = false;

    const TiXmlNode* pTag = NULL;
    while ((pTag = pRootElement->IterateChildren(pTag)))
    {
      CStdString elementName = pTag->ValueStr();

      if (elementName == "exists")
      {
        const TiXmlNode* pValue = pTag->FirstChild();

        if (pValue)
        {
          CStdString existValue = pValue->ValueStr();

          CLog::Log(LOGDEBUG,"CGUIDialogKeyboard::ShowAndGetNewUsername - Server returned <exist=%s> for [username=%s] (cnu)",existValue.c_str(),customUsername.c_str());

          if (existValue.Equals("0"))
          {
            isUsernameAvailable = true;
          }
          else
          {
            CLog::Log(LOGERROR,"CGUIWindowLoginScreen::ParseServerCreatedNewUserResponse - Server returned <exist=%s> for [username=%s] (cnu)",existValue.c_str(),customUsername.c_str());

            CStdString message;
            message.Format(g_localizeStrings.Get(53446),customUsername);
            CGUIDialogOK2::ShowAndGetInput(g_localizeStrings.Get(53701),message);
            continue;
          }
        }
        else
        {
          CLog::Log(LOGERROR,"CGUIWindowLoginScreen::ParseServerCreatedNewUserResponse - FAILED to get <exist> element value. [username=%s] (cnu)",customUsername.c_str());

          CStdString message;
          message.Format(g_localizeStrings.Get(53447),customUsername);
          CGUIDialogOK2::ShowAndGetInput(g_localizeStrings.Get(53701),message);
          continue;
        }
      }
    }

    if (isUsernameAvailable)
    {
      exit = true;
    }
  }

  CLog::Log(LOGDEBUG,"CGUIDialogKeyboard::ShowAndGetNewUsername - Exit function with [customUsername=%s] (cnu)",customUsername.c_str());

  return true;
}

void CGUIDialogKeyboard::Close(bool forceClose)
{
  // reset the heading (we don't always have this)
  m_strHeading = "";
  // call base class
  CGUIDialog::Close(forceClose);
}

void CGUIDialogKeyboard::OnSymbols()
{
  if (m_keyType == SYMBOLS)
  {
    m_keyType = LOWER;
    SetProperty(SYMBOLS_ON, false);
  }
  else
  {
    m_keyType = SYMBOLS;
    SetProperty(SYMBOLS_ON, true);
  }
  UpdateButtons();
}

void CGUIDialogKeyboard::OnShift()
{
  m_bShift = !m_bShift;
  UpdateButtons();
}

void CGUIDialogKeyboard::OnIPAddress()
{
  m_strEdit.Empty();

  // find any IP address in the current string if there is any
  // We match to #.#.#.#
  CStdString utf8String;
  g_charsetConverter.wToUTF8(m_strEdit, utf8String);
  CStdString ip;
  CRegExp reg;
  reg.RegComp("[0-9]+\\.[0-9]+\\.[0-9]+\\.[0-9]+");
  int start = reg.RegFind(utf8String.c_str());
  int length = 0;
  if (start > -1)
  {
    length = reg.GetSubLength(0);
    ip = utf8String.Mid(start, length);
  }
  else
    start = utf8String.size();

  if (CGUIDialogNumeric::ShowAndGetIPAddress(ip, m_strHeading))
  {
    utf8String = utf8String.Left(start) + ip + utf8String.Mid(start + length);
    g_charsetConverter.utf8ToW(utf8String, m_strEdit);
    UpdateLabel();
  }
}

void CGUIDialogKeyboard::ResetShiftAndSymbols()
{
  if (m_bShift) OnShift();
  if (m_keyType == SYMBOLS) OnSymbols();
  m_lastRemoteClickTime = 0;
}

const char* CGUIDialogKeyboard::s_charsSeries[10] = { " !@#$%^&*()[]{}<>/\\|0", ".,;:\'\"-+_=?`~1", "ABC2", "DEF3", "GHI4", "JKL5", "MNO6", "PQRS7", "TUV8", "WXYZ9" };

void CGUIDialogKeyboard::SetControlLabel(int id, const CStdString &label)
{ // find all controls with this id, and set all their labels
  CGUIMessage message(GUI_MSG_LABEL_SET, GetID(), id);
  message.SetLabel(label);
  for (unsigned int i = 0; i < m_children.size(); i++)
  {
    if (m_children[i]->GetID() == id || m_children[i]->IsGroup())
      m_children[i]->OnMessage(message);
  }
}

void CGUIDialogKeyboard::OnOK()
{
  const CGUIEditControl* pEdit = ((const CGUIEditControl*)GetControl(CTL_LABEL_EDIT));
  if (pEdit)
  {
    m_strEdit = pEdit->GetUnicodeLabel();
  }

  m_bIsConfirmed = true;

  Close();
}

bool CGUIDialogKeyboard::ShowAndGetFilter(CStdString &filter, bool searching)
{
  CGUIDialogKeyboard *pKeyboard = (CGUIDialogKeyboard*)g_windowManager.GetWindow(WINDOW_DIALOG_KEYBOARD);

  if (!pKeyboard)
    return false;

  pKeyboard->m_filtering = searching ? FILTERING_SEARCH : FILTERING_CURRENT;
  bool ret = ShowAndGetInput(filter, true);
  pKeyboard->m_filtering = FILTERING_NONE;
  return ret;
}

void CGUIDialogKeyboard::HideShowButtonLines()
{
  std::vector<CGUIControl *> controls;  
  for (int l = FIRST_LINE; l <= LAST_LINE; l++)
  {
    SET_CONTROL_HIDDEN(l);
    CGUIControlGroup* group = (CGUIControlGroup*) GetControl(l);
    if (group)
    {
      group->GetControls(controls);
      for (size_t i = 0; i < controls.size(); i++)
      {
        if (controls[i]->IsVisible())
        {
          SET_CONTROL_VISIBLE(l);
          break;
        }
      }
    }
  }   
}
