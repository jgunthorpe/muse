/* ########################################################################

   Generic Wave Table front end

   ########################################################################
*/
#ifndef WAVETBL_HC
#define WAVETBL_HC

#include <OutputBs.hc>

struct WavChannelState
{
   boolean Enabled;
   short Sample;
   long Pitch;
   long Pan;
   long Volume;
   long OrgLeft;
   long OrgRight;
   long OrgMain;
   short Channel1;
   short Channel2;
   unsigned long FreqMul;
};

struct WavInstData
{
   unsigned long Flags;
   unsigned long MemLoc;
   unsigned long LoopBegin;
   unsigned long LoopEnd;
   unsigned long SampleEnd;
   unsigned long Size;
   unsigned long FreqMul;
   octet Conv16to8;
};

typedef IDLSequence<WavChannelState> SeqWavChannelState;
typedef IDLSequence<WavInstData> SeqWavInstData;

class museWaveTableMixer;
class museWaveTableClass;

class museWaveTable : public museOutputBase
{
   SeqWavChannelState Channels;
   SeqWavInstData Instruments;
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

   protected:
   int MaxChans;
   int MidChans;
   int MinChans;
   float SecDiv;
   int MaxTime;

   virtual long OpenDevice(char **Error);
   virtual void CloseDevice();

   virtual void SetVoiceCount(int Count);
   virtual void WaitTimeTick();
   virtual void SetVolume(int Channel,unsigned long Vol,int Rate);
   virtual void StartTimer(unsigned long Len);
   virtual void StopTimer();
   virtual void StartVoice(int Channel,unsigned long Offset,
                           unsigned long LoopBegin,unsigned long LoopEnd,
                           unsigned long Flags);
   virtual void StopVoice(int Channel);
   virtual void SetAll(int Channel,signed long Pan,unsigned long Freq,unsigned long Volume,int Rate);
   virtual void SetFrequency(int Channel,unsigned long Freq);
   virtual unsigned long RAMSize();
   virtual int IsStopped(int Channel);
   virtual void Download(char *Data,int Signed,unsigned long MemLoc,unsigned int Size);

   public :
   static museWaveTableClass *__ClassObject;
   inline virtual SOMClass *somGetClass();

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
   virtual float LenToSeconds(unsigned long Len);

   museWaveTable();
   ~museWaveTable();
};

class museWaveTableClass : public museOutputClass
{
   public:

   virtual boolean somDescendedFrom(SOMClass *Base)
   {
      if (Base == museWaveTable::__ClassObject)
         return TRUE;
      return museOutputClass::somDescendedFrom(Base);
   }
   virtual SOMObject *somNew() {return 0;};
   virtual string somGetName() {return "museWaveTable";};

   museWaveTableClass();
};

inline SOMClass *museWaveTable::somGetClass() {return __ClassObject;};

#endif
