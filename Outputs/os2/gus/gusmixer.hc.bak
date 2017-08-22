/* ########################################################################

   GUSMixer - GUS Output Class

   ########################################################################
*/
#ifndef GUSMIXER_HC
#define GUSMIXER_HC

#include <OutputBs.hc>

typedef struct
{
   boolean Enabled;
   short Sample;
   long Pitch;
   long Balance;
   long TargetB;
   long Volume;
   long OrgLeft;
   long OrgRight;
   long OrgMain;
   short Channel1;
   short Channel2;
   unsigned long FreqMul;
} GUSChannelState;

typedef struct
{
   unsigned long Flags;
   unsigned long MemLoc;
   unsigned long LoopBegin;
   unsigned long LoopEnd;
   unsigned long SampleEnd;
   unsigned long Size;
   unsigned long FreqMul;
   octet Conv16to8;
} GUSInstData;

typedef IDLSequence<GUSChannelState> SeqGUSChannelState;
typedef IDLSequence<GUSInstData> SeqGUSInstData;

class museGUSMixer;
class museGUSMixerClass;

class museGUSMixer : public museOutputBase
{
   SeqGUSChannelState Channels;
   SeqGUSInstData Instruments;
   octet NumChans;
   octet PlayMode;
   unsigned long LastTime;
   octet ChanList[33];
   short NextChan;
   short LastChan;
   long TempoDiv;
   unsigned long ComputeMutex;
   unsigned long SyncSem;
   unsigned long CurrentTime;
   unsigned long WaitTime;
   boolean Pause;
   unsigned long Version;

   public :
   static museGUSMixerClass *__ClassObject;
   inline virtual SOMClass *somGetClass();

   long OpenGUS(string* Error);
   long CloseGUS();

   virtual unsigned long SecondsToLen(float Seconds);
   virtual void SetChannelMask(SequenceBoolean* Mask);
   virtual void SetVolumeScale(unsigned short Factor);
   virtual void LoadSamples(SequenceSample* Samples);
   virtual long Compute(SequenceChannel* Channels, unsigned long Len);
   virtual unsigned long Sync(unsigned long Time);
   virtual void StopWhenDone();
   virtual void StopNotes();
   virtual long InitPlay(string* Error);
   virtual long StopPlay();
   virtual void PausePlay();
   virtual void ResumePlay();
   virtual long TestOutput();
   virtual void ForceCompError();
   virtual void SetOptions(string Options);
   virtual string GetCurOptionDesc();
   virtual float LenToSeconds(unsigned long Len);

   museGUSMixer();
   ~museGUSMixer();
};

class museGUSMixerClass : public museOutputClass
{
   octet DetectStatus;

   public:

   virtual string GetTypeName();
   virtual void GetOptionHelp(SequenceString* OptList);
   virtual octet IsSupported();

   virtual boolean somDescendedFrom(SOMClass *Base)
   {
      if (Base == museGUSMixer::__ClassObject)
         return TRUE;
      return museOutputClass::somDescendedFrom(Base);
   }
   virtual SOMObject *somNew() {return new museGUSMixer;};
   virtual string somGetName() {return "museGUSMixer";};

   museGUSMixerClass();
};

inline SOMClass *museGUSMixer::somGetClass() {return __ClassObject;};

#endif
