/* ########################################################################

   WaveTable mixer front end.

   Provides an intermediate interface between hardware wavetable drivers
   and the muse mixer engine.

   ########################################################################
*/
#include <Muse.h>
#include <WaveTbl.hc>

#include <stdio.h>
#include <math.h>

#define VC_DATA_TYPE            0x04            /* 0=8 bit,1=16 bit */
#define VC_LOOP_ENABLE          0x08            /* 1=enable */
#define VC_BI_LOOP		0x10		/* 1=bi directional looping */

museWaveTable::museWaveTable()
{
   DBU_FuncTrace("museWaveTable","museWaveTable",TRACE_SIMPLE);

   Channels.construct();
   Instruments.construct();
   NumChans = 0;
   PlayMode = 0;
   LastTime = 0;
   CurrentTime = 0;
   WaitTime = 0;
   MaxChannels = 32;
   Pause = false;
   Downloader = true;
   DosCreateMutexSem(0,&ComputeMutex,0,0);
   DosCreateEventSem(0,(HEV *)(&SyncSem),0,false);
}

museWaveTable::~museWaveTable()
{
   DBU_FuncTrace("museWaveTable","~museWaveTable",TRACE_SIMPLE);

   CloseDevice();
   Channels.free();
   Instruments.free();
   DosClose(ComputeMutex);
   DosPostEventSem(SyncSem);
   DosCloseEventSem(SyncSem);
}

void museWaveTable::SetVolumeScale(unsigned short)
{
   DBU_FuncTrace("museWaveTable","SetVolumeScale",TRACE_SIMPLE);
   int Chans;
   if (NumChans < MidChans)
      Chans = min(MaxChans,max(NumChans + 2,MinChans));
   else
      Chans = min(MaxChans,max(NumChans,14));

   somPrintf("Chans - %u\n",(unsigned long)Chans);
   SetVoiceCount(Chans);
   NextChan = 0;
   LastChan = Chans;
   for (int I = 0; I != Chans; I++)
      ChanList[I] = I;
   StopNotes();
};

void museWaveTable::LoadSamples(SequenceSample *Smp)
{
   DBU_FuncTrace("museWaveTable","LoadSamples",TRACE_SIMPLE);

   // Free existing samples
   Instruments.free();
   Instruments[Smp->size()].MemLoc = 0;       // Pre allocate

   // Determine Total Size of Insts and copy into our inst structures
   unsigned long Size = 0;
   SeqWavInstData::iterator CurInst = Instruments.begin();
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
   int RamSize = RAMSize();

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
   Mem = (char *)malloc(Largest+50);
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
      if ((Cur->Flags & (1 << 2)) != 0)
         Download(Mem,0,Offset,CurInst->Size + 20);
      else
         Download(Mem,1,Offset,CurInst->Size + 20);

      somPrintf("A\n");
      CurInst->SampleEnd += Offset;
      CurInst->LoopBegin += Offset;
      CurInst->LoopEnd += Offset;
   };
   free(Mem);
//printf("Done6\n");
}

