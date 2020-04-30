#ifndef URLCODE_HEADER
#define URLCODE_HEADER

#include <string.h>

bool char_encode(char src, char *dest)
{
char legal[128]="abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXY0123456789\0";
for (int a=0; a<strlen(legal); a++)
 if (legal[a]==src)
  {
  sprintf(dest,"%c\0",src);
  return false;
  }
}

#endif
