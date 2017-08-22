// -*- mode: cpp; mode: fold -*-
// Description								/*{{{*/
/* ######################################################################

   DACMixer - Class to allow Muse/2 to output a module to a DAC.

   This class supports a range of output formats from 8KHz stereo 8Bit to
   44KHz stero 16 bit with both 16 and 8 bit samples. The module handler
   calls Calc each cycle (called a frame in S3M's) which will mix that short
   period into the output buffer.
   
   This class also serves as a basis for all of the digital output devices.
   
   Mixing is actually performed by a set of lower level classes. This one
   only implements the high level interface for DAC devices and high level
   processing of pattern data.

   ##################################################################### */
									/*}}}*/
// Includes								/*{{{*/
#include <muse.h>
#include <dac.h>
#include <math.h>
#include <cmdline.h>
   									/*}}}*/

museFileOutputClass *museFileOutput::Meta = new museFileOutputClass;
museDACMixerClass *museDACMixer::Meta = new museDACMixerClass;

template class Sequence<museDACMixer::MixChannel>;

// DACMixer::museDACMixer - Constructor					/*{{{*/
// ---------------------------------------------------------------------
/* Just zeros data members */
museDACMixer::museDACMixer()
{
   ForceCError = 0;
   NumChans = 0;
   Surround = true;
   ScalePercent = 0;
   ScalingConstant = 1;

   SRate = 0;
   MBits = 0;
   MStereo = 1;
   MaxChannels = 64;

   Mixer = 0;
   OutFilter = 0;
   OutBegin = 0;
   OutEnd = 0;
}
									/*}}}*/
// DACMixer::~museDACMixer - Destructor					/*{{{*/
// ---------------------------------------------------------------------
/* Frees the memory allocated by the sequences */
museDACMixer::~museDACMixer()
{
   Samples.free();
   Chns.free();
   FilterStack.free();
}
									/*}}}*/
// DACMixer::SetMaxChannels - Confiugre the soft upper chan limit	/*{{{*/
// ---------------------------------------------------------------------
/* This is used by other bits of the program to determine what the 
   recommended max number of channels is. (Impulse Tracker) */
unsigned long museDACMixer::SetMaxChannels(unsigned long Count)
{
   if (Count == 0)
      return 256;

   MaxChannels = min(Count,256);
   return MaxChannels;
}
									/*}}}*/
// DACMixer::InitPlay - Set default values.				/*{{{*/
// ---------------------------------------------------------------------
/* This will read the information parsed by the INI class. If someone
   has called a function that changes the default before hand then 
   that value will be used. */
long museDACMixer::InitPlay(char **Error)
{
   *Error = 0;
   
   // This is the list of all options the class supports.
   IniOptions *Ini = GetIniOptions();

   // Set stereo (this will override any previous setting) <ik>
   if (Ini->IsFlag("stereo") == 1)
      MStereo = true;

   if (Ini->IsFlag("mono") == 1)
      MStereo = false;
   SetMixParams(0,0,MStereo);
   
   // Change the bit rate
   if (MBits == 0)
   {
      if (Ini->IsFlag("8bit") == 1)
	 MBits = 8;

      if (Ini->IsFlag("16bit") == 1)
	 MBits = 16;

      if (MBits != 2)
	 SetMixParams(0,MBits,MStereo);
   }
   
   // Change the srate
   if (SRate == 0)
      SetMixParams(atoi(Ini->GetArg("srate","0")),0,MStereo);
   
   // Set surround sound (this will override any previous setting) <ik>
   if (Ini->IsFlag("surround") == 1)
      Surround = true;
   else
      Surround = false;

   // Set the scale factor.
   if (ScalePercent == 0)
      ScalePercent = atoi(Ini->GetArg("scale","100"));
   
   return 0;
}
									/*}}}*/
// DACMixer::GetCurOptionDesc - Returns the current configuration	/*{{{*/
// ---------------------------------------------------------------------
/* This returns the current device's configuration. */
char *museDACMixer::GetCurOptionDesc()
{
   char *C = new char[300];
   sprintf(C,"Mixing at %luHz, %lu bit %s %lu%% Scale.",
           (unsigned long)SRate,(unsigned long)MBits,
           (MStereo == true?"Stereo":"Mono"),ScalePercent);
   return C;
}
									/*}}}*/
