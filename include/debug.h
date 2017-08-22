// -*- mode: cpp; mode: fold -*-
// Description								/*{{{*/
/* ######################################################################

   Debug - A set of functions to allow low level code to output debug
           information.
   
   In many cases a failure in one of the lower level subsystems is not
   fatal but should be recored. Similarly various information is usefull
   to record should a problem arise. These functions allow that. They are
   prefixed with #defines to snag some information about the context.

   Since macros cannot accept a variable number of parameters a two
   line system is often used. The syntax is something like:
   
   WarnC();
   DebugMessage("%s","Hello");
   
   Until DebugMessage is called the entire debug system is busy for all
   other threads so be nice.
   
   GCC is capable of including the function name as it compiles, this
   feature is used. I do not use the GCC extension of variable macro
   parameters because that is completely nonportable.
   
   This source is placed in the Public Domain, do with it what you will
   It was originally written by Jason Gunthorpe <jgg@gpu.srv.ualberta.ca>   
   
   ##################################################################### */
									/*}}}*/
#ifndef DEBUG_H
#define DEBUG_H

#ifdef __GNUC__
# define Trace(Level,Msg) DebugTrace(__FILE__,__LINE__,\
        __FUNCTION__,Level,Msg);
# define Warn(Msg) DebugWarn(__FILE__,__LINE__,__FUNCTION__,Msg);
# define Error(Msg) DebugError(__FILE__,__LINE__,__FUNCTION__,Msg);
# define ErrNo(Msg) DebugErrNo(__FILE__,__LINE__,__FUNCTION__,Msg);

// These macros expect that DebugMessage() will be called directly after ONCE.
# define TraceC(Level) DebugTraceC(__FILE__,__LINE__,\
        __FUNCTION__,Level);
# define WarnC() DebugWarnC(__FILE__,__LINE__,__FUNCTION__);
# define ErrorC() DebugErrorC(__FILE__,__LINE__,__FUNCTION__);

#else

# define Trace(Level,Msg) DebugTrace(__FILE__,__LINE__,0,Level,Msg);
# define Warn(Msg) DebugWarn(__FILE__,__LINE__,0,Msg);
# define Error(Msg) DebugError(__FILE__,__LINE__,0,Msg);
# define ErrNo(Msg) DebugErrNo(__FILE__,__LINE__,0,Msg);

// These macros expect that DebugMessage() will be called directly after ONCE.
# define TraceC(Level) DebugTraceC(__FILE__,__LINE__,0,Level);
# define WarnC() DebugWarnC(__FILE__,__LINE__,0);
# define ErrorC() DebugErrorC(__FILE__,__LINE__,0);
#endif

void DebugTrace(const char *File,int Line,const char *Func,int Level,
		const char *Msg);
void DebugWarn(const char *File,int Line,const char *Func,const char *Msg);
void DebugError(const char *File,int Line,const char *Func,const char *Msg);
void DebugErrNo(const char *File,int Line,const char *Func,const char *Msg);

void DebugTraceC(const char *File,int Line,const char *Func,int Level);
void DebugWarnC(const char *File,int Line,const char *Func);
void DebugErrorC(const char *File,int Line,const char *Func);

void DebugMessage(const char *Format,...);

#define TRACE_INFO 10
#define TRACE_TRACE 1

#include <stdio.h>
extern FILE *DebugFD;
#endif
