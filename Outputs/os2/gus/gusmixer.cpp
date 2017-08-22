/* ########################################################################

   GUSMixer - Class to allow Muse/2 to output a module to a GUS under OS/2
              using the Manley GUS drivers

   ########################################################################
*/
#ifndef CPPCOMPILE
#include <Sequence.hh>
#include <GUSMixer.hh>
#include <DebugUtils.hh>
#else
#include <IDLSeq.hc>
#include <GUSMixer.hc>
#include <DebugUtl.hc>
#include <HandLst.hc>

museGUSMixerClass *museGUSMixer::__ClassObject = new museGUSMixerClass;
#endif

#include <Flags.h>
#include <MinMax.h>

#define INCL_DOS
#include <os2.h>

#include <stdio.h>
#include <math.h>
#include <Flags.h>
#include "ultradev.h"

// Defined in a C module so it doesn't conform to normal manglings
extern "C"{
HFILE GUSHandle;
}

int PlayChan = 0;

museGUSMixer::museGUSMixer()
{
   DBU_FuncTrace("museGUSMixer","museGUSMixer",TRACE_SIMPLE);

   Channels.construct();
   Instruments.construct();
   NumChans = 0;
   PlayMode = 0;
   LastTime = 0;
   CurrentTime = 0;
   WaitTime = 0;
   Version = 0;
   MaxChannels = 32;
   Pause = false;
   Downloader = true;
   DosCreateMutexSem(0,&ComputeMutex,0,0);
   DosCreateEventSem(0,(HEV *)(&SyncSem),0,false);
}

museGUSMixer::~museGUSMixer()
{
   DBU_FuncTrace("museGUSMixer","~museGUSMixer",TRACE_SIMPLE);

   CloseGUS();
   Channels.free();
   Instruments.free();
   DosClose(ComputeMutex);
   DosPostEventSem(SyncSem);
   DosCloseEventSem(SyncSem);
}

void museGUSMixerClass::GetOptionHelp(SequenceString *)
{
}

void museGUSMixer::SetOptions(char *S)
{
   PlayChan = atoi(S);
}

string museGUSMixer::GetCurOptionDesc()
{
   char *C = (char *)SOMMalloc(300);

   if (GUSHandle == 0)
   {
      sprintf(C,"");
      return C;
   }

   int RamSize;
   UltraSizeDram(&RamSize);
   int K;
   UltraGetDriverVersion(&K);

   sprintf(C,"Driver Version %1.2f, %uK of GUS RAM",(float)(K/100.0),RamSize);
   return C;
}

void museGUSMixer::SetVolumeScale(unsigned short)
{
   DBU_FuncTrace("museGUSMixer","SetVolumeScale",TRACE_SIMPLE);
   int Chans;
   if (NumChans < 20)
      Chans = min(32,max(NumChans + 2,14));
   else
      Chans = min(32,max(NumChans,14));

   somPrintf("Chans - %u\n",(unsigned long)Chans);
   UltraSetNVoices(Chans);
   NextChan = 0;
   LastChan = Chans;
   for (int I = 0; I != Chans; I++)
      ChanList[I] = I;
   StopNotes();
};

