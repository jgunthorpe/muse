// -*- mode: cpp; mode: fold -*-
// Description								/*{{{*/
/* ######################################################################

   Line File - This class performs high speed line reading.
   
   This class is a very simple per line parser. It uses a sliding window
   on the file to get it's speed.
   
   Simply put it allocates N bytes, fills it with data from the file, 
   seeks to the \n, puts a 0 in and then calls that a line. When a line 
   runs of the end of the buffer the remaining bit is memcpy'd to the 
   start of the buffer and the buffer is filled again. This provides a 
   low overhead (0 copy in 90% of the cases) reader. The block size
   must be larger than the max line length or it will get mad :> 
   
   The returned pointer can be fully manipulated, as long as no bytes 
   past the null are changed.
   
   Note! After each call to GetLine or PeekLine the old line is INVALID.
   
   This source is placed in the Public Domain, do with it what you will
   It was originally written by Jason Gunthorpe <jgg@gpu.srv.ualberta.ca>   
   
   ##################################################################### */
									/*}}}*/
#ifndef LINEFILE_H
#define LINEFILE_H

#include <ptypes.h>

class LineFile
{
   int Fd;
   char *Buffer;
   char *BEnd;
   unsigned long Size;
   char *Cur;       // Current scan position
   char *Peek;      // Line returned by peek.
   int Error;       // Error indicator
      
   public:

   // Returns false when the end of the file is reached.
   bool GetLine(char *&Line);
   bool PeekLine(char *&Line);
   
   // Returns false if the file can't be opened
   bool Open(const char *File);
   
   LineFile(unsigned long BSize = 4096);
   ~LineFile();
};

#endif
