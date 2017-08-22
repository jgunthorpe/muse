// -*- mode: cpp; mode: fold -*-
// Description								/*{{{*/
/* ######################################################################

   String Util - Some usefull string functions.

   See the ansi refernces for the normal functions, these are all case
   insensitive versions of those.
   
   This source is placed in the Public Domain, do with it what you will
   It was originally written by Jason Gunthorpe <jgg@gpu.srv.ualberta.ca>   
   
   ##################################################################### */
									/*}}}*/
// Includes								/*{{{*/
#include <strutl.h>
#include <ctype.h>
   									/*}}}*/

// strstrip - Remove white space from the front and back of a string	/*{{{*/
// ---------------------------------------------------------------------
/* This is handy to use when parsing a file */
char *_strstrip(char *String)
{
   for (;*String != 0 && (*String == ' ' || *String == '\t'); String++);

   char *End = String + strlen(String);
   for (;End != String && (*End == ' ' || *End == '\t'); End--);
   *End = 0;
   return String;
};
									/*}}}*/
// stristr - Case independant versino of strstr				/*{{{*/
// ---------------------------------------------------------------------
/* */
char *_stristr(const char *str1, const char *str2)
{
   int len1 = strlen(str1);
   int len2 = strlen(str2);
   int i;
   int j;
   int k;

   // Returns str1 if str2 empty
   if (!len2)
      return (char *)str1;    

   // Return 0 if str1 empty
   if (!len1)
      return 0;

   i = 0;
   for(;;)
   {
      while(i < len1 && toupper(str1[i]) != toupper(str2[0]))
         i++;
      
      if (i == len1)
         return 0;
      j = 0;
      k = i;
      while (i < len1 && j < len2 && toupper(str1[i]) == toupper(str2[j]))
      {
         i++;
         j++;
      }
      if (j == len2)
         return (char *)str1+k;
      if (i == len1)
         return 0;
   }
}
									/*}}}*/

#ifdef __GNUC__
int _stricmp(const char *S1,const char *S2)
{
   while (*S1 != 0 && *S2 != 0)
   {
      if (toupper(*S1) != toupper(*S2))
	 return -1;
      S1++;
      S2++;
   }
   
   if (*S1 == 0 && *S2 == 0)
      return 0;
   else 
      return 1;
}

int _strnicmp(const char *S1,const char *S2,unsigned int Count)
{
   while (*S1 != 0 && *S2 != 0 && Count != 0)
   {
      if (toupper(*S1) != toupper(*S2))
	 return -1;
      S1++;
      S2++;
      Count--;
   }

   if (*S1 == 0 && *S2 == 0)
      return 0;
   else 
      return 1;
}
#endif
