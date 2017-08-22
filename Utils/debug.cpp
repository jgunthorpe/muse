// -*- mode: cpp; mode: fold -*-
// Description								/*{{{*/
/* ######################################################################

   Debug - A set of functions to allow low level code to output debug
           information.
   
   This source is placed in the Public Domain, do with it what you will
   It was originally written by Jason Gunthorpe <jgg@gpu.srv.ualberta.ca>   

   ##################################################################### */
									/*}}}*/
// Includes								/*{{{*/
#include <debug.h>
#include <thread.h>
#include <stdio.h>
#include <stdarg.h>
#include <errno.h>
#include <string.h>
   									/*}}}*/

// Debugging output FD.
FILE *DebugFD = 0;

// DebugTrace - Write a trace line					/*{{{*/
// ---------------------------------------------------------------------
/* */
void DebugTrace(const char *File,int Line,const char *Func,int /*Level*/,
		const char *Msg)
{
   if (DebugFD == 0)
      return;
   fprintf(DebugFD,"^%s:%d %s %s->\n",File,Line,Func,Msg);
   fflush(DebugFD);
   fflush(DebugFD);
}
									/*}}}*/
// DebugWarn - Write a warning line					/*{{{*/
// ---------------------------------------------------------------------
/* */
void DebugWarn(const char *File,int Line,const char *Func,const char *Msg)
{
   if (DebugFD == 0)
      return;
   fprintf(DebugFD,"$%s:%d %s %s\n",File,Line,Func,Msg);
   fflush(DebugFD);
}
									/*}}}*/
// DebugError - Write a serious error					/*{{{*/
// ---------------------------------------------------------------------
/* */
void DebugError(const char *File,int Line,const char *Func,const char *Msg)
{
   if (DebugFD == 0)
      return;
   fprintf(DebugFD,"!%s:%d %s %s\n",File,Line,Func,Msg);
   fflush(DebugFD);
}
									/*}}}*/
// DebugErrNo - Write a library error with errno			/*{{{*/
// ---------------------------------------------------------------------
/* */
void DebugErrNo(const char *File,int Line,const char *Func,const char *Msg)
{
   if (DebugFD == 0)
      return;
   fprintf(DebugFD,"!%s:%d %s %s: %s (%u)",File,Line,Func,Msg,strerror(errno),errno);
   fflush(DebugFD);
}
									/*}}}*/

// DebugTraceC - Write a continued trace line				/*{{{*/
// ---------------------------------------------------------------------
/* */
void DebugTraceC(const char *File,int Line,const char *Func,int /*Level*/)
{
   if (DebugFD == 0)
      return;
   fprintf(DebugFD,"^%s:%d %s->",File,Line,Func);
   fflush(DebugFD);
}
									/*}}}*/
// DebugWarn - Write a continued warning line				/*{{{*/
// ---------------------------------------------------------------------
/* */
void DebugWarnC(const char *File,int Line,const char *Func)
{
   if (DebugFD == 0)
      return;
   fprintf(DebugFD,"$%s:%d %s->",File,Line,Func);
   fflush(DebugFD);
}
									/*}}}*/
// DebugError - Write a continued serious error				/*{{{*/
// ---------------------------------------------------------------------
/* */
void DebugErrorC(const char *File,int Line,const char *Func)
{
   if (DebugFD == 0)
      return;
   fprintf(DebugFD,"!%s:%d %s->",File,Line,Func);
   fflush(DebugFD);
}
									/*}}}*/

// DebugMessage - Writes the continued message.				/*{{{*/
// ---------------------------------------------------------------------
/* */
void DebugMessage(const char *Format,...)
{
   if (DebugFD == 0)
      return;
   va_list args;
   va_start(args,Format);
   vfprintf(DebugFD,Format,args);
   fprintf(DebugFD,"\n");
   va_end(args);
   fflush(DebugFD);
}
									/*}}}*/
