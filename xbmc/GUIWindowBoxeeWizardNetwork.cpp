#include "stdafx.h"
#include "GUIWindowManager.h"
#include "GUIDialogOK.h"
#include "GUIDialogKeyboard.h"
#include "GUIDialogProgress.h"
#include "GUIDialogOK.h"
#include "GUIWindowBoxeeWizardNetwork.h"
#include "Application.h"
#include "GUIButtonControl.h"
#include "GUIListContainer.h"

#define CONTROL_INTERFACES       50
#define CONTROL_SEP1             51
#define CONTROL_WIRELESS         55
#define CONTROL_SEP2             56
#define CONTROL_PASSWORD_GROUP   9030
#define CONTROL_PASSWORD         75
#define CONTROL_MANUAL           97
#define CONTROL_BACK             98
#define CONTROL_NEXT             99
#define CONTROL_ENC_GROUP        9040
#define CONTROL_ENC              9041
#define CONTROL_ENC_SELECTION    9042

using namespace std;

char* ENC_LABELS[] = { "None", "WEP (ASCII)", "WEP (HEX)", "WPA", "WPA2" };
EncMode ENC_MODES[] = { ENC_NONE, ENC_WEP_ASCII, ENC_WEP_HEX, ENC_WPA, ENC_WPA2 };

CGUIWindowBoxeeWizardNetwork::CGUIWindowBoxeeWizardNetwork(void)
    : CGUIDialog(WINDOW_BOXEE_WIZARD_NETWORK, "boxee_wizard_network.xml")
{
   m_interfaces = g_application.getNetwork().GetInterfaceList();
}

CGUIWindowBoxeeWizardNetwork::~CGUIWindowBoxeeWizardNetwork(void)
{
}

void CGUIWindowBoxeeWizardNetwork::OnInitWindow()
{  
   CGUIWindow::OnInitWindow();
   
   // Clear the list of encryption mehods and initialize it
   CGUIMessage msgReset(GUI_MSG_LABEL_RESET, GetID(), CONTROL_ENC);
   OnMessage(msgReset);

   for (int i = 0; i < 5; i++)
   {
      CFileItemPtr item ( new CFileItem(ENC_LABELS[i]) );
      CGUIMessage msg(GUI_MSG_LABEL_ADD, GetID(), CONTROL_ENC, 0, 0, item);
      OnMessage(msg);            
   }

   ShowInterfaces();

   SET_CONTROL_FOCUS(CONTROL_INTERFACES, 0);
   CONTROL_DISABLE(CONTROL_NEXT);   
}

