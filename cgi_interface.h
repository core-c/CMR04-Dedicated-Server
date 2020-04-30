/*--------------------------------------------------------
  CGI Variable Wrapper    http://www.purplepixie.org/cgi
  * Read the Licence *    dcutting [at] purplepixie.org

  cgi_interface.h         30/03/2007
  2.07
--------------------------------------------------------*/



#ifndef CGI_INTERFACE
#define CGI_INTERFACE

#define LENMAX_VAR_NAME 254
#define LENMAX_VAR_VALUE 1024

#define LENMAX_VAR_NULLSTRING 2

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cookie.h"

class CGI_Variable
 {
 public:
 CGI_Variable(const char *iName="", const char *iValue="");
 char Name[LENMAX_VAR_NAME];
 char Value[LENMAX_VAR_VALUE];
 CGI_Variable *Next;
 char Encoded[ (LENMAX_VAR_NAME + LENMAX_VAR_VALUE) *2 ];
 char *Encode(void);
 };

class CGI_Interface
 {
 public:
 bool PrintError;
 bool ReturnValidPointer;
 char NullString[LENMAX_VAR_NULLSTRING];
 CGI_Interface(bool pError=false);
 int AutoLoad(void);
 int CookieLoad(void);
 void PrintDump(void);
 int StringLoad(const char *input);
 char* Fetch(const char *name, bool UseReturnValidPointer);
 int Set(const char *name, const char *value);
 int HiddenDump(void);
 int GetDump(void);
 ~CGI_Interface();
 private:
 bool Add(const char *name, const char *value);
 CGI_Variable *First;
 };

#endif

