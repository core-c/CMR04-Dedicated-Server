/*--------------------------------------------------------
  CGI Variable Wrapper    http://www.purplepixie.org/cgi
  * Read the Licence *    dcutting [at] purplepixie.org

  cgi_interface.cpp       30/03/2007
  2.07
--------------------------------------------------------*/


#ifndef CGI_MAIN
#define CGI_MAIN

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cgi_interface.h"

CGI_Variable::CGI_Variable(const char *iName, const char *iValue)
 {
 strcpy(Name,iName);
 strcpy(Value,iValue);
 Next=0;
 }

char *CGI_Variable::Encode(void)
 {
 int EncPtr=0;
 int InpPtr=0;
 int InpMax=strlen(Value);
 for (InpPtr=0; InpPtr<InpMax; InpPtr++)
  {
  switch (Value[InpPtr])
   {
   case ' ':
	Encoded[EncPtr++]='+';
	break;
   case '\"':
        Encoded[EncPtr++]='%';
        Encoded[EncPtr++]='2';
        Encoded[EncPtr++]='2';
	break;
   case '%':
        Encoded[EncPtr++]='%';
        Encoded[EncPtr++]='2';
        Encoded[EncPtr++]='5';
	break;
   case '&':
        Encoded[EncPtr++]='%';
        Encoded[EncPtr++]='2';
        Encoded[EncPtr++]='6';
	break;
   case '?':
        Encoded[EncPtr++]='%';
        Encoded[EncPtr++]='3';
        Encoded[EncPtr++]='f';
	break;
   case '+':
	Encoded[EncPtr++]='%';
	Encoded[EncPtr++]='2';
	Encoded[EncPtr++]='b';
	break;
   default:
	Encoded[EncPtr++]=Value[InpPtr];
   }
  }
 

 return Encoded;
 }

CGI_Interface::CGI_Interface(bool pError)
 {
 PrintError=pError;
 First=0;
 ReturnValidPointer=false;
 for (int a=0; a< LENMAX_VAR_NULLSTRING ; a++)
  {
  NullString[a]=0;
  }
 }

int CGI_Interface::AutoLoad(void)
 {
 char *method=getenv("REQUEST_METHOD");
 if ( (method==0) || (strlen(method)<3) || (strcmp(method,"(null)")==0) )
  {
  if (PrintError) printf("CGI Error: No method specified: %s\n<br>\n",method);
  return -1;
  }
 if (strcmp("GET",method)==0)
  return StringLoad(getenv("QUERY_STRING"));
 else if (strcmp("POST",method)==0)
  {
  int count=0;
  char buff[1024*32];
  fgets(buff,1024*32,stdin);
  while (!feof(stdin))
   {
   count+=StringLoad(buff);
   fgets(buff,1024*32,stdin);
   }
  if (strlen(buff)>0) count+=StringLoad(buff);
  return count;
  }
 else
  {
  if (PrintError) printf("CGI Error: Unsupported Method %s\n <br>\n",method);
  return -1;
  }
 }

int CGI_Interface::CookieLoad(void)
{ // New in 2005 versions (2.06+) - might still not work properly :o(

char ts[256];
for (int a=0; a<256; a++)
 ts[a]=0;

char *cookies=getenv("HTTP_COOKIE");

int loaded=0;

int tptr=0;
for (int cpos=0; cpos<strlen(cookies); cpos++)
 {
 char n=cookies[cpos];
 if ((n!=';')&&(n!=' '))
  ts[tptr++]=n;
 else if (n!=' ')
  {
  loaded+=StringLoad(ts);
  tptr=0;
  for (int z=0; z<256; z++) ts[z]=0;
  }
 }

if (strlen(ts)>0) StringLoad(ts);

return loaded;
}

bool CGI_Interface::Add(const char *name, const char *value)
 {
 CGI_Variable *nv=new CGI_Variable(name,value);
 if (First==0)
  {
  First=nv;
  return true;
  }
 CGI_Variable *c=First;
 while (c->Next!=0) c=c->Next;
 c->Next=nv;
 return true;
 }

CGI_Interface::~CGI_Interface()
 {
 if (First==0) return;
 CGI_Variable *c=First;
 while (c->Next!=0)
  {
  CGI_Variable *d=c;
  c=c->Next;
  delete d;
  }
 delete c;
 }

int CGI_Interface::StringLoad(const char *input)
 {
 if (input==0) return -1;
 if (strlen(input)<=0) return -1;
 int count=0;
 char cname[LENMAX_VAR_NAME];
 char cvalue[LENMAX_VAR_VALUE];
 int pos=0;
 bool name=true;
 long val=0;
 char d[5];
 for (int a=0; a<strlen(input); a++)
  {
  if (name) { cname[pos]=0; cname[pos+1]=0; }
  else { cvalue[pos]=0; cvalue[pos+1]=0; }
  switch (input[a])
   {
   case '=':
    if (name)
     {
     cname[pos]=0;
     pos=0;
     name=false;
     }
    break;
   case '&':
    if (!name)
     cvalue[pos]=0;
    else
     { cvalue[0]=0; cname[pos]=0; }
    Set(cname,cvalue);
    cname[0]=0;
    cvalue[0]=0;
    count++;
    name=true;
    pos=0;
    break;
   case '\%':
    strncpy(d,input+a+1,2);
    val=strtol(d,(char**)NULL,16);
    if (name) cname[pos]=val;
    else cvalue[pos]=val;
    pos++;
    a=a+2;
    break;
   case '+':
    if (name) cname[pos]=' ';
    else cvalue[pos]=' ';
    pos++;
    break;
   default:
    if (name) cname[pos]=input[a];
    else cvalue[pos]=input[a];
    pos++;
   }
  }
 if (strlen(cname)>0) { Set(cname,cvalue); count++; }
 return count;
 }
     
void CGI_Interface::PrintDump(void)
{
printf("CGI Interface Variable Dump: <p>\n\n");
CGI_Variable *c=First;
int count=0;
while (c!=0)
 {
 printf("%d: %s: %s <br>\n",count+1,c->Name,c->Value);
 c=c->Next;
 count++;
 }
printf("\n<p>\nTotal of %d Variables Found\n<p>\n",count);
}

char* CGI_Interface::Fetch(const char *name, bool UseReturnValidPointer=true)
{
CGI_Variable *c;
c=First;
while (c!=0)
 {
 if ( strcmp(c->Name,name)==0) return c->Value;
 c=c->Next;
 }
if (!UseReturnValidPointer) return 0;
if (ReturnValidPointer) return NullString;
return 0;
}

int CGI_Interface::Set(const char *name, const char *value)
{
char *c=Fetch(name,false);
if (c==0)
 Add(name,value);
else
 strcpy(c,value);
return 1;
}

int CGI_Interface::HiddenDump(void)
{
CGI_Variable *c=First;
int count=0;
while (c!=0)
 {
 printf("<input type=hidden name=\"%s\" value=\"",c->Name);
 for (int a=0; a<strlen(c->Value); a++)
  {
  if (c->Value[a]=='\"') printf("%s","%22");
  else printf("%c",c->Value[a]);
  }
 printf("\">\n");
 count++;
 c=c->Next;
 }
return count;
}

int CGI_Interface::GetDump(void)
{
CGI_Variable *c=First;
int count=0;
while (c!=0)
 {
 if (count>0) printf("&");
 printf("%s=%s",c->Name,c->Encode());
 count++;
 c=c->Next;
 }
return count;
}

#endif