bool CGUIWindowBoxeeWizardNetwork::OnAction(const CAction &action)
{
   int iControl = GetFocusedControlID();

   if (action.wID == ACTION_PREVIOUS_MENU || (action.wID == ACTION_SELECT_ITEM && iControl == CONTROL_BACK))
   {
      Close();
   }
   else if (action.wID == ACTION_SELECT_ITEM && iControl == CONTROL_INTERFACES)
   {  
     ShowWirelessNetworksIfNeeded();
     CGUIListContainer *pList = (CGUIListContainer *)GetControl(CONTROL_INTERFACES);
     if (pList)
       pList->SetSingleSelectedItem();

     return true;
   }
   else if (action.wID == ACTION_MOVE_LEFT && iControl == CONTROL_WIRELESS)
   {
     ShowInterfaces();
     SET_CONTROL_FOCUS(CONTROL_INTERFACES, 0);
     CGUIListContainer *pList = (CGUIListContainer *)GetControl(CONTROL_WIRELESS);
     if (pList)
       pList->SetSingleSelectedItem();

      return true;
   }
   else if (action.wID == ACTION_SELECT_ITEM && iControl == CONTROL_WIRELESS)
   {      
      ShowPasswordIfNeeded();
      CGUIListContainer *pList = (CGUIListContainer *)GetControl(CONTROL_WIRELESS);
      if (pList)
        pList->SetSingleSelectedItem();
      return true;
   }
   else if (action.wID == ACTION_SELECT_ITEM && iControl == CONTROL_PASSWORD)   
   {      
      CGUIButtonControl* passwordButton = (CGUIButtonControl*) GetControl(iControl);
      CStdString password = passwordButton->GetLabel();
      if (CGUIDialogKeyboard::ShowAndGetInput(password, g_localizeStrings.Get(789), false))
      {         
         passwordButton->SetLabel(password);
         CONTROL_ENABLE(CONTROL_NEXT);                        
         SET_CONTROL_FOCUS(CONTROL_NEXT, 0);
      }
      return true;
   }
   else if (action.wID == ACTION_SELECT_ITEM && iControl == CONTROL_ENC)
   {      
      CGUIButtonControl* encSelectionButton = (CGUIButtonControl*) GetControl(CONTROL_ENC_SELECTION);
   
      CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), CONTROL_ENC);
      OnMessage(msg);
      int iItem = msg.GetParam1();
      encSelectionButton->SetLabel(ENC_LABELS[iItem]);
      
      SET_CONTROL_HIDDEN(CONTROL_ENC);
      
      if (iItem == ENC_NONE)
      {
         SET_CONTROL_HIDDEN(CONTROL_PASSWORD_GROUP);
         SET_CONTROL_FOCUS(CONTROL_NEXT, 0);
      }
      else
      {
         SET_CONTROL_FOCUS(CONTROL_PASSWORD, 0);
      }
      return true;
   }
   else if (action.wID == ACTION_MOVE_LEFT && (iControl == CONTROL_ENC_SELECTION || iControl == CONTROL_PASSWORD))
   {
      SET_CONTROL_HIDDEN(CONTROL_ENC_GROUP);
      SET_CONTROL_HIDDEN(CONTROL_PASSWORD_GROUP);
      SET_CONTROL_FOCUS(CONTROL_WIRELESS, 0);
      return true;
   }
   else if (action.wID == ACTION_MOVE_DOWN && iControl == CONTROL_ENC_SELECTION)
   {
      if (GetControl(CONTROL_PASSWORD_GROUP)->IsVisible())
      {
         SET_CONTROL_FOCUS(CONTROL_PASSWORD, 0);
      }
      else if (!GetControl(CONTROL_NEXT)->IsDisabled())
      {
         SET_CONTROL_FOCUS(CONTROL_NEXT, 0);
      }
      return true;     
   }
   else if (action.wID == ACTION_MOVE_UP && (iControl == CONTROL_NEXT || iControl == CONTROL_BACK))
   {
      if (GetControl(CONTROL_PASSWORD_GROUP)->IsVisible())
      {
         SET_CONTROL_FOCUS(CONTROL_PASSWORD, 0);
      }
      else if (GetControl(CONTROL_ENC_GROUP)->IsVisible())
      {
         SET_CONTROL_FOCUS(CONTROL_ENC_SELECTION, 0);
      }
      else 
      {
         SET_CONTROL_FOCUS(CONTROL_INTERFACES, 0);
      }
      return true;     
   }
   else if (action.wID == ACTION_SELECT_ITEM && iControl == CONTROL_NEXT)
   {
      if (GetControl(CONTROL_ENC_SELECTION)->IsVisible() && GetControl(CONTROL_PASSWORD)->IsVisible())
      {
        CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), CONTROL_ENC);
        OnMessage(msg);
        int iItem = msg.GetParam1();
        
        CGUIButtonControl* passwordButton = (CGUIButtonControl*) GetControl(CONTROL_PASSWORD);
        CStdString password = passwordButton->GetLabel();
    
        if (ENC_MODES[iItem] == ENC_WEP_HEX && !IsHexString(password))
        {
          CGUIDialogOK *pDialogOK = (CGUIDialogOK *)m_gWindowManager.GetWindow(WINDOW_DIALOG_OK);
          pDialogOK->SetHeading("");
          pDialogOK->SetLine(0, 51018);
          pDialogOK->SetLine(1, 51019);
          pDialogOK->DoModal();

          return true;
        }
      }
               
      if (SaveConfiguration())
      {
         // Close all wizard dialogs
         Close();
         CGUIDialog* dialog = (CGUIDialog*) m_gWindowManager.GetWindow(WINDOW_BOXEE_WIZARD_AUDIO);
         dialog->Close();
         dialog = (CGUIDialog*) m_gWindowManager.GetWindow(WINDOW_BOXEE_WIZARD_RESOLUTION);
         dialog->Close();
      }
      
      return true;
   }
   
   return CGUIWindow::OnAction(action);
}