// DACMixer::SetVolumeScale - Sets the scaling parameters		/*{{{*/
// ---------------------------------------------------------------------
/* Sets the overall mixing parameters, the volume scaling factor. This
   function should be called after the channel mask is set. For 
   historical reasons this functiona accepts a short. That short used
   to be the volume scaling factor stored in s3m files. */
void museDACMixer::SetVolumeScale(unsigned short)
{
   if (Mixer != 0)
      ScalingConstant = Mixer->ComputeSConst(ScalePercent,NumChans);

   ChanVolPot = NumChans*VolMax;
};
									/*}}}*/
// DAXMixer::SetScale - Set the scaling percentage.			/*{{{*/
// ---------------------------------------------------------------------
/* This provides the amplification that is required for high channel
   modules to be played at an audible volume level. */
unsigned long museDACMixer::SetScale(unsigned short Percent)
{
   if (Percent == 0)
      return ScalePercent;

   ScalePercent = Percent;

   if (Mixer != 0)
      ScalingConstant = Mixer->ComputeSConst(ScalePercent,NumChans);

   ChanVolPot = NumChans*VolMax;
   return ScalePercent;
}
									/*}}}*/
// DACMixer::SetMixParams - Stores the mixer parameters			/*{{{*/
// ---------------------------------------------------------------------
/* Stash away the mixer parameters so we know what we are mixing into. */
long museDACMixer::SetMixParams(unsigned long SRate, octet Bits, bool Stereo)
{
   this->SRate = SRate;
   MStereo = Stereo;
   MBits = Bits;
   return 0;
}
									/*}}}*/
// DACMixer::GetMixParams - Returns the current set of mixer params	/*{{{*/
// ---------------------------------------------------------------------
/* */
void museDACMixer::GetMixParams(unsigned long *SRate, octet *Bits, bool *Stereo)
{
   *SRate = this->SRate;
   *Stereo = MStereo;
   *Bits = MBits;
}
									/*}}}*/
// DACMixer::UseMixer - Select a new base mixer				/*{{{*/
// ---------------------------------------------------------------------
/* This member can change the active mixer even while a song is being 
   played. The old mixer is returned and should be freed if it was
   dynamically allocated. */
museMixerBase *museDACMixer::UseMixer(museMixerBase *Mixer)
{
   ComputeMutex.Lock();

   museMixerBase *old = this->Mixer;

   this->Mixer = Mixer;
   Mixer->Owner = this;

   // Havent inited fully yet
   if (Chns.size() == 0)
   {
      ComputeMutex.UnLock();
      return old;
   }

   Mixer->Reset();
   Mixer->Channels(Chns.size());

   // Start all the channels that are playing now
   for (MixChannel *I = Chns.begin(); I != Chns.end(); I++)
   {
      if (I->Playing == false)
         continue;

      Mixer->ChanInst(I - Chns.begin());
      Mixer->StartChan(I - Chns.begin());
      Mixer->ChanPitch(I - Chns.begin());
      Mixer->ChanVolume(I - Chns.begin());
   }

   ComputeMutex.UnLock();
   return old;
}
   									/*}}}*/
// DACMixer::UseOFilter - Change the output filter			/*{{{*/
// ---------------------------------------------------------------------
/* Alters the output filter, can be called during computations */
museOutputFilterBase *museDACMixer::UseOFilter(museOutputFilterBase *Filter)
{
   ComputeMutex.Lock();

   museOutputFilterBase *out = OutFilter;

   OutFilter = Filter;
   OutFilter->Owner = this;

   ComputeMutex.UnLock();
   return out;
}
									/*}}}*/
// DACMixer::AddFilter - Adds a filter to the filter stack		/*{{{*/
// ---------------------------------------------------------------------
/* This adds a digital filter into the pre-output filter stack. */
 void museDACMixer::AddFilter(museFilterBase *Filter)
{
   ComputeMutex.Lock();

   Filter->Owner = this;
   FilterStack.push_backv(Filter);

   ComputeMutex.UnLock();
}
									/*}}}*/
