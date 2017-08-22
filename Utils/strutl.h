// -*- mode: cpp; mode: fold -*-
// Description								/*{{{*/
/* ######################################################################

   String Util - These are some usefull string functions
   
   We use extra-ansi syntax so as to conform with local compiles whenever
   possible. str?dupcpp is provided to duplicate a string using new 
   instead of malloc, that makes deallocation consistantly use delete [].
   
   _str?icmp is an Extra Ansi function that is commonly seen in DOS 
   compilers.

   _strstrip is a function to remove whitespace from the front and end
   of a string.
   
   This source is placed in the Public Domain, do with it what you will
   It was originally written by Jason Gunthorpe <jgg@gpu.srv.ualberta.ca>   
   
   ##################################################################### */
									/*}}}*/
#ifndef STRUTL_H
#define STRUTL_H

#include <string.h>

// Just like strdup but uses new.
inline char *_strdupcpp(const char *S)
{
   return strcpy(new char[strlen(S)+1],S);
}									   

// Copies a specific portion of the string, it does null terminate.
inline char *_strndupcpp(const char *S,unsigned int Len)
{
   char *T = strncpy(new char[Len+1],S,Len);
   T[Len] = 0;
   return T;
}

char *_stristr(const char *str1, const char *str2);
char *_strstrip(char *String);

// This is a stricmp function, gcc lacks one (strange but true)
#ifdef __GNUC__
int _stricmp(const char *S1,const char *S2);
int _strnicmp(const char *S1,const char *S2,unsigned int Count);
#endif
									   
#endif