void museGUSMixer::LoadSamples(SequenceSample *Smp)
{
   DBU_FuncTrace("museGUSMixer","LoadSamples",TRACE_SIMPLE);

   // Free existing samples
   Instruments.free();
   Instruments[Smp->size()].MemLoc = 0;       // Pre allocate

   // Determine Total Size of Insts and copy into our inst structures
   unsigned long Size = 0;
   SeqGUSInstData::iterator CurInst = Instruments.begin();
   for (SequenceSample::iterator Cur = Smp->begin(); Cur != Smp->end(); Cur++, CurInst++)
   {
      if (Cur->Sample == 0 || Cur->SampleEnd == 0 || Cur->LoopEnd == 0)
      {
         CurInst->FreqMul = 1;
         CurInst->Size = 0;
         CurInst->Flags = 0;
         continue;
      }

      if ((Cur->Flags & (1 << 0)) == 0)
      {
         CurInst->SampleEnd = Cur->SampleEnd;
         CurInst->LoopBegin = CurInst->SampleEnd;
         CurInst->LoopEnd = CurInst->LoopBegin;
         CurInst->Flags = 0;
      }
      else
      {
         CurInst->LoopBegin = min(Cur->LoopBegin,Cur->SampleEnd);
         CurInst->LoopEnd = min(Cur->LoopEnd,Cur->SampleEnd);
         CurInst->SampleEnd = min(Cur->LoopEnd,Cur->SampleEnd);
         CurInst->Flags = VC_LOOP_ENABLE;

         // Ping pong loops
         if ((Cur->Flags & (1 << 3)) != 0)
            CurInst->Flags |= VC_BI_LOOP;
      }

      // 16 bit
      if ((Cur->Flags & (1 << 1)) != 0)
      {
         CurInst->Conv16to8 = 1;
         CurInst->LoopBegin /= 2;
         CurInst->LoopEnd /= 2;
         CurInst->SampleEnd /= 2;
      }
      else
         CurInst->Conv16to8 = 0;

      CurInst->Size = CurInst->SampleEnd;
      CurInst->FreqMul = 1;
      Size = Size + CurInst->Size/CurInst->FreqMul + 50;
   }
//printf("Done1\n");
   // Generate a map
   unsigned char *SortedList = new unsigned char[Smp->size()];
   for (int I = 0; I != Smp->size();I++)
      SortedList[I] = I;

//printf("Done2\n");
   // Get GUS ram size
   int RamSize;
   UltraSizeDram(&RamSize);
   RamSize *= 1023;

//printf("Done3\n");
   /* IMM, Determine the FreqMul needed to fit all the samples into the ram,
      each inst has it's own FreqMul. Larger insts are downsampled first and
      then smaller ones */
   unsigned short IEnd = 1;
   do
   {
      // Sort it
      char Swap;
      do
      {
         Swap = 0;
         for (I = 0; I != Smp->size() - 1;I++)
         {
            if (Instruments[SortedList[I]].Size/Instruments[SortedList[I]].FreqMul <
                Instruments[SortedList[I+1]].Size/Instruments[SortedList[I+1]].FreqMul)
            {
               unsigned char Temp = SortedList[I];
               SortedList[I] = SortedList[I + 1];
               SortedList[I + 1] = Temp;
               Swap = 1;
            }
         }
      }
      while (Swap == 1);

      for (I = 0; (RamSize < Size) && (I != IEnd); I++)
      {
         if (Instruments[SortedList[I]].FreqMul == 0)
            continue;

         Size -= Instruments[SortedList[I]].Size/Instruments[SortedList[I]].FreqMul;
         Instruments[SortedList[I]].FreqMul++;
         Size += Instruments[SortedList[I]].Size/Instruments[SortedList[I]].FreqMul;

         if (Instruments[SortedList[I]].Size < 5000)
         {
            if (I == 0)
               break;
         }
      }

      if (IEnd != Smp->size())
         IEnd++;
   }
   while (RamSize < Size);

//printf("Done4\n");
   unsigned long Largest = 0;
   unsigned long Size2 = 0;
   unsigned long Offset = 64;
   for (I = 0;I != Instruments.size(); I++)
   {
      if (Instruments[I].FreqMul == 0 || Instruments[I].Size == 0)
         continue;

      Instruments[I].Size /= Instruments[I].FreqMul;
      Instruments[I].LoopBegin /= Instruments[I].FreqMul;
      Instruments[I].LoopEnd /= Instruments[I].FreqMul;
      Instruments[I].SampleEnd /= Instruments[I].FreqMul;
      Instruments[I].MemLoc = Offset;

      Offset += ceil((float)(Instruments[I].Size + 20)/32.0)*32;
      Size2 += Instruments[I].Size;
      Largest = max(Largest,Instruments[I].Size);
   }
   somPrintf("Size-%u,%u\n",Size2,Largest);
   somPrintf("IMM DONE\n");

//printf("Done5\n");
   char *Mem;
   DosAllocMem((void **)&Mem, Largest + 50, PAG_COMMIT | PAG_WRITE);
   memset(Mem,0,Largest + 50);

   CurInst = Instruments.begin();
   for (Cur = Smp->begin(); Cur != Smp->end(); Cur++, CurInst++)
   {

      if (Cur->Sample == 0 || Cur->SampleEnd == 0)
      {
         CurInst->MemLoc = 0xFFFFFFFF;
         continue;
      }
      somPrintf("I-%u,S-%u\n",CurInst - Instruments.begin(),Cur->SampleEnd);
      Offset = CurInst->MemLoc;

      if (CurInst->Conv16to8 == 1)
      {
         signed short *Src = (signed short *)Cur->Sample;
         unsigned char *Dest = (unsigned char *)Mem;
         signed short *SrcEnd = (signed short *)Cur->Sample;
         if ((Cur->Flags & (1 << 0)) == 0)
            SrcEnd += Cur->SampleEnd/2;
         else
            SrcEnd += Cur->LoopEnd/2;

         for (; Src < SrcEnd; Src += CurInst->FreqMul,Dest++)
            *Dest = ((unsigned char)((*Src)/0x100)) ^ 128;
      }
      else
      {
         unsigned char *Src = Cur->Sample;
         unsigned char *Dest = (unsigned char *)Mem;
         unsigned char *SrcEnd = Cur->Sample;
         if ((Cur->Flags & (1 << 0)) == 0)
            SrcEnd += Cur->SampleEnd;
         else
            SrcEnd += Cur->LoopEnd;

         for (; Src < SrcEnd; Src += CurInst->FreqMul,Dest++)
            *Dest = *Src;
      }

      somPrintf("Copy DONE\n");

      if ((Cur->Flags & (1 << 0)) == 0)
      {
         char *Pos = Mem + CurInst->SampleEnd;
         if (Pos > Mem)
         {
            char V = Pos[-1];
            for(int I = 0;I != 20;I++,Pos++)
               *Pos = V;
         }
      }
      else
      {
         if ((Cur->Flags & (1 << 3)) == 0)
         {
            char *Pos = Mem + CurInst->LoopEnd;
            char *V = Mem + CurInst->LoopBegin;

            if (CurInst->LoopBegin == 0) // Odd fix for trans
               V++;
            if (Pos > Mem && V > Mem)
            {
               for(int I = 0;I != 20;I++,Pos++,V++)
                  *Pos = *V;
            }
            CurInst->LoopEnd += 5;
            CurInst->LoopBegin += 5;
         }
         else
         {
            char *Pos = Mem + CurInst->LoopEnd;
            char *V = Mem + CurInst->LoopEnd - 2;

            if (Pos > Mem && V > Mem)
            {
               for(int I = 0;I != 20;I++,Pos++,V--)
                  *Pos = *V;
            }
         }
      }
      somPrintf("W");
      if (Version < 100)
      {
         if ((Cur->Flags & (1 << 2)) != 0)
            UltraDownload2(Mem,0,Offset,CurInst->Size + 20,true);
         else
            UltraDownload2(Mem,DMA_CVT_2,Offset,CurInst->Size + 20,true);
      }
      else
      {
         if ((Cur->Flags & (1 << 2)) != 0)
            UltraDownload(Mem,0,Offset,CurInst->Size + 20,true);
         else
            UltraDownload(Mem,DMA_CVT_2,Offset,CurInst->Size + 20,true);
      }

      somPrintf("A\n");
      CurInst->SampleEnd += Offset;
      CurInst->LoopBegin += Offset;
      CurInst->LoopEnd += Offset;

/*      if (CurInst->Conv16to8 != 1)
         continue;
      UltraStartVoice(0,Offset,Cur->LoopBegin,CurInst->SampleEnd,CurInst->Flags);
      UltraSetAll(0,7,8000,500,13,0);
      DosSleep(10000);
      UltraStopVoice(0);
      DosBeep(100,100);*/
   };
   DosFreeMem(Mem);
//printf("Done6\n");
}