// DACMixer::RemoveFilter - Removes a filter from the filter stack	/*{{{*/
// ---------------------------------------------------------------------
/* */
void museDACMixer::RemoveFilter(museFilterBase *Filter)
{
   ComputeMutex.Lock();

   for (museFilterBase **I = FilterStack.begin(); I != FilterStack.end(); I++)
      if (*I == Filter)
         FilterStack.erase(I);

   ComputeMutex.UnLock();
}
									/*}}}*/
// DACMixer::GetFilterStack - Returns the filter stack			/*{{{*/
// ---------------------------------------------------------------------
/* */
 void museDACMixer::GetFilterStack(SequenceFilter *Filters)
{
   ComputeMutex.Lock();

   Filters->construct();
   Filters->Duplicate(FilterStack);

   ComputeMutex.UnLock();
}
									/*}}}*/
// DACMixer::SetFilterStack - Assumes control of a given set of filters	/*{{{*/
// ---------------------------------------------------------------------
/* */
 void museDACMixer::SetFilterStack(SequenceFilter *Filters)
{
   ComputeMutex.Lock();

   FilterStack.Duplicate(*Filters);
   for (museFilterBase **I = FilterStack.begin(); I != FilterStack.end(); I++)
      (*I)->Owner = this;

   ComputeMutex.UnLock();
}
									/*}}}*/

// DACMixer::LoadSamples - Download samples to the mixer device		/*{{{*/
// ---------------------------------------------------------------------
/* This just makes a copy of the passed sample structure, the mixer uses 
   the same format as the players */
void museDACMixer::LoadSamples(SequenceSample *Smp)
{
   Samples.Duplicate(*Smp);
};
									/*}}}*/
// DACMixer::SetChannelMask - Configure 'on' channels.			/*{{{*/
// ---------------------------------------------------------------------
/* Determines which channels are on and off. The mask sequence can be
   of unlimited size, but each channel will be very very quiet if too
   many channels are enabled. This is a direct mapping from the channel 
   structures in many file format. */
void museDACMixer::SetChannelMask(Sequencebool *Mask)
{
   // Resize the channels structure
   NumChans = 0;

   // Copy the enabled bit to the channel structure.
   Chns.free();
   Chns.reserve(Mask->size());
   for (unsigned int I = 0;I != Mask->size(); I++)
   {
      if ((*Mask)[I] == true)
      {
         NumChans++;
         Chns[I].Enabled = 1;
      }
      Chns[I].CurInst = 0xFFFF;
   }

   if (Mixer != 0);
      Mixer->Channels(Mask->size());
};
									/*}}}*/
// DACMixer::SecondsToLen - Convert time to device units		/*{{{*/
// ---------------------------------------------------------------------
/* The digital mixer uses the sampling rate as a conversion factor for 
   time, this puts the DUs into samples to mix. */
#define round(x)((x)-floor(x) < 0.5?floor(x):ceil(x))
unsigned long museDACMixer::SecondsToLen(float Seconds)
{
   return (unsigned long)round(Seconds*((float)SRate));
};
									/*}}}*/
// DACMixer::LenToSeconds - Inverse of the Seconds to Len		/*{{{*/
// ---------------------------------------------------------------------
/* Simply converts the DUs into seconds. */
float museDACMixer::LenToSeconds(unsigned long Len)
{
   return ((float)Len)/((float)SRate);
}
									/*}}}*/
// DACMixer::Interp - Performs an N point interpolation between volumes./*{{{*/
// ---------------------------------------------------------------------
/* This is part of the experimental post/pre volume slider. */
void museDACMixer::Interp(Vol &MV,Vol &VF,Vol &VT,signed long Count,signed long Steps)
{
   MV.L = VF.L + ((VT.L - VF.L)*Count)/Steps;
   MV.R = VF.R + ((VT.R - VF.R)*Count)/Steps;
   MV.M = VF.M + ((VT.M - VF.M)*Count)/Steps;
}
									/*}}}*/
// DACMixer::Compute - Play pattern data				/*{{{*/
// ---------------------------------------------------------------------
/* This is the front end for the actual mixer, it convertes the change
   information into actual values in the ChannelRecord which is used by
   the mixer. It calls functions in the low level mixer to alter the
   channel state.

   Carefull, ordering matters */
