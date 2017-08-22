// -*- mode: cpp; mode: fold -*-
// Description								/*{{{*/
/* ######################################################################

   TimeBuffer - Patterndata buffering class
   
   This uses an evil method of storing the pattern data. It uses a linked
   list of buffers. Each buffer contains 100 frames. Compute inserts frames
   at the end of the current buffer and GetFrame pulls out out of the 
   beginning. A simple queue. But when a buffer is exhausted the a new
   buffer is allocated an linked to the last one to provide another 100 
   entries. When GetFrame runs out of the old buffer it frees it and
   goes on. A single spare buffer is kept so that in the case were
   the real time is only a few frames behind the computed no buffer
   allocations are performed.
   
   What is stored in the buffers is NOT delta pattern data, but the current
   data for that frame. All values are as accurate as possible.

   ##################################################################### */
									/*}}}*/
// Include Files							/*{{{*/
#include <muse.h>
#include <timebuf.h>
   									/*}}}*/

// TimeBuffer::museTimeBuffer - Constructor				/*{{{*/
// ---------------------------------------------------------------------
/* This zeros the data members and then allocates a single buffer. The
   class stores data using a circular buffering arrangment. */
museTimeBuffer::museTimeBuffer()
{
   Head = 0;
   Tail = 0;
   Spare = 0;
   CurPush = 0;
   CurPop = 0;
   CurrentTime = 0;
   CurrentChanDepth = 0;
   Tail = Head = GetBuffer();
   memset(&Frame,0,sizeof(Frame));
}
									/*}}}*/
// TimeBuffer::~museTimeBuffer - Destructor 		 		/*{{{*/
// ---------------------------------------------------------------------
/* Frees all of the allocated data blocks. */
museTimeBuffer::~museTimeBuffer()
{
   // Free all the blocks
   museTimeBase *Temp = Head;
   while (Temp != 0)
   {
      Head = Head->Next;
      FreeBuffer(Temp);
      Temp  = Head;
   }
   FreeBuffer(Spare);
}
									/*}}}*/
// TimeBuffer::InitPlay - Reset the timing and free blocks		/*{{{*/
// ---------------------------------------------------------------------
/* Resets the current time and frees most of the data blocks. */
long museTimeBuffer::InitPlay(char **Error)
{
   // Free most of the blocks
   museTimeBase *Temp = Head;
   while (Temp != Tail)
   {
      Head = Head->Next;
      FreeBuffer(Temp);
      Temp  = Head;
   }

   CurPush = 0;
   CurPop = 0;
   CurrentTime = 0;
   memset(&Frame,0,sizeof(Frame));

   return museOutputLink::InitPlay(Error);
}
									/*}}}*/
// TimeBuffer::GetBuffer - Returns a new data block.			/*{{{*/
// ---------------------------------------------------------------------
/* It is useufll to have a single spare buffer block because of the way
   speed changes tend to occure in modules -- and other aspects. This
   will allocate a new data block if the spare does not have one. */
museTimeBase *museTimeBuffer::GetBuffer()
{
   // Allocate a new block
   if (Spare == 0)
   {
      museTimeBase *Temp = new museTimeBase;
      Temp->RealSize = 100;
      Temp->Size = Temp->RealSize;
      Temp->ChanDepth = CurrentChanDepth;
      Temp->Frames = new museFrameInfo[Temp->Size];
      Temp->Next = 0;
      if (Temp->ChanDepth != 0)
      {
         Temp->Channels = new museChannel[Temp->ChanDepth*Temp->Size];

         // Zero channel structure
         memset(Temp->Channels,0,Tail->ChanDepth*sizeof(*Tail->Channels)*Tail->Size);
      }
      else
         Temp->Channels = 0;

      return Temp;
   }

   // Take it out of the spare
   museTimeBase *Temp = Spare;
   Spare = 0;
   Temp->Next = 0;
   Temp->Size = Temp->RealSize;

   // Reallocate the channel buffer.
   if (Temp->ChanDepth != CurrentChanDepth)
   {
      delete [] Temp->Channels;
      Temp->ChanDepth = CurrentChanDepth;
      Temp->Channels = new museChannel[Temp->ChanDepth*Temp->Size];

      // Zero channel structure
      memset(Temp->Channels,0,Tail->ChanDepth*sizeof(*Tail->Channels)*Tail->Size);
   }
   return Temp;
}
									/*}}}*/
