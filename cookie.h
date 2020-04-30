/*--------------------------------------------------------
  CGI Variable Wrapper    http://www.purplepixie.org/cgi
  * Read the Licence *    dcutting [at] purplepixie.org

  cookie.h                02/01/2005

  Basic cookie support integration for CGI Wrapper
--------------------------------------------------------*/

#ifndef CGI_COOKIE
#define CGI_COOKIE

// Functions and Stuff

bool SetCookie(const char *name, const char *value);


// Function Definitions and All The Funk

bool SetCookie(const char *name, const char *value)
{
printf("Set-Cookie: %s=%s\n",name,value);
return true;
}



#endif

