// -*- mode: cpp; mode: fold -*-
// Description								/*{{{*/
/* ######################################################################

   EffectFilter - Output filtering class that provides many effects
   
   This class provides volume and pitch envoloping for Muse Pattern
   Data. It operates directly on the output data reshaping it to the
   specifications given by the classes local variables.
   
   Full control over each channel is provided.

   The ranges for various elements are:
      Volume 0 -> 100 (float)
      Pitch 0 -> 400% (float)
      Speed 0 -> 400% (float)
      PanBalance  -PanMax -> PanMax
      PanDepth  -PanMax*2 -> PanMax*2 (default is PanMax)
      PanCenter -PanMax -> PanMax

   Pan Balance controls the volumes of the left and right channels
   independantly, negative values lower the right channel and positive
   values lower the left channel.

   Pan Depth controls the pan field separation, PanMax is normal, while
   PanMax*2 is enhanced separation, 0 is no separation and negative numbers
   perform a stereo flip.

   Pan Center controls the pan field center point, unlike balance a value
   of PanMax does not remove the left speaker totally from the output,
   it combines left and right equally and plays on the right channel, it
   controls the pan point that a panning of 0 represents.

   ##################################################################### */
									/*}}}*/
#ifndef EFXFILTR_H
#define EFXFILTR_H

#ifndef OUTPUTBS_H
#include <outputbs.h>
#endif

class museEffectFilter;
class museEffectClass;
class museOutputLink;

// This is all the effects that can be applied to a channel
struct museChanEfx
{
   float Volume;
   float Pitch;
   long PanBalance;
   long PanDepth;
   long PanCenter;
   unsigned long Disabled;      // 0 = Not; 1 = Disabled; 2 = Blank
};

#define SequenceChanEfx Sequence<museChanEfx>

// Class to bounce calls to a different target, makes museOutputBase non abstract
class museOutputLink : public museOutputBase
{
   public:

   museOutputBase *NextLink;

   virtual unsigned long SecondsToLen(float Seconds);
   virtual float LenToSeconds(unsigned long Len);
   virtual void SetChannelMask(Sequencebool *Mask);
   virtual void SetVolumeScale(unsigned short Factor);
   virtual void LoadSamples(SequenceSample *Samples);
   virtual long Compute(SequenceChannel *Channels, unsigned long Len);
   virtual void SetFrameInfo(museFrameInfo *Frame);
   virtual unsigned long Sync(unsigned long Time);
   virtual char *GetCurOptionDesc();

   virtual long GetMaxChannels();
   virtual void SetChanVolPot(unsigned long Pot);

   virtual void StopNotes();

   virtual long InitPlay(char **Error);
   virtual long StopPlay();
   virtual void StopWhenDone();
   virtual void PausePlay();
   virtual void ResumePlay();
   virtual void ForceCompError();

   virtual unsigned long SetMaxChannels(unsigned long Count);

   museOutputLink();
};

class musePrintFilter : public museOutputLink
{
   public:

   int SoloChan;   // Set to only show 1 channel
   virtual long Compute(SequenceChannel *Channels, unsigned long Len);

   musePrintFilter();
};

class museEffectFilter : public museOutputLink
{
   unsigned long Flags;

   SequenceChanEfx ChanEfx;
   SequenceChannel ChanState;
   threadMutex Mutex;

   museChanEfx Normal;
   museChanEfx Current;
   float ISpeed;
   public:

   float Volume;
   float Pitch;
   float Speed;
   long PanBalance;
   long PanDepth;
   long PanCenter;

   virtual void SetChanEfx(SequenceChanEfx *Efx);
   void InitChanEfx(SequenceChanEfx *Efx);
   virtual void SetChanged(unsigned long Flags);
   virtual long Compute(SequenceChannel *Channels, unsigned long Len);
   void Reset();

   museEffectFilter();
   ~museEffectFilter();
};

#endif