void museGUSMixer::SetChannelMask(Sequencebool *Mask)
{
   DBU_FuncTrace("museGUSMixer","SetChannelMask",TRACE_SIMPLE);

   // Resize the channels structure
   Channels.reserve(Mask->size());
   Channels._length = Mask->size();
   NumChans = 0;

   // Copy the enabled bit to the channel structure.
   for (unsigned int I = 0;I != Mask->size(); I++)
   {
      Channels[I].Enabled = (*Mask)[I];
      Channels[I].Channel1 = -1;
      Channels[I].Channel2 = -1;
      Channels[I].FreqMul = 0;
      Channels[I].Sample = -1;
      if ((*Mask)[I] == true)
         NumChans++;
   }
};

unsigned long museGUSMixer::SecondsToLen(float Seconds)
{
//   DBU_FuncTrace("museGUSMixer","SecondsToLen",TRACE_SIMPLE);
   return Seconds/0.00008;
};

float museGUSMixer::LenToSeconds(unsigned long Len)
{
   return ((float)Len)*0.00008;
}

#define ChanFadeSpeed 32
long museGUSMixer::Compute(SequenceChannel *Chans,unsigned long Len)
{
   if (PlayMode == 2)
   {
      PlayMode = 1;
      return 1;
   }
   if (PlayMode != 1)
      return 1;

   // See if the timer has been started
   if (TempoDiv == 0)
      TempoDiv = 1;

   if (Len == 0)
      Len = SecondsToLen(0.01);

   if (LastTime != 0)
   {
      for (int I = 0; I < TempoDiv;I++)
      {
         UltraBlockTimerHandler1();
         int Len2 = LastTime;
         if (Len2 > 254)
            TempoDiv = ceil(((float)Len2)/254.0);
         else
            TempoDiv = 1;
         UltraStartTimer(1,LastTime/TempoDiv);
      }
   }
   CurrentTime += LastTime;

   // Handle pausing
   if (Pause == true)
   {
      // Setup iterators to spend as little time in this loop as possible
      SeqGUSChannelState::iterator ChCur = Channels.begin(); // Current state
      SeqGUSChannelState::iterator ChEnd = Channels.end();
      for (;ChCur != ChEnd;ChCur++)
      {
         /* Channel is disabled, so ignore completely, this flag is set
            earlier and will not change during playback */
         if (ChCur->Enabled == false || ChCur->Channel1 == -1)
            continue;

         UltraVectorLinearVolume(ChCur->Channel1,0,63,0);
      }

      while (Pause == true && PlayMode == 1)
         DosSleep(100);
      if (PlayMode != 1)
         return 1;

      ChCur = Channels.begin(); // Current state
      ChEnd = Channels.end();
      for (;ChCur != ChEnd;ChCur++)
      {
         /* Channel is disabled, so ignore completely, this flag is set
            earlier and will not change during playback */
         if (ChCur->Enabled == false || ChCur->Channel1 == -1)
            continue;

         UltraVectorLinearVolume(ChCur->Channel1,ChCur->Volume,63,0);
      }
   }

   DosRequestMutexSem(ComputeMutex,SEM_INDEFINITE_WAIT);

   // Reconfigure the timer to use the new time provided
   if (LastTime != Len)
   {
      int Len2 = Len;
      LastTime = Len;
      if (Len2 > 254)
         TempoDiv = ceil(((float)Len2)/254.0);
      else
         TempoDiv = 1;
      UltraStartTimer(1,LastTime/TempoDiv);
   }
   if (Version < 100)
      UltraStartTimer(1,LastTime/TempoDiv);

   // Setup iterators to spend as little time in this loop as possible
   SequenceChannel::iterator InCur = Chans->begin();
   SequenceChannel::iterator InEnd = Chans->end();
   SeqGUSChannelState::iterator ChCur = Channels.begin(); // Current state
   SeqGUSChannelState::iterator ChEnd = Channels.end();

   // Loop through all the channels
   for (int I = 1;(InCur != InEnd) && (ChCur != ChEnd); InCur++,ChCur++,I++)
   {
      int SetWhat = 0;

      /* Channel is disabled, so ignore completely, this flag is set
         earlier and will not change during playback */
      if (ChCur->Enabled == false)
      {
         InCur->Flags |= 1 << 31;
         continue;
      }
      if (PlayChan != 0 && I != PlayChan)
         continue;

      // Check for looped sample or non playing sample
      if (ChCur->Sample != -1 && (Instruments[ChCur->Sample].Flags & VC_LOOP_ENABLE) == 0 &&
          ChCur->Channel1 != -1)
      {
         // Reclaim stopped voices
         int I2 = ChCur->Channel1;
         int Rc = UltraVoiceStopped(&I2);

         if (I2 == 1)
         {
            ChanList[LastChan] = ChCur->Channel1;
            LastChan++;
            if (LastChan >= 33)
               LastChan = 0;
            UltraVectorLinearVolume(ChCur->Channel1,0,ChanFadeSpeed,0);
            ChCur->Channel1 = -1;
            ChCur->Channel2 = -1;
         }
      }

      // Set instrument
      char ReTrig = 0;
      if ((InCur->Flags & (1 << 0)) != 0)
      {
         // Retrigger if the instrument has changed
         if (ChCur->Sample != InCur->Sample && ChCur->Channel1 != -1)
            ReTrig = 1;

         // Range check
         if (InCur->Sample < Instruments.size())
            ChCur->Sample = InCur->Sample;
         else
         {
            ChCur->Sample = -1;
         }
      }

      // Set volume
      if ((InCur->Flags & (1 << 1)) != 0)
      {
         int Volume = ChCur->Volume;
         ChCur->Volume = (bound(0L,InCur->MainVol,VolMax)*511)/VolMax;
         if (InCur->Pan == PanSurround)
            ChCur->TargetB = 7;
         else
            ChCur->TargetB = ((bound(0L,InCur->Pan + PanMax,PanSpan))*0xF)/PanSpan;

         if (ChCur->Volume != Volume)
            SetWhat |= 1;
      }

      // Stop playing this voice
      if ((InCur->Flags & (1 << 2)) != 0)
      {
         if (ChCur->Channel1 != -1)
         {
            ChanList[LastChan] = ChCur->Channel1;
            LastChan++;
            if (LastChan >= 33)
               LastChan = 0;
            UltraVectorLinearVolume(ChCur->Channel1,0,ChanFadeSpeed,0);
            ChCur->Channel1 = -1;
            ChCur->Channel2 = -1;
         }
         ReTrig = 0;
      }

      // Change pitch
      if ((InCur->Flags & (1 << 4)) != 0)
      {
         // Pitch is in Hz and already is corrected for the samples center freq
         if (ChCur->Pitch != InCur->Pitch)
         {
            ChCur->Pitch = InCur->Pitch;
            SetWhat |= 2;
         }
      }

      // Start the sample playing
      if ((InCur->Flags & (1 << 3)) != 0 || ReTrig == 1)
      {
         if (ChCur->Channel1 != -1)
         {
            ChanList[LastChan] = ChCur->Channel1;
            LastChan++;
            if (LastChan >= 33)
               LastChan = 0;
            UltraVectorLinearVolume(ChCur->Channel1,0,ChanFadeSpeed,0);
            ChCur->Channel1 = -1;
            ChCur->Channel2 = -1;
         }

         if (ChCur->Sample != -1)
         {
            // Get the sample to play
            SeqGUSInstData::iterator ICur = &Instruments[ChCur->Sample];

            // Ensure a sample is present
            if (ICur->MemLoc != 0xFFFFFFFF)
            {
               ChCur->Channel1 = ChanList[NextChan];
               if (NextChan == LastChan)
                  ChCur->Channel1 = -1;
               else
               {
                  NextChan++;
                  if (NextChan >= 33)
                     NextChan = 0;
                  UltraStartVoice(ChCur->Channel1,
                    min(ICur->MemLoc + InCur->SampleOffset,ICur->LoopEnd),
                                 ICur->LoopBegin,ICur->LoopEnd,ICur->Flags);
                  SetWhat |= 3;
               }
               ChCur->FreqMul = ICur->FreqMul;
            }
         }
      }

      if (ChCur->Channel1 == -1)
      {
         InCur->Flags |= 1 << 31;
         continue;
      }

      // Umm, Wha? (Happens sometimes)
      if (ChCur->FreqMul == 0)
      {
         somPrintf("0 freqmul\n");
         ChanList[LastChan] = ChCur->Channel1;
         LastChan++;
         if (LastChan >= 33)
            LastChan = 0;
         UltraVectorLinearVolume(ChCur->Channel1,0,01,0);
         ChCur->Channel1 = -1;
         ChCur->Channel2 = -1;
         continue;
      }

      if (ChCur->Balance != ChCur->TargetB)
      {
/*         if (ChCur->Balance > ChCur->TargetB)
            ChCur->Balance--;
         else
            ChCur->Balance++;  */
         ChCur->Balance = ChCur->TargetB;
         SetWhat |= 3;
      }

      if ((SetWhat & 3) == 3)
      {
         UltraSetAll(ChCur->Channel1,ChCur->Balance,ChCur->Pitch/ChCur->FreqMul,ChCur->Volume,13,0);
         SetWhat = 0;
      }
      if ((SetWhat & 1) != 0)
         UltraVectorLinearVolume(ChCur->Channel1,ChCur->Volume,13,0);
      if ((SetWhat & 2) != 0)
         UltraSetFrequency(ChCur->Channel1,ChCur->Pitch/ChCur->FreqMul);
   };
   DosReleaseMutexSem(ComputeMutex);

   // Extra channels
   for (;InCur != InEnd; InCur++)
      InCur->Flags |= 1 << 31;

   if (WaitTime != 0xFFFFFFFF)
      if (CurrentTime >= WaitTime)
      {
         WaitTime = 0xFFFFFFFF;
         DosPostEventSem(SyncSem);
      }
   return 0;
};
unsigned long museGUSMixer::Sync(unsigned long Time)
{
   if (Time >= CurrentTime)
   {
      WaitTime = Time;
      ULONG Jnk;
      DosWaitEventSem((HEV)SyncSem,-1);
      DosResetEventSem((HEV)SyncSem,&Jnk);
   }
   return CurrentTime;
}
void museGUSMixer::StopNotes()
{
   DBU_FuncTrace("museGUSMixer","StopNotes",TRACE_SIMPLE);
   for (int I = 0; I != 32; I++)
   {
      UltraVectorLinearVolume(I,0,10,0);
      UltraStopVoice(I);
      UltraStartVoice(I,0,0,0,0);
   }
   LastTime = 0;
   UltraStopTimer(1);
}