void CGUIWindowBoxeeWizardNetwork::ShowInterfaces()
{
   NetworkAssignment assignment;
   CStdString ipAddress;
   CStdString networkMask;
   CStdString defaultGateway;
   CStdString essId;
   CStdString key;
   EncMode encryptionMode;
   
   // Clear the list first
   CGUIMessage msgReset(GUI_MSG_LABEL_RESET, GetID(), CONTROL_INTERFACES);
   OnMessage(msgReset);
   
   m_interfaceItems.clear();   
   
   // Count how many interfaces of each type so we can append number in case there's more than one 
   int ethernetCount = 0;
   int wirelessCount = 0;
   for (unsigned int i = 0; i < m_interfaces.size(); i++)
   {
      if (m_interfaces[i]->IsWireless())
         wirelessCount++;
      else
         ethernetCount++;
   }
   
   // Go over all the interfaces and display them
   int iSelectedItem = 0;
   CStdString label;
   int ethernetCurrent = 1;
   int wirelessCurrent = 1;
   
   for (unsigned int i = 0; i < m_interfaces.size(); i++)
   {
      m_interfaces[i]->GetSettings(assignment, ipAddress, networkMask, defaultGateway, essId, key, encryptionMode);
      
      // Set the right label
      if (m_interfaces[i]->IsWireless() && wirelessCount == 1)
         label = "Wireless";
      else if (!(m_interfaces[i]->IsWireless()) && ethernetCount == 1) 
         label = "Ethernet";
      else if (m_interfaces[i]->IsWireless() && wirelessCount > 1)
      {
         label.Format("Wireless %d", wirelessCurrent);
         wirelessCurrent++;
      }
      else if (!(m_interfaces[i]->IsWireless()) && ethernetCount > 1)
      {
         label.Format("Ethernet %d", ethernetCurrent);
         ethernetCurrent++;
      }
                  
      CFileItemPtr item ( new CFileItem(label) );
      CGUIMessage msg(GUI_MSG_LABEL_ADD, GetID(), CONTROL_INTERFACES, 0, 0, item);
      OnMessage(msg);            

      m_interfaceItems.push_back(item);
      
      if (assignment != NETWORK_DISABLED)
        iSelectedItem = m_interfaces.size() - 1;            
   }
   
   CGUIMessage msg(GUI_MSG_ITEM_SELECT, GetID(), CONTROL_INTERFACES, iSelectedItem);
   OnMessage(msg);
   
   SET_CONTROL_HIDDEN(CONTROL_SEP1);
   SET_CONTROL_HIDDEN(CONTROL_WIRELESS);
   SET_CONTROL_HIDDEN(CONTROL_SEP2);
   SET_CONTROL_HIDDEN(CONTROL_PASSWORD_GROUP);
   SET_CONTROL_HIDDEN(CONTROL_ENC_GROUP);   
   
   CONTROL_DISABLE(CONTROL_NEXT);      
}