void museWaveTable::SetChannelMask(Sequencebool *Mask)
{
   DBU_FuncTrace("museWaveTable","SetChannelMask",TRACE_SIMPLE);

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

unsigned long museWaveTable::SecondsToLen(float Seconds)
{
//   DBU_FuncTrace("museWaveTable","SecondsToLen",TRACE_SIMPLE);
   return Seconds/SecDiv;
};

float museWaveTable::LenToSeconds(unsigned long Len)
{
   return ((float)Len)*SecDiv;
}

#define ChanFadeSpeed 32
long museWaveTable::Compute(SequenceChannel *Chans,unsigned long Len)
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
         WaitTimeTick();
         int Len2 = LastTime;
         if (Len2 > MaxTime)
            TempoDiv = ceil(((float)Len2)/((float)MaxTime));
         else
            TempoDiv = 1;
         StartTimer(LastTime/TempoDiv);
      }
   }
   CurrentTime += LastTime;

   // Handle pausing
   if (Pause == true)
   {
      // Setup iterators to spend as little time in this loop as possible
      SeqWavChannelState::iterator ChCur = Channels.begin(); // Current state
      SeqWavChannelState::iterator ChEnd = Channels.end();
      for (;ChCur != ChEnd;ChCur++)
      {
         /* Channel is disabled, so ignore completely, this flag is set
            earlier and will not change during playback */
         if (ChCur->Enabled == false || ChCur->Channel1 == -1)
            continue;

         SetVolume(ChCur->Channel1,0,63);
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

         SetVolume(ChCur->Channel1,ChCur->Volume,63);
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
      StartTimer(LastTime/TempoDiv);
   }

   // Setup iterators to spend as little time in this loop as possible
   SequenceChannel::iterator InCur = Chans->begin();
   SequenceChannel::iterator InEnd = Chans->end();
   SeqWavChannelState::iterator ChCur = Channels.begin(); // Current state
   SeqWavChannelState::iterator ChEnd = Channels.end();

   // Loop through all the channels
   for (int I = 1;(InCur != InEnd) && (ChCur != ChEnd); InCur++,ChCur++,I++)
   {
      int SetWhat = 0;

      /* Channel is disabled, so ignore completely, this flag is set
         earlier and will not change during playback */
      if (ChCur->Enabled == false)
      {
         InCur->Flags |= CHAN_Free;
         continue;
      }

      // Check for looped sample or non playing sample
      if (ChCur->Sample != -1 && (Instruments[ChCur->Sample].Flags & VC_LOOP_ENABLE) == 0 &&
          ChCur->Channel1 != -1)
      {
         // Reclaim stopped voices
         if (IsStopped(ChCur->Channel1) == 1)
         {
            ChanList[LastChan] = ChCur->Channel1;
            LastChan++;
            if (LastChan >= 33)
               LastChan = 0;
            SetVolume(ChCur->Channel1,0,ChanFadeSpeed);
            ChCur->Channel1 = -1;
            ChCur->Channel2 = -1;
         }
      }

      // Set instrument
      char ReTrig = 0;
      if ((InCur->Flags & CHAN_Instrument) != 0)
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
      if ((InCur->Flags & CHAN_Volume) != 0)
      {
         int Volume = ChCur->Volume;
         int Pan = ChCur->Pan;

         ChCur->Volume = bound(0L,InCur->MainVol,VolMax);
         if (InCur->Pan == PanSurround)
            ChCur->Pan = 0;
         else
            ChCur->Pan = bound(-PanMax,InCur->Pan,PanMax);
         if (ChCur->Pan != Pan)
            SetWhat |= 3;

         if (ChCur->Volume != Volume)
            SetWhat |= 1;
      }

      // Stop playing this voice
      if ((InCur->Flags & CHAN_Cut) != 0)
      {
         if (ChCur->Channel1 != -1)
         {
            ChanList[LastChan] = ChCur->Channel1;
            LastChan++;
            if (LastChan >= 33)
               LastChan = 0;
            SetVolume(ChCur->Channel1,0,ChanFadeSpeed);
            ChCur->Channel1 = -1;
            ChCur->Channel2 = -1;
         }
         ReTrig = 0;
      }

      // Change pitch
      if ((InCur->Flags & CHAN_Pitch) != 0)
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
            SetVolume(ChCur->Channel1,0,ChanFadeSpeed);
            ChCur->Channel1 = -1;
            ChCur->Channel2 = -1;
         }

         if (ChCur->Sample != -1)
         {
            // Get the sample to play
            SeqWavInstData::iterator ICur = &Instruments[ChCur->Sample];

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
                  StartVoice(ChCur->Channel1,
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
         SetVolume(ChCur->Channel1,0,1);
         ChCur->Channel1 = -1;
         ChCur->Channel2 = -1;
         continue;
      }

      if ((SetWhat & 3) == 3)
      {
         SetAll(ChCur->Channel1,ChCur->Pan,ChCur->Pitch/ChCur->FreqMul,ChCur->Volume,13);
         SetWhat = 0;
      }
      if ((SetWhat & 1) != 0)
         SetVolume(ChCur->Channel1,ChCur->Volume,13);
      if ((SetWhat & 2) != 0)
         SetFrequency(ChCur->Channel1,ChCur->Pitch/ChCur->FreqMul);
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
unsigned long museWaveTable::Sync(unsigned long Time)
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
void museWaveTable::StopNotes()
{
   DBU_FuncTrace("museWaveTable","StopNotes",TRACE_SIMPLE);
   for (int I = 0; I != 32; I++)
   {
      SetVolume(I,0,10);
      StopVoice(I);
      StartVoice(I,0,0,0,0);
   }
   LastTime = 0;
   StopTimer();
}

void museWaveTable::StopWhenDone()
{
   DBU_FuncTrace("museWaveTable","StopWhenDone",TRACE_SIMPLE);

   // See if the timer has been started
   if (LastTime != 0)
      WaitTimeTick();

   StopPlay();
   CloseDevice();
}

long museWaveTable::InitPlay(string *Error)
{
   DBU_FuncTrace("museWaveTable","InitPlay",TRACE_SIMPLE);

   *Error = 0;

   long Rc = 0;
   if ((Rc = OpenDevice(Error)) != 0)
      return Rc;

   StopNotes();

   PlayMode = 1;

   CurrentTime = 0;
   WaitTime = 0xFFFFFFFF;
   return 0;
}

long museWaveTable::StopPlay()
{
   DBU_FuncTrace("museWaveTable","StopPlay",TRACE_SIMPLE);

   StopNotes();

   LastTime = 0;
   PlayMode = 3;
   return 0;
}

void museWaveTable::ForceCompError()
{
   PlayMode = 2;
};

void museWaveTable::PausePlay()
{
   DBU_FuncTrace("museWaveTable","PausePlay",TRACE_SIMPLE);
   Pause = true;
}

void museWaveTable::ResumePlay()
{
   DBU_FuncTrace("museWaveTable","ResumePlay",TRACE_SIMPLE);
   Pause = false;
}

long museWaveTable::TestOutput()
{
   DBU_FuncTrace("museWaveTable","TestOutput",TRACE_SIMPLE);
   return 0;
}

/* ########################################################################

   Class - museWaveTableClass (Class of museGUSMMPM2)
   Member - Constructor (Inits the class)

   Sets up the class.

   ########################################################################
*/
museWaveTableClass::museWaveTableClass()
{
#ifdef CPPCOMPILE
   MajorVersion = 1;
   MinorVersion = 0;
#endif
}
#ifdef CPPCOMPILE
museWaveTableClass *museWaveTable::__ClassObject = new museWaveTableClass;
#endif
