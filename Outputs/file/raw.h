/* ########################################################################

   RAW - RAW Output device

   ########################################################################
*/
#ifndef RAW_H
#define RAW_H

#include <dac.h>

class museFileRAW;
class museFileRAWClass;

class museFileRAW : public museFileOutput
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
   static museFileRAWClass *Meta;
   inline virtual MetaClass *GetMetaClass();

   virtual long InitPlay(char **Error);
   virtual long StopPlay();
   virtual void StopWhenDone();
   virtual void PausePlay();
   virtual void ResumePlay();
   virtual long TestOutput();

   virtual long SetMixParams(unsigned long SRate,octet Bits,bool Stereo);
   virtual long GetNextBuffer(octet **Begin,octet **End);

   museFileRAW();
   ~museFileRAW();
};

class museFileRAWClass : public museFileOutputClass
{
   public:

   virtual const char *GetTypeName();
   virtual octet IsSupported();

   virtual bool DescendedFrom(MetaClass *Base)
   {
      if (Base == museFileRAW::Meta)
         return true;
      return museFileOutputClass::DescendedFrom(Base);
   }
   virtual MetaObject *New() {return new museFileRAW;};
   virtual const char *GetName() {return "museFileRAW";};

   museFileRAWClass();
};

inline MetaClass *museFileRAW::GetMetaClass() {return Meta;};

#endif