void CGUIWindowBoxeeWizardNetwork::ShowWirelessNetworksIfNeeded()
{
   CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), CONTROL_INTERFACES );
   OnMessage(msg);
   int iItem = msg.GetParam1();
   
   CNetworkInterface* interface = m_interfaces[iItem];
   
   if (interface->IsWireless())
   {
      SET_CONTROL_VISIBLE(CONTROL_WIRELESS);
      SET_CONTROL_VISIBLE(CONTROL_SEP1);
      ShowWirelessNetworks(interface);
      SET_CONTROL_FOCUS(CONTROL_WIRELESS, 0);
      CONTROL_DISABLE(CONTROL_NEXT);            
   }
   else
   {
      SET_CONTROL_HIDDEN(CONTROL_SEP1);
      SET_CONTROL_HIDDEN(CONTROL_WIRELESS);
      SET_CONTROL_HIDDEN(CONTROL_SEP2);
      SET_CONTROL_HIDDEN(CONTROL_PASSWORD_GROUP);
      SET_CONTROL_HIDDEN(CONTROL_ENC_GROUP);
      CONTROL_ENABLE(CONTROL_NEXT);            
      SET_CONTROL_FOCUS(CONTROL_NEXT, 0);
   }
}

void CGUIWindowBoxeeWizardNetwork::ShowWirelessNetworks(CNetworkInterface* interface)
{
   // Clear the list first
   {
      CGUIMessage msgReset(GUI_MSG_LABEL_RESET, GetID(), CONTROL_WIRELESS);
      OnMessage(msgReset);
   }
   
   m_networkItems.clear(); 
   m_aps.clear();
      
   CGUIDialogProgress* pDlgProgress = (CGUIDialogProgress*)m_gWindowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
   pDlgProgress->SetHeading("");
   pDlgProgress->SetLine(0, "Searching for wireless networks...");
   pDlgProgress->SetLine(1, "");
   pDlgProgress->SetLine(2, "");
   pDlgProgress->StartModal();
   pDlgProgress->Progress();

   // Get the list of access points. Try this 3 times if you get an empty list
   int retryCount = 0;
   while (m_aps.size() == 0 && retryCount < 3)
   {
     m_aps = interface->GetAccessPoints();
     retryCount++;
   }

   pDlgProgress->Close();
   
   CStdString currentEssId = interface->GetCurrentWirelessEssId();
   int iSelectedItem = -1;
   
   for (unsigned int i = 0; i < m_aps.size(); i++)
   {
      CFileItemPtr item ( new CFileItem(m_aps[i].getEssId()) );
      
      int q = m_aps[i].getQuality();
      if (q <= 25) item->SetIconImage("wizard_wireless_icon_1.png");
      else if (q <= 50) item->SetIconImage("wizard_wireless_icon_2.png");
      else if (q <= 75) item->SetIconImage("wizard_wireless_icon_3.png");
      else if (q <= 100) item->SetIconImage("wizard_wireless_icon_4.png");

      if (m_aps[i].getEncryptionMode() != ENC_NONE)      
         item->SetThumbnailImage("wizard_lock_icon.png");

      CGUIMessage msg(GUI_MSG_LABEL_ADD, GetID(), CONTROL_WIRELESS, 0, 0, item);
      OnMessage(msg);            

      m_networkItems.push_back(item);
      
      if (m_aps[i].getEssId() == currentEssId)
        iSelectedItem = i;              
   }

   // If the current essid was not found in the scanned list, add it as "Other"   
   if (iSelectedItem == -1 && currentEssId != "")
   {
      CFileItemPtr item ( new CFileItem(currentEssId) );
      item->SetIconImage("button_keyboard_off.png");
      CGUIMessage msg(GUI_MSG_LABEL_ADD, GetID(), CONTROL_WIRELESS, 0, 0, item);
      OnMessage(msg);
      m_networkItems.push_back(item);
      iSelectedItem = m_aps.size();
   }
   else
   {
      CFileItemPtr item ( new CFileItem("Other...") );
      item->SetIconImage("button_keyboard_off.png");
      CGUIMessage msg(GUI_MSG_LABEL_ADD, GetID(), CONTROL_WIRELESS, 0, 0, item);
      OnMessage(msg);
      m_networkItems.push_back(item);
      if (iSelectedItem == -1)
         iSelectedItem = 0;
   }
   
   {
      CGUIMessage msg(GUI_MSG_ITEM_SELECT, GetID(), CONTROL_WIRELESS, iSelectedItem);
      OnMessage(msg);
   }            
   
   CONTROL_DISABLE(CONTROL_NEXT);   
}

