#ifndef BROWSEWINDOWCONFIGURATION_H_
#define BROWSEWINDOWCONFIGURATION_H_

#include "StdString.h"
#include "BrowseWindowFilter.h"
#include <map>
#include <vector>
#include "GUIBoxeeViewState.h"

#define BROWSE_MODE_VIDEO "video"
#define BROWSE_MODE_MUSIC "music"
#define BROWSE_MODE_PICTURES "pictures"
#define BROWSE_MODE_OTHER "other"
#define BROWSE_MODE_FOLDER "folder"


// The purpose of this class is to provide configuration
// parameters for browse screen
class CBrowseWindowConfiguration
{
public:

	CBrowseWindowConfiguration();
	virtual ~CBrowseWindowConfiguration();
	
	// Master list of all available filters
	static std::map<int, CBrowseWindowFilter*> m_filters;

	// This function is called from the contructor and it creates a master list of filters
	static void CreateFilters();
	
	bool HasPath() { return !m_strPath.IsEmpty(); }

	// Filters
	CBrowseWindowFilter* GetFilter(int iFilterId) const;
	void AddActiveFilter(int iFilterId);
	void AddCustomFilter(CBrowseWindowFilter* pCustomFilter);
	bool ApplyFilter(const CFileItem* pItem) const;
	bool ApplySpecificFilter(const CFileItem* pItem, CBrowseWindowFilter* pFilter) const;
  bool HasFilters();
	void ClearActiveFilters();
  void RemoveCustomFilter(int filterId);
	
	// Vector of currently activated filters
  std::vector<int> m_activeFilters;
  std::vector<CBrowseWindowFilter*> m_customFilters;

	// Vector of filters that are available in the current configuration
	std::vector<CBrowseWindowFilterContainer> m_currentFilters;

	void ClearViewTypes();
	void ClearSortMethods();

	CStdString m_strType;
	CStdString m_strTitle;
	CStdString m_strPath;
	CStdString m_strLabel;
	CStdString m_strTitleIcon;
	bool m_loaded;

	int m_iSelectedItem;
  CStdString m_strBackgroundImage;

	// Factory method to create configuration by type
	static CBrowseWindowConfiguration GetConfigurationByType(const CStdString& strType);
	static CBrowseWindowConfiguration GetConfigurationByPath(const CStdString& strPath);
	
	// Updates the list of current filters according the provided path and configuration type
	static void UpdateFiltersByPath(const CStdString& strPath, CBrowseWindowConfiguration& configuration);

	// Initializes filters according to type 
	static void InitializeFilters(const CStdString& strType, CBrowseWindowConfiguration& configuration);

	// Auxiliary methods for configuration initializations
	static void InitializeVideoFilters(CBrowseWindowConfiguration& configuration);

	std::vector<CBoxeeView> m_viewTypes;
	std::vector<CBoxeeSort> m_sortMethods;

private:

	 CBrowseWindowAllowFilter* m_browseWindowAllowFilter;
};

#endif /* BROWSEWINDOWCONFIGURATION_H_ */
