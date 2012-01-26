#ifndef GUIBOXEEVIEWSTATE_H_
#define GUIBOXEEVIEWSTATE_H_

#include "SortFileItem.h"
#include "Key.h"

class CGUIBoxeeViewState
{
public:
  CGUIBoxeeViewState(const CStdString& path, int windowId = WINDOW_BOXEE_BROWSE);
  virtual ~CGUIBoxeeViewState();

  const CStdString& GetPath();
  
  bool InitializeBoxeeViewState(const CFileItemList& items, int windowID);
  void SaveBoxeeViewState();

  /////////////////////////
  // View Type Functions //
  /////////////////////////
  
  void AddViewTypes(std::vector<CBoxeeView>& viewTypesVec,const CStdString& chosenViewTypeId);
  void GetViewTypes(std::vector<CBoxeeView>& viewVec);
  bool GetBoxeeView(CBoxeeView& boxeeView) const;

  bool SetViewType(int viewTypeByType);
  int GetViewType() const;
  
  static int GetViewTypeAsEnum(const CStdString& viewType);
  static const CStdString GetViewTypeAsString(int viewType);
  
  ////////////////////////////
  // Sort Methods Functions //
  ////////////////////////////

  void AddSortMethods(std::vector<CBoxeeSort>& sortMethodsVec,const CStdString& chosenSortMedthodId);
  void GetSortMethods(std::vector<CBoxeeSort>& sortVec);
  bool GetBoxeeSort(CBoxeeSort& boxeeSort) const;
  
  bool SetSortById(const CStdString& sortMethodId);

  SORT_METHOD GetSortMethod() const;
  
  SORT_ORDER GetSortOrder() const;
  
  CStdString GetSortName() const;

  static SORT_METHOD GetSortMethodAsEnum(const CStdString& sortBy);
  static const CStdString GetSortMethodAsString(SORT_METHOD sortMethod);
  static SORT_ORDER GetSortOrderAsEnum(const CStdString& sortOrder);
  static const CStdString GetSortOrderAsString(SORT_ORDER sortOrder);
  static int GetSortNameAsInt(const CStdString& sortTypeName);
  static const CStdString GetSortNameAsString(int sortName);

protected:
  
  CStdString m_path;
  int m_windowId;
  
private:
 
  bool ShouldGetParentPath(const CStdString& ParentPath);

  /////////////////////////
  // View Type Functions //
  /////////////////////////
  
  bool SetBoxeeViewAsChosen(CBoxeeView& boxeeView);
  
  ////////////////////////////
  // Sort Methods Functions //
  ////////////////////////////

  bool SetBoxeeSortAsChosen(CBoxeeSort& boxeeSort);

  ////////////////////////////
  // View Type data members //
  ////////////////////////////

  std::vector<CBoxeeView> m_viewTypesVec;
  CStdString m_chosenViewTypeKey;
  
  ///////////////////////////////
  // Sort Methods data members //
  ///////////////////////////////

  std::vector<CBoxeeSort> m_sortMethodsVec;
  CStdString m_chosenSortMethodKey;
};

#endif /*GUIBOXEEVIEWSTATE_H_*/