void CGUIWindowBoxeeWizardNetwork::ShowPasswordIfNeeded()
{
   int iItem;
   NetworkAssignment assignment;
   CStdString ipAddress;
   CStdString networkMask;
   CStdString defaultGateway;
   CStdString essId;
   CStdString key;
   EncMode encryptionMode;   
   
   // Get the interface information
   {
      CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), CONTROL_INTERFACES);
      OnMessage(msg);
      iItem = msg.GetParam1();
   }      
   
   if (!m_interfaces[iItem]->IsWireless())
      return;
      
   m_interfaces[iItem]->GetSettings(assignment, ipAddress, networkMask, defaultGateway, essId, key, encryptionMode);
   if (key != "")
   {
      CGUIButtonControl* passwordButton = (CGUIButtonControl*) GetControl(CONTROL_PASSWORD);
      passwordButton->SetLabel(key);
   }
      
   {
      CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), CONTROL_WIRELESS);
      OnMessage(msg);
      iItem = msg.GetParam1();
   }

   if (iItem == (int) m_aps.size())
   {
      // Last item was selected, show the keyboard to get input
      CStdString essId = m_networkItems[m_networkItems.size()-1]->GetLabel();
      if (essId == "Other...")
         essId = "";
         
      if (CGUIDialogKeyboard::ShowAndGetInput(essId, g_localizeStrings.Get(789), false))
      {
         m_networkItems[m_networkItems.size()-1]->SetLabel(essId);

         SET_CONTROL_VISIBLE(CONTROL_SEP2);
         SET_CONTROL_VISIBLE(CONTROL_PASSWORD_GROUP);
         SET_CONTROL_VISIBLE(CONTROL_ENC_GROUP);
         SET_CONTROL_FOCUS(CONTROL_ENC_SELECTION, 0);
      }
   }
   else
   {
      CGUIButtonControl* encSelectionButton = (CGUIButtonControl*) GetControl(CONTROL_ENC_SELECTION);
      encSelectionButton->SetLabel(ENC_LABELS[m_aps[iItem].getEncryptionMode()]);
      
      SET_CONTROL_VISIBLE(CONTROL_SEP2);
      SET_CONTROL_VISIBLE(CONTROL_ENC_GROUP);
      if (m_aps[iItem].getEncryptionMode() == ENC_NONE)
      {
         SET_CONTROL_HIDDEN(CONTROL_PASSWORD_GROUP);
         CONTROL_ENABLE(CONTROL_NEXT);               
         SET_CONTROL_FOCUS(CONTROL_NEXT, 0);
      }
      else
      {
         SET_CONTROL_VISIBLE(CONTROL_PASSWORD_GROUP);
         SET_CONTROL_FOCUS(CONTROL_PASSWORD, 0);
      }
    }
}

void CGUIWindowBoxeeWizardNetwork::ResetCurrentNetworkState()
{
   // Go over all the interfaces and search for the first one which is configured
   m_foundCurrnetNetwork = false;
   for (unsigned int i = 0; i < m_interfaces.size(); i++)
   {
      m_interfaces[i]->GetSettings(m_assignment, m_ipAddress, m_networkMask, m_defaultGateway, m_essId, m_key, m_encryptionMode);
      
      if (m_assignment != NETWORK_DISABLED)
      {
         m_foundCurrnetNetwork = true;
         m_interfaceName = m_interfaces[i]->GetName();
         break;
      }
   }
}

