#include "PlatformDefs.h"
#include "InfoPage.h"
#include "Util.h"
#include "bxexceptions.h"
#include "BoxeeUtils.h"


//InfoPage InfoPage::infoPage;

// To use:
//  static TimeMeasure measure;
//  struct timeval tv;
//  struct timezone tz;
//  struct tm *tm;
//  gettimeofday(&tv, &tz);
//  tm=localtime(&tv.tv_sec);
//
//     Code
//
//
// static string s("Font");
// UpdateTimeMeasure(measure ,tv, s);

// From other place periodically call PrintTimeMeasure() and ClearTimeMeasure()
/*

Make sys independant later.

struct TimeMeasure {
  long long int total;
  std::map<CStdString, long long> str_to_usec;
};


void UpdateTimeMeasure(TimeMeasure &measure, struct timeval &func_start, CStdString& id) {
  int usecs = func_start.tv_usec;
  static int last_secs;
  struct timeval tv;
  struct timezone tz;
  struct tm *tm;
  static int calls = 0;
  calls++;

  gettimeofday(&tv, &tz);
  tm=localtime(&tv.tv_sec);
  int usecs2 = tv.tv_usec;
  if (usecs2 > usecs) {
    usecs2 -= usecs;
    measure.total += usecs2;
    measure.str_to_usec[id]+=usecs2;
  } else {
    printf("Boom %d %d\n", last_secs, tv.tv_sec);
  }
}


void PrintTimeMeasure(TimeMeasure &measure) {
  std::map<CStdString, long long int>::const_iterator nitr;
  printf("Millis in strings:\n");
  for(nitr = measure.str_to_usec.begin(); nitr != measure.str_to_usec.end(); ++nitr) {
    printf("%6"PRId64"\t: %s:\n", (*nitr).second/1000, (*nitr).first.c_str());
  }
  printf("Total mili-seconds:%"PRId64"\n", measure.total/1000);
}

void ClearTimeMeasure(TimeMeasure &measure) {
  measure.str_to_usec.clear();
  measure.total = 0;
}
*/


InfoPage::InfoPage()
{
}

InfoPage::~InfoPage()
{
}


// Print short description about every page.
void InfoPage::PrintMenu() {
  std::map<int, std::pair<InfoPageble*, CStdString> >::iterator iter;   
  printf("===================\n");
  printf("%3d : %s\n", INFOPAGE_EXIT, "Kill application");
  for( iter = pages.begin(); iter != pages.end(); iter++ ) {
    printf("%3d : %s\n", iter->first, iter->second.second.c_str());
  }
  printf("===================\n");  
}


void InfoPage::Process()
{
  while (!m_bStop)
  {
    PrintMenu();
    int i;
    static char buff[200] = "";
    scanf("%d %s", &i, buff); 

    if (i == 0) {
      exit(0);
    }
    
    std::map<int, std::pair<InfoPageble*, CStdString> >::iterator iter;
    iter = pages.find(i);
    if (iter != pages.end()) {
      CStdString page;
      CStdString params(buff);
      page.reserve(10000);
      iter->second.first->GetInfoPage(&page, params);
      printf("%s\n", page.c_str());      
    } else {
      printf("Page not found\n");      
    }
  }
}


void InfoPage::Register(int index, CStdString descr, InfoPageble* p) {
  std::pair<InfoPageble*, CStdString> pa(p,descr);
  pages[index]= pa;
}


void InfoPage::UnRegister(int index) {
  pages.erase(index);
}



