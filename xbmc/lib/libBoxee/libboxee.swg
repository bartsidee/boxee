%module boxee
%include "std_pair.i"
%include "std_string.i"
%include "std_vector.i"
%include "std_map.i"

%apply const std::string& {std::string* foo};

%{
#include "bxstringmap.h"
#include "bxobject.h"
#include "bxconstants.h"
#include "boxee.h"
#include "bxcredentials.h"
#include "bxcurl.h"
#include "bxxmldocument.h"
#include "bxfriend.h"
#include "bxfriendslist.h"
#include "bxmessages.h"
#include "bxrssreader.h"
#include "bxboxeefeed.h"
#include "logger.h"

using namespace BOXEE;

%}

%include "bxstringmap.h"
%include "bxobject.h"
%include "bxconstants.h"
%include "boxee.h"
%include "bxcredentials.h"
%include "bxcurl.h"
%include "bxxmldocument.h"
%include "bxfriend.h"
%include "bxfriendslist.h"
%include "bxmessages.h"
%include "bxrssreader.h"
%include "bxboxeefeed.h"
%include "logger.h"

using namespace BOXEE;
%template(VecMessages) std::vector<BOXEE::BXGeneralMessage>;
%template(MessageFilter) std::map<std::string, std::string>;

