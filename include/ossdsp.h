// -*- mode: cpp; mode: fold -*-
// Description								/*{{{*/
/* ######################################################################

   DSP - OSS (Open Sound System) DSP driver.
   
   OSS is a sound system used in unix, this driver make use of the 
   /dev/dsp device for output. It supposedly exists on many flavors of
   unix, but this driver was written on linux for the 2.0.27 kernel. It
   is incredibly likely that it will work on all other version of OSS.
   
   It performs automatic detection of the devices capabilities and 
   will use the highest possible output quality unless overriden. It
   syncronizes it's internal block size with the size the OSS uses in the
   driver (as recomended by the docs).

   ##################################################################### */
									/*}}}*/
#ifndef OSSDSP_HC
#define OSSDSP_HC

#include <dac.h>

class museOSSDSP;
class museOSSDSPClass;

class museOSSDSP : public museDACMixer
{
   unsigned long SamplingRate;
   octet Bits;
   bool Stereo;
   bool Opened;
   octet Play;            // Current playing state

   // Buffer managment
   octet *Buffer;
   int BlockSize;
   int Blocks;
   unsigned long BufSize;

   // Queue stuff
   int WritePos;
   int ReadPos;
   
   // /dev/dsp fd
   int FD;
   void Close();
   
   public:

   static museOSSDSPClass *Meta;
   inline virtual MetaClass *GetMetaClass();

   virtual long InitPlay(char **Error);
   virtual long StopPlay();
   virtual void StopWhenDone();
   virtual void PausePlay();
   virtual void ResumePlay();
   virtual unsigned long Sync(unsigned long Time);
   
   virtual long SetMixParams(unsigned long SRate,octet Bits,bool Stereo);
   virtual long GetNextBuffer(octet **Begin,octet **End);

   museOSSDSP();
   ~museOSSDSP();
};

class museOSSDSPClass : public museDACMixerClass
{
   public:

   virtual const char *GetTypeName();
   virtual octet IsSupported();

   virtual bool DescendedFrom(MetaClass *Base)
   {
      if (Base == museOSSDSP::Meta)
         return true;
      return museDACMixerClass::DescendedFrom(Base);
   }
   virtual MetaObject *New() {return new museOSSDSP;};
   virtual const char *GetName() {return "museOSSDSP";};

   museOSSDSPClass();
};

inline MetaClass *museOSSDSP::GetMetaClass() {return Meta;};

#endif
