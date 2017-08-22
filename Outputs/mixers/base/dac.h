/* ########################################################################

   DAC - Base class for all Muse/2 Digital Output types

   ########################################################################
*/
#ifndef DAC_H
#define DAC_H

#ifndef OUTPUTBS_H
#include <outputbs.h>
#endif

class museDACMixer;
class museDACMixerClass;
class museFileOutput;
class museFileOutputClass;

#ifndef MIX_H
#include <mix.h>
#endif

class museDACMixer : public museOutputBase
{
   octet NumChans;
   bool Surround;

   threadMutex ComputeMutex;
   bool ForceCError;

   unsigned long ScalePercent;

   museMixerBase *Mixer;

   public:
   struct Vol
   {
      long L;
      long R;
      long M;
   };
   void Interp(Vol &MV,Vol &VF,Vol &VT,signed long Count,signed long Steps);

   struct MixChannel
   {
      unsigned long CurInst;                // Selected instrument
      unsigned char *PlayPos;     // Current sample position
      unsigned char *PlayStart;   // Very first byte in the sample
      unsigned char *EndPos;      // End position
      unsigned char *LoopStart;   // Loop start point
      unsigned char *LoopEnd;     // Loop end point
      int Bit16;                  // 1 if 16 bit sample
      int Signed;                 // 1 if signed
      int PingPong;               // 1 if a ping pong loop
      int Playing;                // 1 if playing
      int Enabled;                // 1 if enabled
      int ReLoop;                 // 1 if the loop should be rewritten on retrig

      unsigned long Incr;        // Amount to add to CurIncr
      unsigned long Add;         // Amount to add to Play Pos
      unsigned long CurIncr;

      float FloatIncr;
      long Pitch;

      // Past, Present and Future volumes
      Vol V[3];
      Vol MV[3];
   };

   SequenceChannel LastData;
   Sequence<MixChannel> Chns;
   SequenceSample Samples;
   unsigned long SRate;
   octet MBits;
   bool MStereo;
   octet *OutBegin;
   octet *OutEnd;
   museOutputFilterBase *OutFilter;
   SequenceFilter FilterStack;
   signed long ScalingConstant;

   static museDACMixerClass *Meta;
   inline virtual MetaClass *GetMetaClass();

   virtual long SetMixParams(unsigned long SRate,octet Bits,bool Stereo);
   virtual void GetMixParams(unsigned long *SRate,octet *Bits,bool *Stereo);
   virtual long GetNextBuffer(octet **Begin,octet **End) = 0;

   virtual unsigned long SecondsToLen(float Seconds);
   virtual float LenToSeconds(unsigned long Len);
   virtual unsigned long SetMaxChannels(unsigned long Count);
   unsigned long SetScale(unsigned short Percent);

   virtual void SetChannelMask(Sequencebool *Mask);
   virtual void SetVolumeScale(unsigned short Factor);
   virtual void LoadSamples(SequenceSample *Samples);
   virtual long Compute(SequenceChannel *Channels, unsigned long Len);
   virtual long InitPlay(char **Error);
      
   virtual void StopNotes();
   virtual void StopWhenDone();

   virtual void ForceCompError();

   virtual char *GetCurOptionDesc();

   museMixerBase *UseMixer(museMixerBase *Mixer);
   museOutputFilterBase *UseOFilter(museOutputFilterBase *Filter);
   void AddFilter(museFilterBase *Filter);
   void RemoveFilter(museFilterBase *Filter);
   void GetFilterStack(SequenceFilter *Filters);
   void SetFilterStack(SequenceFilter *Filters);

   museDACMixer();
   ~museDACMixer();
};

class museDACMixerClass : public museOutputClass
{
   public:

   virtual bool DescendedFrom(MetaClass *Base)
   {
      if (Base == museDACMixer::Meta)
         return true;
      return museOutputClass::DescendedFrom(Base);
   }
   virtual MetaObject *New() {return 0;};
   virtual const char *GetName() {return "museDACMixer";};

   museDACMixerClass();
};

class museFileOutput : public museDACMixer
{
   public:
   static museFileOutputClass *Meta;
   inline virtual MetaClass *GetMetaClass();

   char *FileName;
   char *Extension;

   museFileOutput();
   ~museFileOutput();
};

class museFileOutputClass : public museDACMixerClass
{
   public:

   virtual bool DescendedFrom(MetaClass *Base)
   {
      if (Base == museFileOutput::Meta)
         return true;
      return museOutputClass::DescendedFrom(Base);
   }
   virtual MetaObject *New() {return 0;};
   virtual const char *GetName() {return "museFileOutput";};

   museFileOutputClass();
};

inline MetaClass *museDACMixer::GetMetaClass() {return Meta;};
inline MetaClass *museFileOutput::GetMetaClass() {return Meta;};

#endif
