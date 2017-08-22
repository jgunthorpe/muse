// -*- mode: cpp; mode: fold -*-
// Description								/*{{{*/
/* ######################################################################

   OutputBase - Base class for all Muse Output systems
   
   An output device is used to handle Muse Pattern Data. Muse Patten
   Data is compose of an array of museChannel structures, each describing
   a delta change in the current channel. MPD could be used directly
   to drive a GUS (but isn't see the gus code).

   The basic sequence of events to use an Output device is to Initialize
   the device, download the samples and then begin sending pattern data.

   Timing is achived by always using Device Units. Device Units vary from
   device to device so the function SecondsToLen will return the Device
   Units which corrispond to that time interval, the reverse function
   LenToSeconds will invert the process. -ANY- cumulative time computing
   must be based on Device Units and NOT seconds. If seconds are used then
   the inherent round off kills the accuracy of the math. With a digital
   output device Device Units corrispond to the number of samples it will
   take for the interval (sampling rate).
   
   ##################################################################### */
									/*}}}*/
#ifndef OUTPUTBS_H
#define OUTPUTBS_H

struct museFrameInfo
{
   signed long Order;
   signed long Row;
   signed long Pattern;
   unsigned long Frame;
   unsigned long Time;
   unsigned long SongTime;
   unsigned long GlobalVolume;
   unsigned long Tempo;
   unsigned long Speed;
   unsigned long PlayingChannels;
};

struct museSample
{
   octet *Sample;
   unsigned long Flags;
   unsigned long LoopBegin;
   unsigned long LoopEnd;
   unsigned long SampleEnd;
   char *Name;
   unsigned long MaxNameLen;
   char *SampleName;
   unsigned long MaxSNameLen;
   unsigned long InstNo;
   unsigned long Center;
   long Volume;
   long FineTune;
};

struct museChannel
{
   unsigned long Flags;
   long LeftVol;
   long RightVol;
   long MainVol;
   long Pan;                    // Panning Point
   long Pitch;
   unsigned long Sample;
   unsigned long SampleOffset;
   unsigned long ModuleChannel;
   unsigned long NewLoopEnd;
   unsigned long NewLoopBegin;
   unsigned long NewLoopFlags;
};

#define SequenceSample Sequence<museSample>
#define SequenceChannel Sequence<museChannel>
#define SequenceOutputClass Sequence<museOutputClass *>

class museOutputBase;
class museOutputClass;

class museOutputBase : public MetaObject
{
   public:
   static museOutputClass *Meta;
   inline virtual MetaClass *GetMetaClass();

   virtual void SetChannelMask(Sequencebool *Mask) = 0;
   virtual unsigned long SecondsToLen(float Seconds) = 0;
   virtual void SetVolumeScale(unsigned short Factor) = 0;
   virtual void LoadSamples(SequenceSample *Samples) = 0;
   virtual void SetFrameInfo(museFrameInfo *Frame);
   virtual long Compute(SequenceChannel *Channels, unsigned long Len) = 0;
   virtual unsigned long Sync(unsigned long /*Time*/) {return 0;};

   virtual void StopNotes() = 0;

   virtual long InitPlay(char **Error) = 0;
   virtual long StopPlay() = 0;
   virtual void StopWhenDone() = 0;
   virtual void PausePlay() = 0;
   virtual void ResumePlay() = 0;

   virtual void ForceCompError() = 0;

   virtual char *GetCurOptionDesc() = 0;

   virtual unsigned long SetMaxChannels(unsigned long Count);
   virtual long GetMaxChannels();
   virtual void SetChanVolPot(unsigned long Pot);
   virtual float LenToSeconds(unsigned long Len) = 0;

   long MaxChannels;
   bool RealTime;
   unsigned long ChanVolPot;   // Volume Potential for all channels
   bool Downloader;

   museOutputBase();
};

class museOutputClass : public MetaClass
{
   public:

   octet Level;

   virtual const char *GetTypeName() {return 0;};
   virtual octet IsSupported() {return 0xFF;};

   virtual bool DescendedFrom(MetaClass *Base)
   {
      if (Base == museOutputBase::Meta)
         return true;
      return MetaClass::DescendedFrom(Base);
   }
   virtual MetaObject *New() {return 0;};
   virtual const char *GetName() {return "museOutputBase";};

   virtual void Link() {};
   museOutputClass();
};

MetaClass *museOutputBase::GetMetaClass() {return Meta;};

#endif
