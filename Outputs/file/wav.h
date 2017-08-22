/* ########################################################################

   WAV - WAV Output device

   ########################################################################
*/
#ifndef WAV_H
#define WAV_H

#include <dac.h>

class museFileWAV;
class museFileWAVClass;

class museFileWAV : public museFileOutput
{
   unsigned long SamplingRate;
   octet Bits;
   bool Stereo;
   bool Opened;
   int Handle;
   octet *Buffer;
   unsigned long BufferSize;
   unsigned long TotalSize;
   octet Play;

   public:
   static museFileWAVClass *Meta;
   inline virtual MetaClass *GetMetaClass();

   virtual long InitPlay(char **Error);
   virtual long StopPlay();
   virtual void StopWhenDone();
   virtual void PausePlay();
   virtual void ResumePlay();
   virtual long TestOutput();

   virtual long SetMixParams(unsigned long SRate,octet Bits,bool Stereo);
   virtual long GetNextBuffer(octet **Begin,octet **End);

   museFileWAV();
   ~museFileWAV();
};

class museFileWAVClass : public museFileOutputClass
{
   public:

   virtual const char *GetTypeName();
   virtual octet IsSupported();

   virtual bool DescendedFrom(MetaClass *Base)
   {
      if (Base == museFileWAV::Meta)
         return true;
      return museFileOutputClass::DescendedFrom(Base);
   }
   virtual MetaObject *New() {return new museFileWAV;};
   virtual const char *GetName() {return "museFileWAV";};

   museFileWAVClass();
};

inline MetaClass *museFileWAV::GetMetaClass() {return Meta;};

#endif