void museGUSMixer::StopWhenDone()
{
   if (GUSHandle == 0)
      return;

   DBU_FuncTrace("museGUSMixer","StopWhenDone",TRACE_SIMPLE);

   // See if the timer has been started
   if (LastTime != 0)
      UltraBlockTimerHandler1();

   StopPlay();
   CloseGUS();
}

long museGUSMixer::InitPlay(string *Error)
{
   DBU_FuncTrace("museGUSMixer","InitPlay",TRACE_SIMPLE);

   *Error = 0;

   long Rc = 0;
   if ((Rc = OpenGUS(Error)) != 0)
      return Rc;

   StopNotes();

   PlayMode = 1;

   CurrentTime = 0;
   WaitTime = 0xFFFFFFFF;
   return 0;
}

long museGUSMixer::OpenGUS(string *Error)
{
   DBU_FuncTrace("museGUSMixer","OpenGUS",TRACE_SIMPLE);
   *Error = 0;

   APIRET status;
   ULONG action;
   int k;

   status = DosOpen( "ULTRA1$", &GUSHandle, &action, 0,
			   FILE_NORMAL, FILE_OPEN, OPEN_ACCESS_READWRITE |
            OPEN_SHARE_DENYNONE |
            OPEN_FLAGS_WRITE_THROUGH |
            OPEN_FLAGS_NOINHERIT, NULL);
   somPrintf("GUSHandle is %u\n",(unsigned long)GUSHandle);

   if (status != 0)
   {
      *Error = "Cannot open ULTRA1$ device, are the Manley drivers installed?";
      return 1;
   }

   if (UltraGetAccess() != 0)
   {
      UltraReleaseAccess();
   	DosClose(GUSHandle);
   	*Error = "Cannot gain control of the device, another app has it";
	   return 1;
   }

   if (UltraGetDriverVersion(&k) != 0)
   {
      UltraReleaseAccess();
	   DosClose(GUSHandle);
	   *Error = "Driver is too old, you must have at least V0.85";
   	return 1;
   }

   if (k < 85)
   {
      UltraReleaseAccess();
	   DosClose(GUSHandle);
	   *Error = "Driver is too old, you must have at least V0.85";
   	return false;
   }

   Version = k;

   LastTime = 0;
   RealTime = true;
   return 0;
}

