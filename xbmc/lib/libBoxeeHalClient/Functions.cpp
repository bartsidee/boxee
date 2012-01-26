#include "Common.h"

#include <string.h>
#include <stdio.h>

void StringMap::AddIntParameter( const char* name, int value)
{
  char szValue[200];
  sprintf(szValue, "%d", value );
  (*this)[ name ] = szValue;
}

void StringMap::AddBoolParameter( const char* name, bool value )
{
  (*this)[ name ] = ( value ? "true" : "false" );
}