long museDACMixer::Compute(SequenceChannel *Chans,unsigned long Len)
{
   // Clone the last frame
   if (LastData.size() == 0)
   {
      LastData.Duplicate(*Chans);
      return 0;
   }

   ComputeMutex.Lock();

   // Experimental VSlider
   long Steps = 1;
   long MidPoint = (long)ceil(((float)Steps)/2.0);
   boundv(1,Steps,10);
   unsigned long long StepLen = Len/Steps;
   if (StepLen > Len) 
      StepLen = Len;

   // Error check
   if (Len == 0 || Mixer == 0)
      return 1;

   // The structure we are setting up
   MixChannel *ChCur = Chns.begin();
   MixChannel *ChEnd = Chns.end();

   /* Input structures. This mixer mixes one frame behind the actual play
      point so that it can know the volume changes that are occuring.
      This does have some odd side effects but it does allow interpolation. */
   SequenceChannel::iterator InCur = LastData.begin();
   SequenceChannel::iterator InEnd = LastData.end();
   SequenceChannel::iterator In2Cur = Chans->begin();
   SequenceChannel::iterator In2End = Chans->end();

   int I = 0;
   for (;(InCur != InEnd) && (ChCur != ChEnd); InCur++,ChCur++,I++,In2Cur++)
   {
      if (InCur->Flags == 0)
      {
         // Compute the future volume on this channel
         if (In2Cur < In2End && (In2Cur->Flags & CHAN_Volume) != 0)
         {
            ChCur->V[2].L = bound(-1*VolMax,In2Cur->LeftVol,VolMax);
            ChCur->V[2].R = bound(-1*VolMax,In2Cur->RightVol,VolMax);
            ChCur->V[2].M = bound(0L,In2Cur->MainVol,VolMax);
	    
	    // Surround is a simple waveform inversion on one channel
            if (Surround == false)
            {
               if (ChCur->V[2].L < 0)
                  ChCur->V[2].L = -1*ChCur->V[2].L;
               if (ChCur->V[2].R < 0)
                  ChCur->V[2].R = -1*ChCur->V[2].R;
	    }
         }
         else
         {
            ChCur->V[2] = ChCur->V[1];
         }

         continue;
      }
      
      // We perform all the operations at once when we decide what to do.
      int Volume = 0;
      int Inst = 0;
      int Pitch = 0;
      int Start = 0;
      int Stop = 0;

      // Stop playing now
      if ((InCur->Flags & CHAN_Cut) != 0)
      {
         if (ChCur->Playing == 1)
            Stop = 1;
      }

      // Recompute the instrument if the loop was changed
      if ((InCur->Flags & CHAN_Retrig) != 0 && ChCur->ReLoop == 1 && 
	  (InCur->Flags & CHAN_Instrument) == 0)
      {
         InCur->Flags &= CHAN_Instrument;
         InCur->Sample = ChCur->CurInst;
      }

      // Set instrument
      if ((InCur->Flags & CHAN_Instrument) != 0 &&
          (InCur->Sample != ChCur->CurInst))
      {
	 // Woops, bad sample.
         if (InCur->Sample >= Samples.size() || Samples[InCur->Sample].Sample == 0)
         {
            ChCur->PlayPos = 0;
            ChCur->PlayStart = 0;
            ChCur->EndPos = 0;
            ChCur->LoopStart = 0;
            ChCur->LoopEnd = 0;
            Stop = 1;
         }
         else
         {
            ChCur->CurInst = InCur->Sample;
            ChCur->ReLoop = 0;
            museSample &Samp = Samples[InCur->Sample];

            // Set the start point
            ChCur->PlayPos = Samp.Sample;
            ChCur->PlayStart = Samp.Sample;

            // 16 bit flag
            if ((Samp.Flags & INST_16Bit) != 0)
               ChCur->Bit16 = 1;
            else
               ChCur->Bit16 = 0;

            // Setup the loops and the end point
            ChCur->LoopStart = Samp.Sample + Samp.LoopBegin;
            ChCur->LoopEnd = Samp.Sample + Samp.LoopEnd;
            ChCur->EndPos = Samp.Sample + Samp.SampleEnd;

            // Disable looping
            if (((ChCur->LoopEnd) <= ChCur->LoopStart) ||
               ((Samp.Flags & INST_Loop) == 0))
            {
               ChCur->LoopStart = 0;
               ChCur->LoopEnd = ChCur->EndPos;
            }

            // Clip the loop end point to the End posisition
            if (ChCur->LoopEnd >= ChCur->EndPos)
               ChCur->LoopEnd = ChCur->EndPos;

            // Clip the loop start to the end point
            if (ChCur->LoopStart >= ChCur->EndPos)
               ChCur->LoopStart = 0;

	    // Sign conversion
            if ((Samp.Flags & INST_Signed) != 0)
               ChCur->Signed = true;
            else
               ChCur->Signed = false;

	    // Ping pong config
            if ((Samp.Flags & INST_PingPong) == INST_PingPong)
            {
               ChCur->PingPong = 1;
               ChCur->EndPos = ChCur->LoopEnd;
            }
            else
               ChCur->PingPong = 0;
	    
            Volume = 1;
            Inst = 1;

            // If the channel is playing then restart it
            if (ChCur->Playing == 1 && Stop == 0)
            {
               Start = 1;
               Pitch = 1;
            }
         }
      }

      // Start the sample playing
      if ((InCur->Flags & CHAN_Retrig) != 0 && ChCur->PlayStart != 0)
      {
         if (ChCur->PingPong == 10)
         {
            ChCur->LoopEnd = ChCur->EndPos;
            ChCur->PingPong = 1;
         }
         else
	    if (ChCur->PingPong != 1)
	       ChCur->PingPong = 0;

         ChCur->PlayPos = ChCur->PlayStart +
                   min(InCur->SampleOffset,ChCur->LoopEnd - ChCur->PlayStart);

         if (ChCur->PlayPos >= ChCur->LoopEnd ||
            (ChCur->Pitch == 0 && ((InCur->Flags & CHAN_Pitch) == 0)))
         {
            Stop = 1;
            Start = 0;
         }
         else
         {
            Stop = 0;
            Start = 1;
            Pitch = 1;
            ChCur->V[0].L = -1*ChCur->V[1].L;
            ChCur->V[0].R = -1*ChCur->V[1].R;
            ChCur->V[0].M = -1*ChCur->V[1].M;
            ChCur->MV[1].L = 0;
            ChCur->MV[1].R = 0;
            ChCur->MV[1].M = 0;
         }
      }

      // Set volume
      if ((InCur->Flags & CHAN_Volume) != 0)
      {
         ChCur->V[1].L = bound(-1*VolMax,InCur->LeftVol,VolMax);
         ChCur->V[1].R = bound(-1*VolMax,InCur->RightVol,VolMax);
         ChCur->V[1].M = bound(0L,InCur->MainVol,VolMax);
         if (Surround == false)
         {
            if (ChCur->V[1].L < 0)
               ChCur->V[1].L = -1*ChCur->V[1].L;
            if (ChCur->V[1].R < 0)
               ChCur->V[1].R = -1*ChCur->V[1].R;
         }
      }

      // Change pitch
      if ((InCur->Flags & CHAN_Pitch) != 0 && (ChCur->Pitch != InCur->Pitch))
      {
         double A = InCur->Pitch;
         double B = SRate;
         A = A/B;
         ChCur->Add = (unsigned long)A;
         B = A - ChCur->Add;
         B *= 0xFFFF;
         B *= 0xFFFF;
         ChCur->Incr = (unsigned long)B;
         ChCur->Incr >>= 16;
         ChCur->Incr <<= 16;
         ChCur->Pitch = InCur->Pitch;
         ChCur->FloatIncr = A;

         if (ChCur->Pitch == 0)
         {
            Stop = 1;
            Start = 0;
         }
         else
            Pitch = 1;
      }

      // New Loop
      if ((InCur->Flags & CHAN_NewLoop) != 0)
      {
         printf("NL: %lu,%lu,%lu\n",InCur->NewLoopBegin,InCur->NewLoopEnd,InCur->NewLoopFlags);
         if ((ChCur->Playing == 1 || Start == 1) && ChCur->CurInst < Samples.size())
         {
            ChCur->ReLoop = 1;
            museSample &Sample = Samples[ChCur->CurInst];
            unsigned char *NewEnd = Sample.Sample + bound(0,InCur->NewLoopEnd,Sample.SampleEnd);
            unsigned char *NewStart = Sample.Sample + bound(0,InCur->NewLoopBegin,Sample.SampleEnd);

            // What to do if with others!?
            if (InCur->NewLoopFlags == INST_Reverse)
            {
               if (ChCur->PlayPos > ChCur->LoopStart)
                  ChCur->PingPong = 11;
               else
                  ChCur->PingPong = 2;
               ChCur->LoopEnd = ChCur->LoopStart;
            }

            /* We are cutting the loop, the new loop start has no meaning,
               if we are doing a pingpong then the pingpong sequence is
               aborted and the sample instantly reverses. This should be
               consistant with GUS's capabilities. */
            if ((InCur->NewLoopFlags & INST_Loop) == 0)
            {
               ChCur->EndPos = NewEnd;
               ChCur->LoopEnd = NewEnd;
               ChCur->LoopStart = 0;
               ChCur->PingPong = 0;
               if (ChCur->PlayPos >= NewEnd)
               {
                  Stop = 1;
                  Start = 0;
               }
               else
                  Pitch = 1;
            }
            else
            {
               if ((InCur->NewLoopFlags & INST_PingPong) == INST_PingPong)
               {
                  // Activate pingponging if it was off before.
                  if (ChCur->PingPong == 0)
                     ChCur->PingPong = 1;
               }
               else
               {
                  // Abort any pingpong in progress
                  ChCur->PingPong = 0;
                  ChCur->LoopEnd = ChCur->EndPos;
               }

               // Setup the loop points
               ChCur->EndPos = NewEnd;
               ChCur->LoopStart = NewStart;
               ChCur->LoopEnd = NewEnd;

               // Compensate for the new loop points
               if (ChCur->PingPong == 10)
               {
                  ChCur->LoopEnd = NewStart;
                  if (NewStart >= ChCur->PlayPos)
                  {
                     ChCur->PingPong = 1;
                     ChCur->LoopEnd = NewEnd;
                  }
               }
               else
               {
                  // Past the end, do a loop now.
                  if (ChCur->PlayPos >= NewEnd)
                  {
                     if (ChCur->PingPong == 0)
                        ChCur->PlayPos = NewStart;
                     else
                        if (ChCur->Bit16 == true)
                           ChCur->PlayPos = NewEnd - 2;
                        else
                           ChCur->PlayPos = NewEnd - 1;
                  }
               }

               Pitch = 1;
            }
         }
      }

      // Compute the future volume on this channel
      if (In2Cur < In2End && (In2Cur->Flags & CHAN_Volume) != 0)
      {
         ChCur->V[2].L = bound(-1*VolMax,In2Cur->LeftVol,VolMax);
         ChCur->V[2].R = bound(-1*VolMax,In2Cur->RightVol,VolMax);
         ChCur->V[2].M = bound(0L,In2Cur->MainVol,VolMax);
         if (Surround == false)
         {
            if (ChCur->V[2].L < 0)
               ChCur->V[2].L = -1*ChCur->V[2].L;
            if (ChCur->V[2].R < 0)
               ChCur->V[2].R = -1*ChCur->V[2].R;
	 }
      }
      else
      {
         ChCur->V[2] = ChCur->V[1];
      }

      if (Stop == 1 && Start == 0)
      {
         Mixer->StopChan(I);
         ChCur->Playing = 0;
      }
      if (Inst == 1)
         Mixer->ChanInst(I);
      if (Start == 1 && ChCur->PlayPos != 0)
      {
         Mixer->StartChan(I);
         ChCur->Playing = 1;
      }
      if (Pitch == 1)
         Mixer->ChanPitch(I);
   };
   LastData.Duplicate(*Chans);

   // Perform the slide up from the last frame
   long Count = MidPoint;
   for (ChCur = Chns.begin(); ChCur != ChEnd; ChCur++, I++)
      Interp(ChCur->MV[2],ChCur->V[0],ChCur->V[1],Count,Steps);
   for (; Count != Steps; Count++)
   {
      // Setup the volumes
      I = 0;
      for (ChCur = Chns.begin(); ChCur != ChEnd; ChCur++, I++)
      {
         ChCur->MV[0] = ChCur->MV[1];
         ChCur->MV[1] = ChCur->MV[2];
         Interp(ChCur->MV[2],ChCur->V[0],ChCur->V[1],Count+1,Steps);

         Mixer->ChanVolume(I);
      }
   }

   // Move to the current frame (we are done with the last now)
   for (ChCur = Chns.begin(); ChCur != ChEnd; ChCur++, I++)
   {
      ChCur->V[0] = ChCur->V[1];
      ChCur->V[1] = ChCur->V[2];
   }

   // Perform the slide down to the next frame
   Count = 0;
   for (ChCur = Chns.begin(); ChCur != ChEnd; ChCur++, I++)
      Interp(ChCur->MV[2],ChCur->V[0],ChCur->V[1],Count,Steps);
   for (; Count != MidPoint; Count++)
   {
      // Setup the volumes
      I = 0;
      for (ChCur = Chns.begin(); ChCur != ChEnd; ChCur++, I++)
      {
         ChCur->MV[0] = ChCur->MV[1];
         ChCur->MV[1] = ChCur->MV[2];
         Interp(ChCur->MV[2],ChCur->V[0],ChCur->V[1],Count+1,Steps);

         Mixer->ChanVolume(I);
      }
   }

   // Actually perform the mixing.
   long Rc = Mixer->DoMix(Len);

   // Copy over the CHAN_FREE bit, one frame too late :<
   In2Cur = Chans->begin();
   ChCur = Chns.begin();
   I = 0;
   for (;(In2Cur != In2End) && (ChCur != ChEnd); ChCur++,I++,In2Cur++)
   {
      if (ChCur->Playing == 0 && ((In2Cur->Flags & CHAN_Retrig) == 0))
         In2Cur->Flags |= CHAN_Free;
   }

   ComputeMutex.UnLock();

   if (ForceCError != 0)
   {
      ForceCError = 0;
      return 1;
   };
   return Rc;
};
									/*}}}*/