long museGUSMixer::CloseGUS()
{
   DBU_FuncTrace("museGUSMixer","CloseGUS",TRACE_SIMPLE);

   // Force Sync to leave
   CurrentTime = 0xFFFFFFFF;
   DosPostEventSem(SyncSem);
   StopNotes();
   UltraReleaseAccess();
   DosClose(GUSHandle);
   return 0;
}

long museGUSMixer::StopPlay()
{
   DBU_FuncTrace("museGUSMixer","StopPlay",TRACE_SIMPLE);

   StopNotes();

   LastTime = 0;
   PlayMode = 3;
   return 0;
}

void museGUSMixer::ForceCompError()
{
   PlayMode = 2;
};

void museGUSMixer::PausePlay()
{
   DBU_FuncTrace("museGUSMixer","PausePlay",TRACE_SIMPLE);
   Pause = true;
}

void museGUSMixer::ResumePlay()
{
   DBU_FuncTrace("museGUSMixer","ResumePlay",TRACE_SIMPLE);
   Pause = false;
}

long museGUSMixer::TestOutput()
{
   DBU_FuncTrace("museGUSMixer","TestOutput",TRACE_SIMPLE);
   return 0;
}

/* ########################################################################

   Class - museGUSMixerClass (Class of museGUSMMPM2)
   Member - Constructor (Inits the class)

   Sets up the class.

   ########################################################################
*/
museGUSMixerClass::museGUSMixerClass()
{
   DetectStatus = 0;
   Level = 5;
#ifdef CPPCOMPILE
   MajorVersion = 1;
   MinorVersion = 3;
   museHandlerList::__ClassObject->AddOutput(this);
#endif
}

