/* ########################################################################

   Mixer - Base for the mixers

   ########################################################################
*/
#ifndef MIXER_H
#define MIXER_H

class museMixerBase;
class museMixerClass;
class museOutputFilterBase;
class museOFilterClass;
class museFilterBase;
class museFilterClass;

#define SequenceFilter Sequence<museFilterBase *>

#ifndef DAC_H
#include <dac.h>
#endif

class museMixerBase : public MetaObject
{
   public:
   museDACMixer *Owner;

   static museMixerClass *Meta;
   inline virtual MetaClass *GetMeta();

   virtual long DoMix(unsigned long Len) = 0;
   virtual void StopChan(int Chan) = 0;
   virtual void StartChan(int Chan) = 0;
   virtual void ChanVolume(int Chan) = 0;
   virtual void ChanPitch(int Chan) = 0;
   virtual void ChanInst(int Chan) = 0;
   virtual void Reset() = 0;
   virtual void Channels(int I) = 0;
   virtual void DumpBuffer() = 0;
   virtual signed long ComputeSConst(unsigned long Percent,unsigned long Chans) = 0;

   virtual ~museMixerBase();
};

class museMixerClass : public MetaClass
{
   public:

   virtual const char *GetTypeName() {return 0;};

   virtual bool DescendedFrom(MetaClass *Base)
   {
      if (Base == museMixerBase::Meta)
         return true;
      return MetaClass::DescendedFrom(Base);
   }
   virtual MetaObject *New() {return 0;};
   virtual const char *GetName() {return "museMixerBase";};

   virtual void Link() {};
   museMixerClass();
};
MetaClass *museMixerBase::GetMeta() {return Meta;};

class museOutputFilterBase : public MetaObject
{
   public:
   museDACMixer *Owner;

   static museOFilterClass *Meta;
   inline virtual MetaClass *GetMetaClass();

   // 32 bit stereo filters
   virtual void Filter16(unsigned short *Out,unsigned short *OutEnd,signed long *In) = 0;
   virtual void Filter8(unsigned char *Out,unsigned char *OutEnd,signed long *In) = 0;

   // 32 bit mono filters
   virtual void Filter16m(unsigned short *Out,unsigned short *OutEnd,signed long *In) = 0;
   virtual void Filter8m(unsigned char *Out,unsigned char *OutEnd,signed long *In) = 0;

   virtual ~museOutputFilterBase();
};

class museOFilterClass : public MetaClass
{
   public:

   virtual const char *GetTypeName() {return 0;};

   virtual bool DescendedFrom(MetaClass *Base)
   {
      if (Base == museOutputFilterBase::Meta)
         return true;
      return MetaClass::DescendedFrom(Base);
   }
   virtual MetaObject *New() {return 0;};
   virtual const char *GetName() {return "museOutputFilterBase";};

   virtual void Link() {};
   museOFilterClass();
};
MetaClass *museOutputFilterBase::GetMetaClass() {return Meta;};

class museFilterBase : public MetaObject
{
   public:
   museDACMixer *Owner;

   static museFilterClass *Meta;
   inline virtual MetaClass *GetMetaClass();

   // 32 bit stereo filters
   virtual void Filter(signed long *Out,signed long *OutEnd) = 0;
   virtual void Filterm(signed long *Out,signed long *OutEnd) = 0;

   virtual ~museFilterBase();
};

class museFilterClass : public MetaClass
{
   public:

   virtual char *GetTypeName() {return 0;};

   virtual bool DescendedFrom(MetaClass *Base)
   {
      if (Base == museFilterBase::Meta)
         return true;
      return MetaClass::DescendedFrom(Base);
   }
   virtual MetaObject *New() {return 0;};
   virtual const char *GetName() {return "museFilterBase";};

   virtual void Link() {};
   museFilterClass();
};
MetaClass *museFilterBase::GetMetaClass() {return Meta;};

#endif