// DACMixer::StopNote - Halts playback of all notes			/*{{{*/
// ---------------------------------------------------------------------
/* Simply 0's the CurInst for all the channels */
void museDACMixer::StopNotes()
{
   for (MixChannel *I = Chns.begin(); I != Chns.end(); I++)
   {
      if (I->Playing == 1)
         Mixer->StopChan(I - Chns.begin());

      memset(I,0,sizeof(*I));
      I->CurInst = 0xFFFF;
   }
}
									/*}}}*/
// DACMixer::StopWhenDone - Halt after the last byte is played		/*{{{*/
// ---------------------------------------------------------------------
/* The lowlevel DAC device calls this. It will finish off the last buffer
   but will not call GetNextBuffer. */
void museDACMixer::StopWhenDone()
{
   Mixer->DumpBuffer();
}
									/*}}}*/
// DACMixer::ForceCompError - Force compute to return an error code.	/*{{{*/
// ---------------------------------------------------------------------
/* This is used to gently halt playback from another thread. Format->Play()
   will return with an error code because compute returned an errro
   code. */
void museDACMixer::ForceCompError()
{
   ForceCError = 1;
}
									/*}}}*/

// DACMixerClass::museDACMixerClass - Constructor			/*{{{*/
// ---------------------------------------------------------------------
/* We add our configuration options right now. */
museDACMixerClass::museDACMixerClass()
{
   MajorVersion = 1;
   MinorVersion = 6;
   
   // This is the list of all options the class supports.
   IniOptions *Ini = GetIniOptions();
   Ini->AddOption("scale",0,0,false,0);
   Ini->AddOption("mono",0,0,true);
   Ini->AddOption("stereo",0,0,true);
   Ini->AddOption("8bit",0,0,true);
   Ini->AddOption("16bit",0,0,true);
   Ini->AddOption("surround",0,0,true);
   Ini->AddOption("srate",0,0,false);
}
									/*}}}*/

// FileOutput::museFileOutput - Base for all file output devices	/*{{{*/
// ---------------------------------------------------------------------
/* Provide a little help by storing the filename and extension */
museFileOutput::museFileOutput()
{
   FileName = 0;
   Extension = ".out";
}
   									/*}}}*/
// FileOutput::~museFileOutput - Destructor				/*{{{*/
// ---------------------------------------------------------------------
/* Free the filename */
museFileOutput::~museFileOutput()
{
   delete [] FileName;
}
   									/*}}}*/
// FileOutputClass::museFileOutputClass - Constructor			/*{{{*/
// ---------------------------------------------------------------------
/* This method is not inlined so that the class may have a base. */
museFileOutputClass::museFileOutputClass()
{
}
									/*}}}*/