// TimeBuffer::FreeBuffer - Deallocates the buffer			/*{{{*/
// ---------------------------------------------------------------------
/* Simply frees the buffer and it's associated frame list and channel list. */
void museTimeBuffer::FreeBuffer(museTimeBase *Buf)
{
   // Store it in the spare if possible
   if (Spare == 0)
      Spare = Buf;
   else
   {
      delete [] Buf->Frames;
      delete [] Buf->Channels;
      delete Buf;
   }
}
									/*}}}*/
// TimeBuffer::SetFrameInfo - Store generic frame information 		/*{{{*/
// ---------------------------------------------------------------------
/* This is called by the fileformat to store information about this
   current frame. It exists in the same thread as compute does. */
void museTimeBuffer::SetFrameInfo(museFrameInfo *Frme)
{
   if (Frme->Frame == 0)
   {
      Frme->Time = 0;
      Frme->PlayingChannels = 0;
      Frame = *Frme;
    }
}
									/*}}}*/
// TimeBuffer::Compute - Buffer the pattern data			/*{{{*/
// ---------------------------------------------------------------------
/* This de-delta encodes it as it buffers it. */
long museTimeBuffer::Compute(SequenceChannel *Channels, unsigned long Len)
{
   CurrentTime += Len;

   // Reallocate if the channel depth has changed
   if (Channels->size() != CurrentChanDepth)
   {
      CurrentChanDepth = Channels->size();

      // First item, just delete/new
      if (CurPush == 0)
      {
         delete [] Tail->Channels;
         Tail->ChanDepth = CurrentChanDepth;
         Tail->Channels = new museChannel[Tail->ChanDepth*Tail->Size];

         // Zero channel structure
         memset(Tail->Channels,0,Tail->ChanDepth*sizeof(*Tail->Channels)*Tail->Size);
      }
      else
      {
         // Cut this buffer short and append a new one.
         Tail->Size = CurPush;
         Tail->Next = GetBuffer();
         Tail = Tail->Next;
         CurPush = 0;
      }
   }

   // Fill in CHAN_Free
   long Rc = museOutputLink::Compute(Channels,Len);

   /* Copy channel data, the data placed in the struct is the CURRENT state,
      not the delta state as is passed in */
   museChannel *Ch = Tail->Channels + CurPush*Tail->ChanDepth;
   unsigned int Playing = 0;
   for (SequenceChannel::iterator I = Channels->begin(); I != Channels->end(); I++, Ch++)
   {
      // Remove free indicator if it is set
      Ch->Flags = Ch->Flags & (~CHAN_Free);
      Ch->Flags |= I->Flags;
      Ch->ModuleChannel = I->ModuleChannel;

      // Playing
      if ((Ch->Flags & CHAN_Free) == 0)
         Playing++;

      // Empty
      if ((I->Flags & (~CHAN_Free)) == 0)
         continue;

      // Copy new inst
      if ((I->Flags & CHAN_Instrument) != 0)
      {
         // Forced retrig
         if (Ch->Sample != I->Sample)
            Ch->Flags |= CHAN_Retrig;
         Ch->Sample = I->Sample;
      }

      // Copy sample offset
      if ((I->Flags & CHAN_Retrig) != 0)
      {
         Ch->SampleOffset = I->SampleOffset;
         // Note started, increase channel counter and remove free state
         if ((Ch->Flags & CHAN_Free) != 0)
         {
            Ch->Flags = Ch->Flags & (~CHAN_Free);
            Playing++;
         }
      }

      // Copy pitch
      if ((I->Flags & CHAN_Pitch) != 0)
         Ch->Pitch = I->Pitch;

      // Change volume
      if ((I->Flags & CHAN_Volume) != 0)
      {
         Ch->LeftVol = I->LeftVol;
         Ch->RightVol = I->RightVol;
         Ch->MainVol = I->MainVol;
         Ch->Pan = I->Pan;
      }

      // Change looping
      if ((I->Flags & CHAN_NewLoop) != 0)
      {
         Ch->NewLoopEnd = I->NewLoopEnd;
         Ch->NewLoopBegin = I->NewLoopBegin;
         Ch->NewLoopFlags = I->NewLoopFlags;
      }
   }

   // Ensures the highest channel count is very accurate
   Frame.PlayingChannels = max(Frame.PlayingChannels,Playing);

   // Add the actual frame data
   if (Frame.Time == 0)
   {
      Frame.Time = CurrentTime - Len;
      if (Frame.SongTime == TimeNull)
         Frame.SongTime = Frame.Time;

      Tail->Frames[CurPush] = Frame;
      museChannel *Ch = Tail->Channels + CurPush*Tail->ChanDepth;

      // Odd to resolve thread issues, atomic changing of CurPush
      if (CurPush + 1 == Tail->Size)
      {
         Tail->Next = GetBuffer();
         memcpy(Tail->Next->Channels,Ch,sizeof(*Ch)*Tail->Next->ChanDepth);
         Ch = Tail->Next->Channels;
         CurPush = 0;
         Tail = Tail->Next;
      }
      else
      {
         memcpy(Ch+Tail->ChanDepth,Ch,sizeof(*Ch)*Tail->ChanDepth);
         Ch += Tail->ChanDepth;
         CurPush++;
      }
      for (museChannel *T = Ch; T != Ch + Tail->ChanDepth;T++)
         T->Flags = 0;
   }
   return Rc;
}
									/*}}}*/