/* ########################################################################

   Class - museGUSMixerClass (Class of museGUSMMPM2)
   Member - GetTypeName (Returns the name of this class)
   IDL - string GetTypeName

   Returns a pointer to a string containing 'Gravis UltraSound'.
   This string is valid while the object exists.

   ########################################################################
*/
string museGUSMixerClass::GetTypeName()
{
   return "Gravis UltraSound";
}

octet museGUSMixerClass::IsSupported()
{
   if (DetectStatus == 1)
      return 0;
   if (DetectStatus == 2)
      return 5;

   APIRET status;
   ULONG action;
   int k;

   status = DosOpen( "ULTRA1$", &GUSHandle, &action, 0,
			   FILE_NORMAL, FILE_OPEN, OPEN_ACCESS_READWRITE |
            OPEN_SHARE_DENYNONE |
            OPEN_FLAGS_WRITE_THROUGH, NULL);

   if (status != 0)
   {
      DetectStatus = 1;
      return 0;
   }

   if (UltraGetAccess() != 0)
   {
      UltraReleaseAccess();
   	DosClose(GUSHandle);
      DetectStatus = 1;
      return 0;
   }

   if (UltraGetDriverVersion(&k) != 0)
   {
      UltraReleaseAccess();
	   DosClose(GUSHandle);
      DetectStatus = 1;
      return 0;
   }

   if (k < 85)
   {
      UltraReleaseAccess();
   	DosClose(GUSHandle);
      DetectStatus = 1;
      return 0;
   }

   UltraReleaseAccess();
	DosClose(GUSHandle);
   DetectStatus = 2;
   return 5;
}

#ifndef CPPCOMPILE
extern "C"
{
   void QueryOutputHandlers(Sequence<museOutputClass *> *);
}

void QueryOutputHandlers(Sequence<museOutputClass *> *List)
{
   List->construct();
   (*List)[0] = (museOutputClass *)museGUSMixer::__ClassObject;
}
#endif