void CGUIWindowBoxeeWizardNetwork::GetUserConfiguration(CStdString& interfaceName, CStdString& essId, CStdString& key, EncMode& enc)
{
  // Get current settings from the GUI components
   CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), CONTROL_INTERFACES);
   OnMessage(msg);
   int iItem = msg.GetParam1();
   CNetworkInterface* interface = m_interfaces[iItem]; 
   interfaceName = interface->GetName();
   
   if (interface->IsWireless())
   {
      CGUIMessage msg2(GUI_MSG_ITEM_SELECTED, GetID(), CONTROL_WIRELESS);
      OnMessage(msg2);
      int iItem = msg2.GetParam1();
      essId = m_networkItems[iItem]->GetLabel();
            
      CGUIButtonControl* passwordButton = (CGUIButtonControl*) GetControl(CONTROL_PASSWORD);
      key = passwordButton->GetLabel();
       
      CGUIButtonControl* encSelectionButton = (CGUIButtonControl*) GetControl(CONTROL_ENC_SELECTION);
      CStdString encStr = encSelectionButton->GetLabel();
      for (int i = 0; i < 5; i++)
      {
         if (strcasecmp(encStr.c_str(), ENC_LABELS[i]) == 0)
         {
            enc = ENC_MODES[i];
            break;
         }
      }
   }
}

bool CGUIWindowBoxeeWizardNetwork::NetworkConfigurationChanged()
{
   if (!m_foundCurrnetNetwork)
      return true;
      
   CStdString currentEssId;
   CStdString currentKey;
   EncMode    currentEnc;
   CStdString currentInterfaceName;
   
   GetUserConfiguration(currentInterfaceName, currentEssId, currentKey, currentEnc);

   CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), CONTROL_INTERFACES);
   OnMessage(msg);
   int iItem = msg.GetParam1();
   CNetworkInterface* interface = m_interfaces[iItem];
      
   // Do the actual comparison
   if (m_interfaceName != currentInterfaceName)
      return true;
            
   bool result = false;   
   if (interface->IsWireless())
   {
      result = (currentEssId != m_essId || currentEnc != m_encryptionMode);
      if (currentEnc != ENC_NONE)
         result = result || (currentKey != m_key);
   }
   
   return result;
}

bool CGUIWindowBoxeeWizardNetwork::SaveConfiguration()
{
   if (!NetworkConfigurationChanged())
      return true;
      
   bool result = false;
   CStdString currentEssId;
   CStdString currentKey;
   EncMode    currentEnc;
   CStdString currentInterfaceName;
   
   GetUserConfiguration(currentInterfaceName, currentEssId, currentKey, currentEnc);
   
   CGUIDialogProgress* pDlgProgress = (CGUIDialogProgress*)m_gWindowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
   pDlgProgress->SetHeading("");
   pDlgProgress->SetLine(0, "Applying network configuration...");
   pDlgProgress->SetLine(1, "");
   pDlgProgress->SetLine(2, "");
   pDlgProgress->StartModal();
   pDlgProgress->Progress();
        
   CStdString empty;
   NetworkAssignment assignment;
   CNetworkInterface* interface; 
   for (unsigned int i = 0; i < m_interfaces.size(); i++)
   {
      interface = m_interfaces[i];
      if (interface->GetName() == currentInterfaceName)
      {
         assignment = NETWORK_DHCP;
         interface->SetSettings(assignment, empty, empty, empty, currentEssId, currentKey, currentEnc);
      }
      else
      { 
         // if we have a different interfaces, we need to take them down
         assignment = NETWORK_DISABLED;
         EncMode enc = ENC_NONE;
         interface->SetSettings(assignment, empty, empty, empty, empty, empty, enc);
      }
   }
   
   pDlgProgress->Close();
     
   if (!interface->IsConnected())
      CGUIDialogOK::ShowAndGetInput(0, 50001, 50002, 0);
   else if (!g_application.IsConnectedToNet())
      CGUIDialogOK::ShowAndGetInput(0, 50003, 50004, 50002);
   else
      result = true;
          
   ResetCurrentNetworkState();
   
   return result;
}

bool CGUIWindowBoxeeWizardNetwork::IsHexString(CStdString& str)
{
  if (str.size() % 2 != 0)
    return false;
    
  for (size_t i = 0; i < str.size(); i++)
  {
    char c = str[i];
    if ((c < '0' || c > '9') && (c < 'a' || c > 'f') && (c < 'A' || c > 'F'))
      return false; 
  }
  
  return true;
}