// TimeBuffer::GetNextFrame - Gets the frame that is closest to now	/*{{{*/
// ---------------------------------------------------------------------
/* This returns the frame closest to the current actual playback position.
   All past frames are dequed. */
void museTimeBuffer::GetNextFrame(museFrameInfo *Frame,SequenceChannel *Chan)
{
   // No items in the queue, wait for some to be added
   Frame->Time = 0;
   while (Head == Tail && CurPop >= CurPush)
   {
      if (Sync(CurrentTime) == 0xFFFFFFFF)
         return;
   }

   // Extract the current frame
   signed long CurExtract = CurPop;
   unsigned long Time = Head->Frames[CurExtract].Time;
   CurPop++;
   if (CurPop == Head->Size)
   {
      // Store the last hit incase it's the correct one
      *Frame = Head->Frames[CurExtract];
      Chan->reserve(Head->ChanDepth);
      Chan->_length = Head->ChanDepth;
      memcpy(Chan->begin(),Head->Channels+CurExtract*Head->ChanDepth,Head->ChanDepth*sizeof(*Head->Channels));
      CurExtract = -1;
      museTimeBase *Temp = Head;
      Head = Head->Next;
      FreeBuffer(Temp);
      CurPop = 0;
   }

   // Sync and isolate the correct element
   Time = Sync(Time);
   if ((Time == 0xFFFFFFFF) || (Head == Tail && CurPop >= CurPush))
   {
      if (CurExtract != -1)
      {
         *Frame = Head->Frames[CurExtract];
         Chan->reserve(Head->ChanDepth);
         Chan->_length = Head->ChanDepth;
         memcpy(Chan->begin(),Head->Channels+CurExtract*Head->ChanDepth,Head->ChanDepth*sizeof(*Head->Channels));
      }
      return;
   }

   // Locate the highest element
   while (Head->Frames[CurPop].Time <= Time)
   {
      CurExtract = CurPop;
      CurPop++;
      if (CurPop == Head->Size)
      {
         // Store the last hit incase it's the correct one
         *Frame = Head->Frames[CurExtract];
         Chan->reserve(Head->ChanDepth);
         Chan->_length = Head->ChanDepth;
         memcpy(Chan->begin(),Head->Channels+CurExtract*Head->ChanDepth,Head->ChanDepth*sizeof(*Head->Channels));
         CurExtract = -1;

         museTimeBase *Temp = Head;
         Head = Head->Next;
         FreeBuffer(Temp);
         CurPop = 0;
      }

      // Ran out of stuf
      if (Head == Tail && CurPop >= CurPush)
         break;
   }

   // Recall the item unless it's already been done.
   if (CurExtract != -1)
   {
      *Frame = Head->Frames[CurExtract];
      Chan->reserve(Head->ChanDepth);
      Chan->_length = Head->ChanDepth;
      memcpy(Chan->begin(),Head->Channels+CurExtract*Head->ChanDepth,Head->ChanDepth*sizeof(*Head->Channels));
   }
}
   									/*}}}*/
