// -*- mode: cpp; mode: fold -*-
// Description								/*{{{*/
/* ######################################################################

   TimeBuf - Buffers patterndata till it is actually played
   
   Because Muse runs in a non realtime environment it is impossible for
   the player to compute data a few milli seconds before it is actually 
   played. So it buffers the raw output, sometimes up to 3s of data are
   stored. This class buffers the muse pattern data and syncronises it
   with the current actual output location. This allows realtime 
   displays and other niceties that would otherwise be impractical in a
   non realtime multitasking operating system.

   ##################################################################### */
									/*}}}*/
#ifndef TIMEBUF_H
#define TIMEBUF_H

#ifndef EFXFILTR_H
#include <efxfiltr.h>
#endif

struct museTimeBase
{
   museTimeBase *Next;
   museFrameInfo *Frames;
   museChannel *Channels;
   unsigned long ChanDepth;
   unsigned long Size;
   unsigned long RealSize;
};

class museTimeBuffer : public museOutputLink
{
   museTimeBase *Head;
   museTimeBase *Tail;
   museTimeBase *Spare;
   unsigned long CurPush;
   unsigned long CurPop;

   museTimeBase *GetBuffer();
   void FreeBuffer(museTimeBase *Buffer);

   museFrameInfo Frame;
   unsigned long CurrentTime;
   unsigned long CurrentChanDepth;
   public:

   void GetNextFrame(museFrameInfo *Frame,SequenceChannel *Chan);
   virtual long InitPlay(char **Error);
   virtual void SetFrameInfo(museFrameInfo *Frame);
   virtual long Compute(SequenceChannel *Channels, unsigned long Len);

   museTimeBuffer();
   ~museTimeBuffer();
};

#endif
